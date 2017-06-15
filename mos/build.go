package main

import (
	"archive/zip"
	"bytes"
	"context"
	"encoding/hex"
	"fmt"
	"io"
	"io/ioutil"
	"mime/multipart"
	"net/http"
	"os"
	"os/exec"
	"path"
	"path/filepath"
	"regexp"
	"runtime"
	"strings"
	"text/template"
	"time"
	"unicode"

	"cesanta.com/common/go/multierror"
	"cesanta.com/common/go/ourio"
	"cesanta.com/mos/build"
	"cesanta.com/mos/build/archive"
	"cesanta.com/mos/build/gitutils"
	"cesanta.com/mos/dev"
	"cesanta.com/mos/flash/common"
	"github.com/cesanta/errors"
	"github.com/golang/glog"
	flag "github.com/spf13/pflag"
	yaml "gopkg.in/yaml.v2"
)

// mos build specific advanced flags
var (
	buildImages = flag.String("docker_images",
		"esp8266=docker.cesanta.com/mg-iot-cloud-project-esp8266:release,"+
			"cc3200=docker.cesanta.com/mg-iot-cloud-project-cc3200:release",
		"build images, arch1=image1,arch2=image2")
	cleanBuild    = flag.Bool("clean", false, "perform a clean build, wipe the previous build state")
	keepTempFiles = flag.Bool("keep-temp-files", false, "keep temp files after the build is done (by default they are in ~/.mos/tmp)")
	modules       = flag.StringSlice("module", []string{}, "location of the module from mos.yaml, in the format: \"module_name:/path/to/location\". Can be used multiple times.")
	libs          = flag.StringSlice("lib", []string{}, "location of the lib from mos.yaml, in the format: \"lib_name:/path/to/location\". Can be used multiple times.")

	buildDockerExtra = flag.StringSlice("build-docker-extra", []string{}, "extra docker flags, added before image name. Can be used multiple times: e.g. --build-docker-extra -v --build-docker-extra /foo:/bar.")
	buildCmdExtra    = flag.StringSlice("build-cmd-extra", []string{}, "extra make flags, added at the end of the make command. Can be used multiple times.")

	buildVarsSlice []string

	noLibsUpdate = flag.Bool("no-libs-update", false, "if true, never try to pull existing libs (treat existing default locations as if they were given in --lib)")

	tmpDir     = ""
	libsDir    = ""
	appsDir    = ""
	modulesDir = ""

	varRegexp = regexp.MustCompile(`\$\{[^}]+\}`)
)

const (
	buildDir = "build"
	codeDir  = "."

	buildLog           = "build.log"
	depsInitCFileName  = "deps_init.c"
	confSchemaFileName = "mos_conf_schema.yml"

	// Manifest version changes:
	//
	// - 2017-06-03: added support for @all_libs in filesystem and sources
	minManifestVersion = "2017-03-17"
	maxManifestVersion = "2017-06-03"

	localLibsDir = "local_libs"

	allLibsKeyword = "@all_libs"
)

func init() {
	hiddenFlags = append(hiddenFlags, "docker_images")

	flag.StringSliceVar(&buildVarsSlice, "build-var", []string{}, "build variable in the format \"NAME:VALUE\" Can be used multiple times.")

	// deprecated since 2017/05/11
	flag.StringSliceVar(&buildVarsSlice, "build_var", []string{}, "deprecated, use --build-var")

	flag.StringVar(&tmpDir, "temp-dir", "~/.mos/tmp", "Directory to store temporary files")
	flag.StringVar(&libsDir, "libs-dir", "~/.mos/libs", "Directory to store libraries into")
	flag.StringVar(&appsDir, "apps-dir", "~/.mos/apps", "Directory to store apps into")
	flag.StringVar(&modulesDir, "modules-dir", "~/.mos/modules", "Directory to store modules into")

}

// buildInit() should be called after all flags are parsed
func buildInit() error {
	// Unfortunately user.Current() doesn't play nicely with static build, so
	// we have to get home directory from the environment

	homeEnvName := "HOME"
	if runtime.GOOS == "windows" {
		homeEnvName = "USERPROFILE"
	}

	homeDir := os.Getenv(homeEnvName)
	// Replace tilda with the actual path to home directory
	if len(tmpDir) > 0 && tmpDir[0] == '~' {
		tmpDir = homeDir + tmpDir[1:]
	}
	if len(libsDir) > 0 && libsDir[0] == '~' {
		libsDir = homeDir + libsDir[1:]
	}
	if len(appsDir) > 0 && appsDir[0] == '~' {
		appsDir = homeDir + appsDir[1:]
	}
	if len(modulesDir) > 0 && modulesDir[0] == '~' {
		modulesDir = homeDir + modulesDir[1:]
	}

	// Absolutize all given paths
	var err error
	tmpDir, err = filepath.Abs(tmpDir)
	if err != nil {
		return errors.Trace(err)
	}

	libsDir, err = filepath.Abs(libsDir)
	if err != nil {
		return errors.Trace(err)
	}

	appsDir, err = filepath.Abs(appsDir)
	if err != nil {
		return errors.Trace(err)
	}

	modulesDir, err = filepath.Abs(modulesDir)
	if err != nil {
		return errors.Trace(err)
	}

	if err := os.MkdirAll(tmpDir, 0777); err != nil {
		return errors.Trace(err)
	}

	return nil
}

type buildParams struct {
	Arch               string
	CustomLibLocations map[string]string
}

func buildHandler(ctx context.Context, devConn *dev.DevConn) error {
	cll, err := getCustomLibLocations()
	if err != nil {
		return errors.Trace(err)
	}

	bParams := buildParams{
		Arch:               *arch,
		CustomLibLocations: cll,
	}

	return errors.Trace(doBuild(ctx, &bParams))
}

func doBuild(ctx context.Context, bParams *buildParams) error {
	var err error
	if *local {
		err = buildLocal(ctx, bParams)
	} else {
		err = buildRemote(bParams)
	}
	if err != nil {
		return errors.Trace(err)
	}

	fwFilename := filepath.Join(buildDir, build.FirmwareFileName)

	fw, err := common.NewZipFirmwareBundle(fwFilename)
	if err == nil {
		reportf("Success, built %s/%s version %s (%s).", fw.Name, fw.Platform, fw.Version, fw.BuildID)
	}

	reportf("Firmware saved to %s", fwFilename)

	return err
}

func buildLocal(ctx context.Context, bParams *buildParams) (err error) {
	defer func() {
		if !*verbose && err != nil {
			log, err := os.Open(path.Join(buildDir, buildLog))
			if err != nil {
				glog.Errorf("can't read build log: %s", err)
				return
			}
			io.Copy(os.Stdout, log)
		}
	}()

	dockerAppPath := "/app"
	dockerMgosPath := "/mongoose-os"

	genDir, err := filepath.Abs(path.Join(buildDir, "gen"))
	if err != nil {
		return errors.Trace(err)
	}

	fwDir := filepath.Join(buildDir, "fw")
	fwDirDocker := path.Join(buildDir, "fw")

	objsDir := filepath.Join(buildDir, "objs")
	objsDirDocker := path.Join(buildDir, "objs")

	fwFilename := filepath.Join(buildDir, build.FirmwareFileName)

	elfFilename := filepath.Join(objsDir, "fw.elf")

	// Perform cleanup before the build {{{
	if *cleanBuild {
		err = os.RemoveAll(buildDir)
		if err != nil {
			return errors.Trace(err)
		}
	} else {
		// This is not going to be a clean build, but we should still remove fw.zip
		// (ignoring any possible errors)
		os.Remove(fwFilename)
	}
	// }}}

	// Prepare build dir and log file {{{
	err = os.MkdirAll(buildDir, 0777)
	if err != nil {
		return errors.Trace(err)
	}

	blog := filepath.Join(buildDir, buildLog)
	logFile, err := os.Create(blog)
	if err != nil {
		return errors.Trace(err)
	}
	defer logFile.Close()
	// }}}

	// Prepare gen dir
	if err := os.MkdirAll(genDir, 0777); err != nil {
		return errors.Trace(err)
	}

	// Create map of given module locations, via --module flag(s)
	customModuleLocations := map[string]string{}
	for _, m := range *modules {
		parts := strings.SplitN(m, ":", 2)
		customModuleLocations[parts[0]] = parts[1]
	}

	mVars := NewManifestVars()

	appDir, err := getCodeDir()
	if err != nil {
		return errors.Trace(err)
	}

	manifest, mtime, err := readManifestWithLibs(
		appDir, bParams, nil, nil, logFile, libsDir, false, /* skip clean */
	)
	if err != nil {
		return errors.Trace(err)
	}

	if err := expandManifestAllLibsPaths(manifest); err != nil {
		return errors.Trace(err)
	}

	if manifest.Arch == "" {
		return errors.Errorf("--arch must be specified or mos.yml should contain an arch key")
	}

	mVars.SetVar("arch", manifest.Arch)

	// Prepare local copies of all sw modules {{{
	for _, m := range manifest.Modules {
		name, err := m.GetName()
		if err != nil {
			return errors.Trace(err)
		}

		targetDir, ok := customModuleLocations[name]
		if !ok {
			// Custom module location wasn't provided in command line, so, we'll
			// use the module name and will clone/pull it if necessary
			reportf("The flag --module is not given for the module %q, going to use the repository", name)

			var err error
			targetDir, err = m.PrepareLocalDir(modulesDir, logFile, true)
			if err != nil {
				return errors.Trace(err)
			}
		} else {
			reportf("Using module %q located at %q", name, targetDir)
		}

		setModuleVars(mVars, name, targetDir)
	}
	// }}}

	// Determine mongoose-os dir (mosDirEffective) {{{
	var mosDirEffective string
	if *mosRepo != "" {
		reportf("Using mongoose-os located at %q", *mosRepo)
		mosDirEffective = *mosRepo
	} else {
		reportf("The flag --repo is not given, going to use mongoose-os repository")

		m := build.SWModule{
			Type: "git",
			// TODO(dfrank) get upstream repo URL from a flag
			// (and this flag needs to be forwarded to fwbuild as well, which should
			// forward it to the mos invocation)
			Origin:  "https://github.com/cesanta/mongoose-os",
			Version: manifest.MongooseOsVersion,
		}

		var err error
		mosDirEffective, err = m.PrepareLocalDir(modulesDir, logFile, true)
		if err != nil {
			return errors.Trace(err)
		}
	}
	setModuleVars(mVars, "mongoose-os", mosDirEffective)

	mosDirEffectiveAbs, err := filepath.Abs(mosDirEffective)
	if err != nil {
		return errors.Annotatef(err, "getting absolute path of %q", mosDirEffective)
	}
	// }}}

	// Get sources and filesystem files from the manifest, expanding placeholders {{{
	appSources := []string{}
	for _, s := range manifest.Sources {
		s, err = mVars.ExpandVars(s)
		if err != nil {
			return errors.Trace(err)
		}
		appSources = append(appSources, s)
	}

	appFSFiles := []string{}
	for _, s := range manifest.Filesystem {
		s, err = mVars.ExpandVars(s)
		if err != nil {
			return errors.Trace(err)
		}
		appFSFiles = append(appFSFiles, s)
	}
	// }}}

	appSourceDirs := []string{}
	appFSDirs := []string{}

	// Generate deps_init C code, and if it's not empty, write it to the temp
	// file and add to sources
	depsCCode, err := getDepsInitCCode(manifest)
	if err != nil {
		return errors.Trace(err)
	}

	if len(depsCCode) != 0 {
		fname := filepath.Join(genDir, depsInitCFileName)

		if err = ioutil.WriteFile(fname, depsCCode, 0666); err != nil {
			return errors.Trace(err)
		}

		// The modification time of autogenerated file should be set to that of
		// the manifest itself, so that make handles dependencies correctly.
		if err := os.Chtimes(fname, mtime, mtime); err != nil {
			return errors.Trace(err)
		}

		appSources = append(appSources, fname)
	}

	ffiSymbols := manifest.FFISymbols
	curConfSchemaFName := ""

	// If config schema is provided in manifest, generate a yaml file suitable
	// for `APP_CONF_SCHEMA`
	if manifest.ConfigSchema != nil && len(manifest.ConfigSchema) > 0 {
		var err error
		curConfSchemaFName = filepath.Join(genDir, confSchemaFileName)

		confSchemaData, err := yaml.Marshal(manifest.ConfigSchema)
		if err != nil {
			return errors.Trace(err)
		}

		if err = ioutil.WriteFile(curConfSchemaFName, confSchemaData, 0666); err != nil {
			return errors.Trace(err)
		}

		// The modification time of conf schema file should be set to that of
		// the manifest itself, so that make handles dependencies correctly.
		if err := os.Chtimes(curConfSchemaFName, mtime, mtime); err != nil {
			return errors.Trace(err)
		}
	}

	// Makefile expects globs, not dir names, so we convert source and filesystem
	// dirs to the appropriate globs. Non-dir items will stay intact.
	appSources, appSourceDirs, err = globify(appSources, []string{"*.c", "*.cpp"})
	if err != nil {
		return errors.Trace(err)
	}

	appFSFiles, appFSDirs, err = globify(appFSFiles, []string{"*"})
	if err != nil {
		return errors.Trace(err)
	}

	reportf("Sources: %v", appSources)

	reportf("Building...")

	appName, err := fixupAppName(manifest.Name)
	if err != nil {
		return errors.Trace(err)
	}

	var errs error
	for k, v := range map[string]string{
		"MGOS_PATH":      dockerMgosPath,
		"PLATFORM":       manifest.Arch,
		"BUILD_DIR":      objsDirDocker,
		"FW_DIR":         fwDirDocker,
		"GEN_DIR":        genDir,
		"FS_STAGING_DIR": path.Join(buildDir, "fs"),
		"APP":            appName,
		"APP_VERSION":    manifest.Version,
		"APP_SOURCES":    strings.Join(appSources, " "),
		"APP_FS_FILES":   strings.Join(appFSFiles, " "),
		"FFI_SYMBOLS":    strings.Join(ffiSymbols, " "),
		"APP_CFLAGS":     generateCflags(manifest.CFlags, manifest.CDefs),
		"APP_CXXFLAGS":   generateCflags(manifest.CXXFlags, manifest.CDefs),
	} {
		err := addBuildVar(manifest, k, v)
		if err != nil {
			errs = multierror.Append(errs, err)
		}
	}
	if errs != nil {
		return errors.Trace(errs)
	}

	// If config schema file was generated, set APP_CONF_SCHEMA appropriately.
	// If not, then check if APP_CONF_SCHEMA was set manually, and warn about
	// that.
	if curConfSchemaFName != "" {
		if err := addBuildVar(manifest, "APP_CONF_SCHEMA", curConfSchemaFName); err != nil {
			return errors.Trace(err)
		}
	} else {
		printConfSchemaWarn(manifest)
	}

	// Add build vars from CLI flags
	for _, v := range buildVarsSlice {
		parts := strings.SplitN(v, ":", 2)
		manifest.BuildVars[parts[0]] = parts[1]
	}

	appPath, err := getCodeDir()
	if err != nil {
		return errors.Trace(err)
	}

	// Invoke actual build (docker or make) {{{
	if os.Getenv("MIOT_SDK_REVISION") == "" {
		// We're outside of the docker container, so invoke docker

		dockerArgs := []string{"run", "--rm", "-i"}

		gitToplevelDir, _ := gitutils.GitGetToplevelDir(appPath)

		appMountPath := ""
		appSubdir := ""
		if gitToplevelDir == "" {
			// We're outside of any git repository: will just mount the application
			// path
			appMountPath = appPath
			appSubdir = ""
		} else {
			// We're inside some git repo: will mount the root of this repo, and
			// remember the app's subdir inside it.
			appMountPath = gitToplevelDir
			appSubdir = appPath[len(gitToplevelDir):]
		}

		// Note about mounts: we mount repo to a stable path (/app) as well as the
		// original path outside the container, whatever it may be, so that absolute
		// path references continue to work (e.g. Git submodules are known to use
		// abs. paths).
		dockerArgs = append(dockerArgs, "-v", fmt.Sprintf("%s:%s", appMountPath, dockerAppPath))
		dockerArgs = append(dockerArgs, "-v", fmt.Sprintf("%s:%s", mosDirEffectiveAbs, dockerMgosPath))
		dockerArgs = append(dockerArgs, "-v", fmt.Sprintf("%s:%s", mosDirEffectiveAbs, mosDirEffectiveAbs))

		// Mount all dirs with source files
		for _, d := range appSourceDirs {
			dockerArgs = append(dockerArgs, "-v", fmt.Sprintf("%s:%s", d, d))
		}

		// Mount all dirs with filesystem files
		for _, d := range appFSDirs {
			dockerArgs = append(dockerArgs, "-v", fmt.Sprintf("%s:%s", d, d))
		}

		// If generated config schema file is present, mount its dir as well
		if curConfSchemaFName != "" {
			d := filepath.Dir(curConfSchemaFName)
			dockerArgs = append(dockerArgs, "-v", fmt.Sprintf("%s:%s", d, d))
		}

		// On Windows and Mac, run container as root since volume sharing on those
		// OSes doesn't play nice with unprivileged user.
		//
		// On other OSes, run it as the current user.
		if runtime.GOOS == "linux" {
			// Unfortunately, user.Current() sometimes panics when the mos binary is
			// built statically, so we have to do the trick with "id -u". Since this
			// code runs on Linux only, this workaround does the trick.
			var data bytes.Buffer
			cmd := exec.Command("id", "-u")
			cmd.Stdout = &data
			if err := cmd.Run(); err != nil {
				return errors.Trace(err)
			}
			sdata := data.String()
			userID := sdata[:len(sdata)-1]

			dockerArgs = append(
				dockerArgs, "--user", fmt.Sprintf("%s:%s", userID, userID),
			)
		}

		// Add extra docker args
		if buildDockerExtra != nil {
			dockerArgs = append(dockerArgs, (*buildDockerExtra)...)
		}

		// Get build image name and tag
		sdkVersionBytes, err := ioutil.ReadFile(
			filepath.Join(mosDirEffective, "fw/platforms", manifest.Arch, "sdk.version"),
		)
		if err != nil {
			return errors.Annotatef(err, "failed to read sdk version file")
		}
		// Drop trailing newline
		sdkVersion := string(sdkVersionBytes[:len(sdkVersionBytes)-1])

		dockerArgs = append(dockerArgs, sdkVersion)

		makeArgs := getMakeArgs(
			fmt.Sprintf("%s%s", dockerAppPath, appSubdir),
			manifest,
		)
		dockerArgs = append(dockerArgs,
			"/bin/bash", "-c", "nice make '"+strings.Join(makeArgs, "' '")+"'",
		)

		if *verbose {
			reportf("Docker arguments: %s", strings.Join(dockerArgs, " "))
		}

		cmd := exec.Command("docker", dockerArgs...)
		err = runCmd(cmd, logFile)
		if err != nil {
			return errors.Trace(err)
		}
	} else {
		// We're already inside of the docker container, so invoke make directly

		manifest.BuildVars["MGOS_PATH"] = mosDirEffectiveAbs

		makeArgs := getMakeArgs(appPath, manifest)

		if *verbose {
			reportf("Make arguments: %s", strings.Join(makeArgs, " "))
		}

		cmd := exec.Command("make", makeArgs...)
		err = runCmd(cmd, logFile)
		if err != nil {
			return errors.Trace(err)
		}
	}
	// }}}

	// Copy firmware to build/fw.zip
	err = ourio.LinkOrCopyFile(
		filepath.Join(fwDir, fmt.Sprintf("%s-%s-last.zip", appName, manifest.Arch)),
		fwFilename,
	)
	if err != nil {
		return errors.Trace(err)
	}

	// Copy ELF file to fw.elf
	err = ourio.LinkOrCopyFile(
		filepath.Join(objsDir, fmt.Sprintf("%s.elf", appName)), elfFilename,
	)
	if err != nil {
		return errors.Trace(err)
	}

	return nil
}

// printConfSchemaWarn checks if APP_CONF_SCHEMA is set in the manifest
// manually, and prints a warning if so.
func printConfSchemaWarn(manifest *build.FWAppManifest) {
	if _, ok := manifest.BuildVars["APP_CONF_SCHEMA"]; ok {
		reportf("===")
		reportf("WARNING: Setting build variable %q in %q "+
			"is deprecated, use \"config_schema\" property instead.",
			"APP_CONF_SCHEMA", build.ManifestFileName,
		)
		reportf("===")
	}
}

func getMakeArgs(dir string, manifest *build.FWAppManifest) []string {
	makeArgs := []string{
		"-j", fmt.Sprintf("%d", runtime.NumCPU()),
		"-C", dir,
		// NOTE that we use path instead of filepath, because it'll run in a docker
		// container, and thus will use Linux path separator
		"-f", path.Join(
			manifest.BuildVars["MGOS_PATH"],
			"fw/platforms",
			manifest.BuildVars["PLATFORM"],
			"Makefile.build",
		),
	}

	for k, v := range manifest.BuildVars {
		makeArgs = append(makeArgs, fmt.Sprintf("%s=%s", k, v))
	}

	// Add extra make args
	if buildCmdExtra != nil {
		makeArgs = append(makeArgs, (*buildCmdExtra)...)
	}

	return makeArgs
}

// globify takes a list of paths, and for each of them which resolves to a
// directory adds each glob from provided globs. Other paths are added as they
// are.
func globify(srcPaths []string, globs []string) (sources []string, dirs []string, err error) {
	for _, p := range srcPaths {
		finfo, err := os.Stat(p)
		var curDir string
		if err == nil && finfo.IsDir() {
			// Item exists and is a directory; add given globs to it
			for _, glob := range globs {
				sources = append(sources, filepath.Join(p, glob))
			}
			curDir = p
		} else {
			if err != nil {
				// Item either does not exist or is a glob
				if !os.IsNotExist(errors.Cause(err)) {
					// Some error other than non-existing file, return an error
					return nil, nil, errors.Trace(err)
				}

				// Try to interpret current item as a glob; if it does not resolve
				// to anything, we'll silently ignore it
				matches, err := filepath.Glob(p)
				if err != nil {
					return nil, nil, errors.Trace(err)
				}

				if len(matches) == 0 {
					// The item did not resolve to anything when interpreted as a glob,
					// assume it does not exist, and silently ignore
					continue
				}
			}

			// Item is an existing file or a glob which resolves to something; just
			// add it as it is
			sources = append(sources, p)
			curDir = filepath.Dir(p)
		}
		d, err := filepath.Abs(curDir)
		if err != nil {
			return nil, nil, errors.Trace(err)
		}
		dirs = append(dirs, d)
	}
	return sources, dirs, nil
}

// addBuildVar adds a given build variable to manifest.BuildVars, but if the
// variable already exists, returns an error (modulo some exceptions, which
// result in a warning instead)
func addBuildVar(manifest *build.FWAppManifest, name, value string) error {
	if _, ok := manifest.BuildVars[name]; ok {
		return errors.Errorf(
			"Build variable %q should not be given in %q "+
				"since it's set by the mos tool automatically",
			name, build.ManifestFileName,
		)
	}
	manifest.BuildVars[name] = value
	return nil
}

// getCmdWriter returns a writer which includes at least the given logFile,
// and if --verbose flag is set, then also stdout.
func getCmdWriter(logFile io.Writer) io.Writer {
	writers := []io.Writer{logFile}
	if *verbose {
		writers = append(writers, os.Stdout)
	}
	return io.MultiWriter(writers...)
}

// runCmd runs given command and redirects its output to the given log file.
// if --verbose flag is set, then the output also goes to the stdout.
func runCmd(cmd *exec.Cmd, logFile io.Writer) error {
	writers := []io.Writer{logFile}
	if *verbose {
		writers = append(writers, os.Stdout)
	}
	out := getCmdWriter(logFile)
	cmd.Stdout = out
	cmd.Stderr = out
	err := cmd.Run()
	if err != nil {
		return errors.Trace(err)
	}
	return nil
}

func getCodeDir() (string, error) {
	absCodeDir, err := filepath.Abs(codeDir)
	if err != nil {
		return "", errors.Trace(err)
	}

	for _, c := range absCodeDir {
		if unicode.IsSpace(c) {
			return "", errors.Errorf("code dir (%q) should not contain spaces", absCodeDir)
		}
	}

	return absCodeDir, nil
}

// readManifest reads manifest file(s) from the specific directory; if the
// manifest or given buildParams have arch specified, then the returned
// manifest will contain all arch-specific adjustments (if any)
func readManifest(appDir string, bParams *buildParams) (*build.FWAppManifest, time.Time, error) {
	manifestFullName := filepath.Join(appDir, build.ManifestFileName)
	manifest, mtime, err := readManifestFile(manifestFullName, true)
	if err != nil {
		return nil, time.Time{}, errors.Trace(err)
	}

	// Override arch with the value given in command line
	if bParams != nil && bParams.Arch != "" {
		manifest.Arch = bParams.Arch
	}
	manifest.Arch = strings.ToLower(manifest.Arch)

	if manifest.Arch != "" {
		manifestArchFullName := filepath.Join(
			appDir,
			fmt.Sprintf(build.ManifestArchFileFmt, manifest.Arch),
		)
		_, err := os.Stat(manifestArchFullName)
		if err == nil {
			// Arch-specific mos.yml does exist, so, handle it
			archManifest, archMtime, err := readManifestFile(manifestArchFullName, false)
			if err != nil {
				return nil, time.Time{}, errors.Trace(err)
			}

			// We should return the latest modification date of all encountered
			// manifests, so let's see if we got the later mtime here
			if archMtime.After(mtime) {
				mtime = archMtime
			}

			// Extend common app manifest with arch-specific things.
			extendManifest(manifest, manifest, archManifest, "", "")
		} else if !os.IsNotExist(err) {
			// Some error other than non-existing mos_<arch>.yml; complain.
			return nil, time.Time{}, errors.Trace(err)
		}
	}

	return manifest, mtime, nil
}

// readManifestFile reads single manifest file (which can be either "main" app
// or lib manifest, or some arch-specific adjustment manifest)
func readManifestFile(
	manifestFullName string, manifestVersionMandatory bool,
) (*build.FWAppManifest, time.Time, error) {
	manifestSrc, err := ioutil.ReadFile(manifestFullName)
	if err != nil {
		return nil, time.Time{}, errors.Trace(err)
	}

	var manifest build.FWAppManifest
	if err := yaml.Unmarshal(manifestSrc, &manifest); err != nil {
		return nil, time.Time{}, errors.Trace(err)
	}

	// If SkeletonVersion is specified, but ManifestVersion is not, then use the
	// former
	if manifest.ManifestVersion == "" && manifest.SkeletonVersion != "" {
		// TODO(dfrank): uncomment the warning below when our examples use
		// manifest_version
		//reportf("WARNING: skeleton_version is deprecated and will be removed eventually, please rename it to manifest_version")
		manifest.ManifestVersion = manifest.SkeletonVersion
	}

	if manifest.ManifestVersion != "" {
		// Check if manifest manifest version is supported by the mos tool
		if manifest.ManifestVersion < minManifestVersion {
			return nil, time.Time{}, errors.Errorf(
				"too old manifest_version %q in %q (oldest supported is %q)",
				manifest.ManifestVersion, manifestFullName, minManifestVersion,
			)
		}

		if manifest.ManifestVersion > maxManifestVersion {
			return nil, time.Time{}, errors.Errorf(
				"too new manifest_version %q in %q (latest supported is %q). Please run \"mos update\".",
				manifest.ManifestVersion, manifestFullName, maxManifestVersion,
			)
		}
	} else if manifestVersionMandatory {
		return nil, time.Time{}, errors.Errorf(
			"manifest version is missing in %q", manifestFullName,
		)
	}

	if manifest.BuildVars == nil {
		manifest.BuildVars = make(map[string]string)
	}

	if manifest.MongooseOsVersion == "" {
		manifest.MongooseOsVersion = "master"
	}

	stat, err := os.Stat(manifestFullName)
	if err != nil {
		return nil, time.Time{}, errors.Trace(err)
	}

	return &manifest, stat.ModTime(), nil
}

func buildRemote(bParams *buildParams) error {
	appDir, err := getCodeDir()
	if err != nil {
		return errors.Trace(err)
	}

	// We'll need to amend the sources significantly with all libs, so copy them
	// to temporary dir first
	tmpCodeDir, err := ioutil.TempDir(tmpDir, "tmp_mos_src_")
	if err != nil {
		return errors.Trace(err)
	}
	if !*keepTempFiles {
		defer os.RemoveAll(tmpCodeDir)
	}

	// Since we're going to copy sources to the temp dir, make sure that nobody
	// else can read them
	if err := os.Chmod(tmpCodeDir, 0700); err != nil {
		return errors.Trace(err)
	}

	if err := ourio.CopyDir(appDir, tmpCodeDir, nil); err != nil {
		return errors.Trace(err)
	}

	// Create directory for libs which are going to be uploaded to the remote builder
	userLibsDir := filepath.Join(tmpCodeDir, localLibsDir)
	err = os.MkdirAll(userLibsDir, 0777)
	if err != nil {
		return errors.Trace(err)
	}

	// Get manifest which includes stuff from all libs
	manifest, _, err := readManifestWithLibs(
		tmpCodeDir, bParams, nil, nil, os.Stdout, userLibsDir, true, /* skip clean */
	)
	if err != nil {
		return errors.Trace(err)
	}

	// Print a warning if APP_CONF_SCHEMA is set in manifest manually
	printConfSchemaWarn(manifest)

	// Amend build vars with the values given in command line
	if len(buildVarsSlice) > 0 {
		if manifest.BuildVars == nil {
			manifest.BuildVars = make(map[string]string)
		}
		for _, v := range buildVarsSlice {
			parts := strings.SplitN(v, ":", 2)
			manifest.BuildVars[parts[0]] = parts[1]
		}
	}

	manifest.Name, err = fixupAppName(manifest.Name)
	if err != nil {
		return errors.Trace(err)
	}

	// Write manifest yaml
	manifestData, err := yaml.Marshal(&manifest)
	if err != nil {
		return errors.Trace(err)
	}

	err = ioutil.WriteFile(
		filepath.Join(tmpCodeDir, build.ManifestFileName),
		manifestData,
		0666,
	)
	if err != nil {
		return errors.Trace(err)
	}

	// Craft file whitelist for zipping
	whitelist := map[string]bool{
		build.ManifestFileName: true,
		localLibsDir:           true,
		".":                    true,
	}
	for _, v := range manifest.Sources {
		whitelist[v] = true
	}
	for _, v := range manifest.Filesystem {
		whitelist[v] = true
	}
	for _, v := range manifest.ExtraFiles {
		whitelist[v] = true
	}

	transformers := make(map[string]fileTransformer)

	// create a zip out of the code dir
	os.Chdir(tmpCodeDir)
	src, err := zipUp(".", whitelist, transformers)
	if err != nil {
		return errors.Trace(err)
	}
	if glog.V(2) {
		glog.V(2).Infof("zip:", hex.Dump(src))
	}
	os.Chdir(appDir)

	// prepare multipart body
	body := &bytes.Buffer{}
	mpw := multipart.NewWriter(body)
	part, err := mpw.CreateFormFile("file", "source.zip")
	if err != nil {
		return errors.Trace(err)
	}

	if _, err := part.Write(src); err != nil {
		return errors.Trace(err)
	}

	if *cleanBuild {
		if err := mpw.WriteField("clean", "1"); err != nil {
			return errors.Trace(err)
		}
	}

	if err := mpw.Close(); err != nil {
		return errors.Trace(err)
	}

	server, err := serverURL()
	if err != nil {
		return errors.Trace(err)
	}

	buildUser := "test"
	buildPass := "test"
	reportf("Connecting to %s, user %s", server, buildUser)

	// invoke the fwbuild API
	uri := fmt.Sprintf("%s/api/%s/firmware/build", server, buildUser)

	reportf("Uploading sources (%d bytes)", len(body.Bytes()))
	req, err := http.NewRequest("POST", uri, body)
	req.Header.Set("Content-Type", mpw.FormDataContentType())
	req.SetBasicAuth(buildUser, buildPass)

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		return errors.Trace(err)
	}

	// handle response
	body.Reset()
	body.ReadFrom(resp.Body)

	switch resp.StatusCode {
	case http.StatusOK, http.StatusTeapot:
		// Build either succeeded or failed

		// unzip build results
		r := bytes.NewReader(body.Bytes())
		os.RemoveAll(buildDir)
		archive.UnzipInto(r, r.Size(), ".", 0)

		// print log in verbose mode or when build fails
		if *verbose || resp.StatusCode != http.StatusOK {
			log, err := os.Open(path.Join(buildDir, buildLog))
			if err != nil {
				return errors.Trace(err)
			}
			io.Copy(os.Stdout, log)
		}

		if resp.StatusCode != http.StatusOK {
			return errors.Errorf("build failed")
		}
		return nil

	default:
		// Unexpected response
		return errors.Errorf("error response: %d: %s", resp.StatusCode, strings.TrimSpace(body.String()))
	}

}

type fileTransformer func(r io.ReadCloser) (io.ReadCloser, error)

// zipUp takes the whitelisted files and directories under path and returns an
// in-memory zip file. The whitelist map is applied to top-level dirs and files
// only. If some file needs to be transformed before placing into a zip
// archive, the appropriate transformer function should be placed at the
// transformers map.
func zipUp(
	dir string,
	whitelist map[string]bool,
	transformers map[string]fileTransformer,
) ([]byte, error) {
	data := &bytes.Buffer{}
	z := zip.NewWriter(data)

	err := filepath.Walk(dir, func(file string, info os.FileInfo, err error) error {
		// Zip files should always contain forward slashes
		fileForwardSlash := file
		if os.PathSeparator != rune('/') {
			fileForwardSlash = strings.Replace(file, string(os.PathSeparator), "/", -1)
		}
		parts := strings.Split(file, string(os.PathSeparator))

		if _, ok := whitelist[parts[0]]; !ok {
			glog.Infof("ignoring %q", file)
			if info.IsDir() {
				return filepath.SkipDir
			} else {
				return nil
			}
		}
		if info.IsDir() {
			return nil
		}

		glog.Infof("zipping %s", file)

		w, err := z.Create(path.Join("src", fileForwardSlash))
		if err != nil {
			return errors.Trace(err)
		}

		var r io.ReadCloser
		r, err = os.Open(file)
		if err != nil {
			return errors.Trace(err)
		}
		defer r.Close()

		t, ok := transformers[fileForwardSlash]
		if !ok {
			t = identityTransformer
		}

		r, err = t(r)
		if err != nil {
			return errors.Trace(err)
		}
		defer r.Close()

		if _, err := io.Copy(w, r); err != nil {
			return errors.Trace(err)
		}

		return nil
	})
	if err != nil {
		return nil, errors.Trace(err)
	}

	z.Close()
	return data.Bytes(), nil
}

func fixupAppName(appName string) (string, error) {
	if appName == "" {
		wd, err := getCodeDir()
		if err != nil {
			return "", errors.Trace(err)
		}
		appName = filepath.Base(wd)
	}

	for _, c := range appName {
		if unicode.IsSpace(c) {
			return "", errors.Errorf("app name (%q) should not contain spaces", appName)
		}
	}

	return appName, nil
}

func identityTransformer(r io.ReadCloser) (io.ReadCloser, error) {
	return r, nil
}

type manifestVars struct {
	subst map[string]string
}

func NewManifestVars() *manifestVars {
	return &manifestVars{
		subst: make(map[string]string),
	}
}

func (mv *manifestVars) SetVar(name, value string) {
	// Note: we opted to use ${foo} instead of {{foo}}, because {{foo}} needs to
	// be quoted in yaml, whereas ${foo} does not.
	glog.Infof("Set '%s'='%s'", name, value)
	mv.subst[fmt.Sprintf("${%s}", name)] = value
}

func (mv *manifestVars) ExpandVars(s string) (string, error) {
	var err error
	result := varRegexp.ReplaceAllStringFunc(s, func(v string) string {
		val, found := mv.subst[v]
		if !found {
			err = errors.Errorf("Unknown var '%s'", v)
		}
		return val
	})
	return result, err
}

func setModuleVars(mVars *manifestVars, moduleName, path string) {
	mVars.SetVar(fmt.Sprintf("%s_path", cleanupModuleName(moduleName)), path)
}

func cleanupModuleName(name string) string {
	ret := ""
	for _, c := range name {
		if !(unicode.IsLetter(c) || unicode.IsDigit(c)) {
			c = '_'
		}
		ret += string(c)
	}
	return ret
}

// readManifestWithLibs reads manifest from the provided dir, "expands" all
// libs (so that the returned manifest does not really contain any libs),
// and also returns the most recent modification time of all encountered
// manifests.
//
// If userLibsDir is different from libsDir, then the libs are initially
// prepared in libsDir anyway, but then copied to userLibsDir, omitting the
// ".git" dir.
//
// If skipClean is true, then clean or non-existing libs will NOT be expanded,
// it's useful when crafting a manifest to send to the remote builder.
func readManifestWithLibs(
	dir string, bParams *buildParams, visitedDirs []string, parentDeps []build.FWAppManifestLibHandled,
	logFile io.Writer, userLibsDir string, skipClean bool,
) (*build.FWAppManifest, time.Time, error) {
	for _, v := range visitedDirs {
		if dir == v {
			return nil, time.Time{}, errors.Errorf("cyclic dependency of the lib %q", dir)
		}
	}

	manifest, mtime, err := readManifest(dir, bParams)
	if err != nil {
		return nil, time.Time{}, errors.Trace(err)
	}

	curDir, err := getCodeDir()
	if err != nil {
		return nil, time.Time{}, errors.Trace(err)
	}

	// Backward compatibility with "Deps", deprecated since 03.06.2017
	for _, v := range manifest.Deps {
		manifest.LibsHandled = append(manifest.LibsHandled, build.FWAppManifestLibHandled{
			Name: v,
		})
	}

	// Prepare all libs {{{
	var cleanLibs []build.SWModule
	var newDeps []build.FWAppManifestLibHandled
libs:
	for _, m := range manifest.Libs {
		name, err := m.GetName()
		if err != nil {
			return nil, time.Time{}, errors.Trace(err)
		}

		reportf("Handling lib %q...", name)

		// Collect all deps: from parent manifest(s), main manifest, and all libs
		// encountered so far
		curDeps := append(parentDeps, append(manifest.LibsHandled, newDeps...)...)

		// Check if this lib is already handled (present in deps)
		// If yes, skip
		for _, v := range curDeps {
			if v.Name == name {
				reportf("Already handled, skipping")
				continue libs
			}
		}

		libDirAbs, ok := bParams.CustomLibLocations[name]

		if !ok {
			reportf("The --lib flag was not given for it, checking repository")

			needPull := true

			if *noLibsUpdate {
				localDir, err := m.GetLocalDir(libsDir)
				if err != nil {
					return nil, time.Time{}, errors.Trace(err)
				}

				if _, err := os.Stat(localDir); err == nil {
					reportf("--no-libs-update was given, and %q exists: skipping update", localDir)
					libDirAbs = localDir
					needPull = false
				}
			}

			if needPull {
				if skipClean {
					isClean, err := m.IsClean(libsDir)
					if err != nil {
						return nil, time.Time{}, errors.Trace(err)
					}

					if isClean {
						reportf("Clean, skipping (will be handled remotely)")
						cleanLibs = append(cleanLibs, m)
						continue
					}
				}

				// Note: we always call PrepareLocalDir for libsDir, but then,
				// if userLibsDir is different, will need to copy it to the new location
				libDirAbs, err = m.PrepareLocalDir(libsDir, os.Stdout, true)
				if err != nil {
					return nil, time.Time{}, errors.Trace(err)
				}
			}
		} else {
			reportf("Using the location %q as is (given as a --lib flag)", libDirAbs)
		}

		libDirForManifest := libDirAbs

		// If libs should be placed in some specific dir, copy the current lib
		// there (it will also affect the libs path used in resulting manifest)
		if userLibsDir != libsDir {
			userLibsDirRel, err := filepath.Rel(dir, userLibsDir)
			if err != nil {
				return nil, time.Time{}, errors.Trace(err)
			}

			userLocalDir := filepath.Join(userLibsDir, filepath.Base(libDirAbs))
			if err := ourio.CopyDir(libDirAbs, userLocalDir, []string{".git"}); err != nil {
				return nil, time.Time{}, errors.Trace(err)
			}
			libDirAbs = filepath.Join(userLibsDir, filepath.Base(libDirAbs))
			libDirForManifest = filepath.Join(userLibsDirRel, filepath.Base(libDirAbs))
		}

		os.Chdir(libDirAbs)

		reportf("Prepared local dir: %q", libDirAbs)

		// We need to create a copy of build params, and if arch is empty there,
		// set it from the outer manifest, because arch is used in libs to handle
		// arch-dependent submanifests, like mos_esp8266.yml.
		bParams2 := *bParams
		if bParams2.Arch == "" {
			bParams2.Arch = manifest.Arch
		}

		libManifest, libMtime, err := readManifestWithLibs(
			libDirAbs, &bParams2, append(visitedDirs, dir), curDeps, logFile, userLibsDir, skipClean,
		)
		if err != nil {
			return nil, time.Time{}, errors.Trace(err)
		}

		// We should return the latest modification date of all encountered
		// manifests, so let's see if we got the later mtime here
		if libMtime.After(mtime) {
			mtime = libMtime
		}

		// Extend app's manifest with that of a lib, and lib's one should go
		// first
		extendManifest(manifest, libManifest, manifest, libDirForManifest, "")

		newDeps = append(newDeps, libManifest.LibsHandled...)
		newDeps = append(newDeps, build.FWAppManifestLibHandled{
			Name: name,
			Path: libDirForManifest,
		})

		os.Chdir(curDir)
	}
	// }}}

	manifest.Libs = cleanLibs

	// Place new deps before the existing ones
	manifest.LibsHandled = append(newDeps, manifest.LibsHandled...)

	return manifest, mtime, nil
}

func getCustomLibLocations() (map[string]string, error) {
	customLibLocations := map[string]string{}
	for _, l := range *libs {
		parts := strings.SplitN(l, ":", 2)

		// Absolutize the given lib path
		var err error
		parts[1], err = filepath.Abs(parts[1])
		if err != nil {
			return nil, errors.Trace(err)
		}

		customLibLocations[parts[0]] = parts[1]
	}
	return customLibLocations, nil
}

type depsInitData struct {
	Deps []string
}

func getDepsInitCCode(manifest *build.FWAppManifest) ([]byte, error) {
	if len(manifest.LibsHandled) == 0 {
		return nil, nil
	}

	tplData := depsInitData{}
	for _, v := range manifest.LibsHandled {
		tplData.Deps = append(tplData.Deps, strings.Replace(v.Name, "-", "_", -1))
	}

	tpl := template.Must(template.New("depsInit").Parse(
		string(MustAsset("data/deps_init.c.tmpl")),
	))

	var c bytes.Buffer
	if err := tpl.Execute(&c, tplData); err != nil {
		return nil, errors.Trace(err)
	}

	return c.Bytes(), nil
}

// extendManifest extends one manifest with another one.
//
// Currently there are two use cases for it:
// - when extending app's manifest with library's manifest;
// - when extending common app's manifest with the arch-specific one.
//
// These cases have different semantics: in the first case, the app's manifest
// should take precedence, but in the second case, the arch-specific manifest
// should take the precedence over that of an app. But NOTE: in both cases,
// it's app's manifest which should get extended.
//
// So, extendManifest takes 3 pointers to manifest:
// - mMain: main manifest which will be extended;
// - m1: lower-precedence manifest (which goes "first", this matters e.g.
//   for config_schema);
// - m2: higher-precedence manifest (which goes "second").
//
// mMain should typically be the same as either m1 or m2.
//
// m2 takes precedence over m1, and can depend on things defined in m1. So
// e.g. when extending app manifest with lib manifest, lib should be m1, app
// should be m2: config schema defined in lib will go before that of an app,
// and if both an app and a lib define the same build variable, app will win.
//
// m1Dir and m2Dir are optional paths for manifests m1 and m2, respectively.
// If the dir is not empty, then it gets prepended to each source and
// filesystem entry (except entries with absolute paths or paths starting with
// a variable)
func extendManifest(mMain, m1, m2 *build.FWAppManifest, m1Dir, m2Dir string) error {

	// Extend sources
	mMain.Sources = append(
		prependPaths(m1.Sources, m1Dir),
		prependPaths(m2.Sources, m2Dir)...,
	)
	// Extend filesystem
	mMain.Filesystem = append(
		prependPaths(m1.Filesystem, m1Dir),
		prependPaths(m2.Filesystem, m2Dir)...,
	)

	// Add modules and libs from lib
	mMain.Modules = append(m1.Modules, m2.Modules...)
	mMain.Libs = append(m1.Libs, m2.Libs...)
	mMain.ConfigSchema = append(m1.ConfigSchema, m2.ConfigSchema...)
	mMain.CFlags = append(m1.CFlags, m2.CFlags...)
	mMain.CXXFlags = append(m1.CXXFlags, m2.CXXFlags...)

	mMain.BuildVars = mergeMapsString(m1.BuildVars, m2.BuildVars)
	mMain.CDefs = mergeMapsString(m1.CDefs, m2.CDefs)

	return nil
}

func prependPaths(items []string, dir string) []string {
	ret := []string{}
	for _, s := range items {
		// If the path is not absolute, and does not start with the variable,
		// prepend it with the library's path
		if dir != "" && s[0] != '$' && s[0] != '@' && !filepath.IsAbs(s) {
			s = filepath.Join(dir, s)
		}
		ret = append(ret, s)
	}
	return ret
}

func generateCflags(cflags []string, cdefs map[string]string) string {
	for k, v := range cdefs {
		cflags = append(cflags, fmt.Sprintf("-D%s=%s", k, v))
	}

	return strings.Join(append(cflags), " ")
}

// mergeMapsString merges two map[string]string into a new one; m2 takes
// precedence over m1
func mergeMapsString(m1, m2 map[string]string) map[string]string {
	bv := make(map[string]string)

	for k, v := range m1 {
		bv[k] = v
	}
	for k, v := range m2 {
		bv[k] = v
	}

	return bv
}

// expandManifestAllLibsPaths expands "@all_libs" for manifest's Sources
// and Filesystem paths
func expandManifestAllLibsPaths(manifest *build.FWAppManifest) error {
	var err error

	manifest.Sources, err = expandAllLibsPaths(manifest.Sources, manifest.LibsHandled)
	if err != nil {
		return errors.Trace(err)
	}

	manifest.Filesystem, err = expandAllLibsPaths(manifest.Filesystem, manifest.LibsHandled)
	if err != nil {
		return errors.Trace(err)
	}

	return nil
}

// expandAllLibsPaths expands "@all_libs" for the given paths slice, and
// returns a new slice
func expandAllLibsPaths(
	paths []string, libsHandled []build.FWAppManifestLibHandled,
) ([]string, error) {
	ret := []string{}

	for _, p := range paths {
		if strings.HasPrefix(p, allLibsKeyword) {
			innerPath := p[len(allLibsKeyword):]
			for _, lh := range libsHandled {
				ret = append(ret, filepath.Join(lh.Path, innerPath))
			}
		} else {
			ret = append(ret, p)
		}
	}

	return ret, nil
}
