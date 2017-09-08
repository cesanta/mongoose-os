package main

import (
	"archive/zip"
	"bytes"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"mime/multipart"
	"net/http"
	"os"
	"os/exec"
	"os/signal"
	"path"
	"path/filepath"
	"regexp"
	"runtime"
	"strings"
	"syscall"
	"text/template"
	"time"
	"unicode"

	"golang.org/x/net/context"

	"cesanta.com/common/go/multierror"
	"cesanta.com/common/go/ourfilepath"
	"cesanta.com/common/go/ourio"
	"cesanta.com/mos/build"
	"cesanta.com/mos/build/archive"
	"cesanta.com/mos/build/gitutils"
	moscommon "cesanta.com/mos/common"
	"cesanta.com/mos/common/paths"
	"cesanta.com/mos/dev"
	"cesanta.com/mos/flash/common"
	"cesanta.com/mos/interpreter"
	"cesanta.com/mos/update"
	"cesanta.com/mos/version"
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
	cleanBuild         = flag.Bool("clean", false, "perform a clean build, wipe the previous build state")
	buildTarget        = flag.String("build-target", moscommon.BuildTargetDefault, "target to build with make")
	keepTempFiles      = flag.Bool("keep-temp-files", false, "keep temp files after the build is done (by default they are in ~/.mos/tmp)")
	modules            = flag.StringSlice("module", []string{}, "location of the module from mos.yaml, in the format: \"module_name:/path/to/location\". Can be used multiple times.")
	libs               = flag.StringSlice("lib", []string{}, "location of the lib from mos.yaml, in the format: \"lib_name:/path/to/location\". Can be used multiple times.")
	libsUpdateInterval = flag.Duration("libs-update-interval", time.Minute*30, "how often to update already fetched libs")

	buildDockerExtra = flag.StringSlice("build-docker-extra", []string{}, "extra docker flags, added before image name. Can be used multiple times: e.g. --build-docker-extra -v --build-docker-extra /foo:/bar.")
	buildCmdExtra    = flag.StringSlice("build-cmd-extra", []string{}, "extra make flags, added at the end of the make command. Can be used multiple times.")
	buildParalellism = flag.Int("build-parallelism", 0, "build parallelism. default is to use number of CPUs.")
	saveBuildStat    = flag.Bool("save-build-stat", true, "save build statistics")

	buildVarsSlice []string

	noLibsUpdate = flag.Bool("no-libs-update", false, "if true, never try to pull existing libs (treat existing default locations as if they were given in --lib)")

	// In-memory buffer containing all the log messages
	logBuf bytes.Buffer

	// Log writer which always writes to the build.log file, os.Stderr and logBuf
	logWriterStderr io.Writer

	// The same as logWriterStderr, but skips os.Stderr unless --verbose is given
	logWriter io.Writer

	// Note: we opted to use ${foo} instead of {{foo}}, because {{foo}} needs to
	// be quoted in yaml, whereas ${foo} does not.
	varRegexp = regexp.MustCompile(`\$\{[^}]+\}`)
)

const (
	projectDir = "."

	// Manifest version changes:
	//
	// - 2017-06-03: added support for @all_libs in filesystem and sources
	// - 2017-06-16: added support for conds with very basic expressions
	//               (only build_vars)
	minManifestVersion = "2017-03-17"
	maxManifestVersion = "2017-06-16"

	localLibsDir = "local_libs"

	allLibsKeyword = "@all_libs"

	depsApp = "app"

	assetPrefix           = "asset://"
	rootManifestAssetName = "data/root_manifest.yml"
)

func init() {
	hiddenFlags = append(hiddenFlags, "docker_images")

	flag.StringSliceVar(&buildVarsSlice, "build-var", []string{}, "build variable in the format \"NAME:VALUE\" Can be used multiple times.")
}

type buildParams struct {
	Platform           string
	BuildTarget        string
	CustomLibLocations map[string]string
}

func buildHandler(ctx context.Context, devConn *dev.DevConn) error {
	cll, err := getCustomLibLocations()
	if err != nil {
		return errors.Trace(err)
	}

	bParams := buildParams{
		Platform:           *platform,
		CustomLibLocations: cll,
		BuildTarget:        *buildTarget,
	}

	return errors.Trace(doBuild(ctx, &bParams))
}

func doBuild(ctx context.Context, bParams *buildParams) error {
	var err error
	buildDir := moscommon.GetBuildDir(projectDir)

	start := time.Now()

	// Request server version in parallel
	serverVersionCh := make(chan *version.VersionJson, 1)
	if !*local {
		go func() {
			v, err := update.GetServerMosVersion(update.GetUpdateChannel())
			if err != nil {
				// Ignore error, it's not really important
				return
			}
			serverVersionCh <- v
		}()
	}

	if err := os.MkdirAll(buildDir, 0777); err != nil {
		return errors.Trace(err)
	}

	blog := moscommon.GetBuildLogFilePath(buildDir)
	logFile, err := os.Create(blog)
	if err != nil {
		return errors.Trace(err)
	}

	// Remove local log, ignore any errors
	os.RemoveAll(moscommon.GetBuildLogLocalFilePath(buildDir))

	logWriterStderr = io.MultiWriter(logFile, &logBuf, os.Stderr)
	logWriter = io.MultiWriter(logFile, &logBuf)

	if *verbose {
		logWriter = logWriterStderr
	}

	// Fail fast if there is no manifest
	if _, err := os.Stat(moscommon.GetManifestFilePath(projectDir)); os.IsNotExist(err) {
		return errors.Errorf("No mos.yml file")
	}

	if *local {
		err = buildLocal(ctx, bParams)
	} else {
		err = buildRemote(bParams)
	}
	if err != nil {
		return errors.Trace(err)
	}

	if *buildTarget == moscommon.BuildTargetDefault {
		// We were building a firmware, so perform the required actions with moving
		// firmware around, etc.
		fwFilename := moscommon.GetFirmwareZipFilePath(buildDir)

		fw, err := common.NewZipFirmwareBundle(fwFilename, "")
		if err != nil {
			return errors.Trace(err)
		}

		end := time.Now()

		if *saveBuildStat {
			bstat := moscommon.BuildStat{
				ArchOld:     fw.Platform,
				Platform:    fw.Platform,
				AppName:     fw.Name,
				BuildTimeMS: int(end.Sub(start) / time.Millisecond),
			}

			data, err := json.MarshalIndent(&bstat, "", "  ")
			if err != nil {
				return errors.Trace(err)
			}

			ioutil.WriteFile(moscommon.GetBuildStatFilePath(buildDir), data, 0666)
		}

		if *local || !*verbose {
			if err == nil {
				freportf(logWriter, "Success, built %s/%s version %s (%s).", fw.Name, fw.Platform, fw.Version, fw.BuildID)
			}

			freportf(logWriterStderr, "Firmware saved to %s", fwFilename)
		}
	} else {
		// We were building some custom target, so just report that we succeeded.
		freportf(logWriterStderr, "Target %s is built successfully", *buildTarget)
	}

	// If received server version, compare it with the local one and notify the
	// user about the update (if available)
	select {
	case v := <-serverVersionCh:
		serverVer := version.GetMosVersionFromBuildId(v.BuildId)
		localVer := version.GetMosVersion()

		if serverVer != localVer {
			freportf(logWriterStderr, "By the way, there is a newer version available: %q (you use %q). Run \"mos update\" to upgrade.", serverVer, localVer)
		}
	default:
	}

	return err
}

func buildLocal(ctx context.Context, bParams *buildParams) (err error) {
	if isInDockerToolbox() {
		freportf(logWriterStderr, "Docker Toolbox detected")
	}

	buildDir := moscommon.GetBuildDir(projectDir)

	defer func() {
		if !*verbose && err != nil {
			log, err := os.Open(moscommon.GetBuildLogFilePath(buildDir))
			if err != nil {
				glog.Errorf("can't read build log: %s", err)
				return
			}
			io.Copy(os.Stdout, log)
		}
	}()

	dockerAppPath := "/app"
	dockerMgosPath := "/mongoose-os"

	buildDirAbs, err := filepath.Abs(buildDir)
	if err != nil {
		return errors.Trace(err)
	}

	genDir := moscommon.GetGeneratedFilesDir(buildDirAbs)

	fwDir := moscommon.GetFirmwareDir(buildDir)
	fwDirDocker := getPathForDocker(fwDir)

	objsDir := moscommon.GetObjectDir(buildDir)
	objsDirDocker := getPathForDocker(objsDir)

	fwFilename := moscommon.GetFirmwareZipFilePath(buildDir)

	elfFilename := moscommon.GetFirmwareElfFilePath(buildDir)

	// Perform cleanup before the build {{{
	if *cleanBuild {
		// Cleanup build dir, but leave build log intact, because we're already
		// writing to it.
		if err := ourio.RemoveFromDir(buildDir, []string{moscommon.GetBuildLogFilePath("")}); err != nil {
			return errors.Trace(err)
		}
	} else {
		// This is not going to be a clean build, but we should still remove fw.zip
		// (ignoring any possible errors)
		os.Remove(fwFilename)
	}
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

	interp := interpreter.NewInterpreter(newMosVars())

	appDir, err := getCodeDirAbs()
	if err != nil {
		return errors.Trace(err)
	}

	manifest, mtime, err := readManifestWithLibs(
		appDir, bParams, logWriter, paths.LibsDir, interp,
		true /* require arch */, false /* skip clean */, true, /* finalize */
	)
	if err != nil {
		return errors.Trace(err)
	}

	switch manifest.Type {
	case build.AppTypeApp:
		// Fine
	case build.AppTypeLib:
		return errors.Errorf("can't build a library; only apps can be built. Libraries can be only used as dependencies for apps or for other libs")
	default:
		return errors.Errorf("invalid project type: %q", manifest.Type)
	}

	// Write final manifest to build dir
	{
		d, err := yaml.Marshal(manifest)
		if err != nil {
			return errors.Trace(err)
		}

		if err := ourio.WriteFileIfDiffers(moscommon.GetMosFinalFilePath(buildDirAbs), d, 0666); err != nil {
			return errors.Trace(err)
		}
	}

	// Check if the app supports the given arch
	if len(manifest.Platforms) > 0 {
		found := false
		for _, v := range manifest.Platforms {
			if v == manifest.Platform {
				found = true
				break
			}
		}

		if !found {
			return errors.Errorf("can't build for the platform %s; only those platforms are supported: %v", manifest.Platform, manifest.Platforms)
		}
	}

	if err := interpreter.SetManifestVars(interp.MVars, manifest); err != nil {
		return errors.Trace(err)
	}

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
			freportf(logWriter, "The flag --module is not given for the module %q, going to use the repository", name)

			var err error
			targetDir, err = m.PrepareLocalDir(paths.ModulesDir, logWriter, true, manifest.ModulesVersion, *libsUpdateInterval)
			if err != nil {
				return errors.Annotatef(err, "preparing local copy of the module %q", name)
			}
		} else {
			freportf(logWriter, "Using module %q located at %q", name, targetDir)
		}

		interpreter.SetModuleVars(interp.MVars, name, targetDir)
	}
	// }}}

	// Determine mongoose-os dir (mosDirEffective) {{{
	mosDirEffective, err := getMosDirEffective(manifest.MongooseOsVersion, *libsUpdateInterval)
	if err != nil {
		return errors.Trace(err)
	}

	interpreter.SetModuleVars(interp.MVars, "mongoose-os", mosDirEffective)

	mosDirEffectiveAbs, err := filepath.Abs(mosDirEffective)
	if err != nil {
		return errors.Annotatef(err, "getting absolute path of %q", mosDirEffective)
	}
	// }}}

	// Get sources and filesystem files from the manifest, expanding expressions {{{
	appSources := []string{}
	for _, s := range manifest.Sources {
		s, err = expandVars(interp, s)
		if err != nil {
			return errors.Trace(err)
		}
		appSources = append(appSources, s)
	}

	appFSFiles := []string{}
	for _, s := range manifest.Filesystem {
		s, err = expandVars(interp, s)
		if err != nil {
			return errors.Trace(err)
		}
		appFSFiles = append(appFSFiles, s)
	}

	appBinLibs := []string{}
	for _, s := range manifest.BinaryLibs {
		s, err = expandVars(interp, s)
		if err != nil {
			return errors.Trace(err)
		}
		appBinLibs = append(appBinLibs, s)
	}
	// }}}

	appSourceDirs := []string{}
	appFSDirs := []string{}
	appBinLibsDirs := []string{}

	// Generate deps_init C code, and if it's not empty, write it to the temp
	// file and add to sources
	depsCCode, err := getDepsInitCCode(manifest)
	if err != nil {
		return errors.Trace(err)
	}

	if len(depsCCode) != 0 {
		fname := moscommon.GetDepsInitCFilePath(buildDirAbs)

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
		curConfSchemaFName = moscommon.GetConfSchemaFilePath(buildDirAbs)

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

	appBinLibs, appBinLibsDirs, err = globify(appBinLibs, []string{"*.a"})
	if err != nil {
		return errors.Trace(err)
	}

	freportf(logWriter, "Sources: %v", appSources)
	if len(appBinLibs) > 0 {
		freportf(logWriter, "Binary libs: %v", appBinLibs)
	}

	freportf(logWriter, "Building...")

	appName, err := fixupAppName(manifest.Name)
	if err != nil {
		return errors.Trace(err)
	}

	var errs error
	for k, v := range map[string]string{
		"MGOS_PATH":      dockerMgosPath,
		"PLATFORM":       manifest.Platform,
		"BUILD_DIR":      objsDirDocker,
		"FW_DIR":         fwDirDocker,
		"GEN_DIR":        getPathForDocker(genDir),
		"FS_STAGING_DIR": getPathForDocker(moscommon.GetFilesystemStagingDir(buildDir)),
		"APP":            appName,
		"APP_VERSION":    manifest.Version,
		"APP_SOURCES":    strings.Join(getPathsForDocker(appSources), " "),
		"APP_FS_FILES":   strings.Join(getPathsForDocker(appFSFiles), " "),
		"APP_BIN_LIBS":   strings.Join(getPathsForDocker(appBinLibs), " "),
		"FFI_SYMBOLS":    strings.Join(ffiSymbols, " "),
		"APP_CFLAGS":     generateCflags(manifest.CFlags, manifest.CDefs),
		"APP_CXXFLAGS":   generateCflags(manifest.CXXFlags, manifest.CDefs),
		"MANIFEST_FINAL": getPathForDocker(moscommon.GetMosFinalFilePath(buildDirAbs)),
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
		if err := addBuildVar(manifest, "APP_CONF_SCHEMA", getPathForDocker(curConfSchemaFName)); err != nil {
			return errors.Trace(err)
		}
	} else {
		printConfSchemaWarn(manifest)
	}

	// Add build vars from CLI flags
	for _, v := range buildVarsSlice {
		pp1 := strings.SplitN(v, ":", 2)
		pp2 := strings.SplitN(v, "=", 2)
		var pp []string
		switch {
		case len(pp1) == 2 && len(pp2) == 1:
			pp = pp1
		case len(pp1) == 1 && len(pp2) == 2:
			pp = pp2
		case len(pp1) == 2 && len(pp2) == 2:
			if len(pp1[0]) < len(pp2[0]) {
				pp = pp1
			} else {
				pp = pp2
			}
		default:
			return errors.Errorf("invalid --build-var spec: %q", v)
		}
		manifest.BuildVars[pp[0]] = pp[1]
	}

	appPath, err := getCodeDirAbs()
	if err != nil {
		return errors.Trace(err)
	}

	// Invoke actual build (docker or make) {{{
	if os.Getenv("MGOS_SDK_REVISION") == "" && os.Getenv("MIOT_SDK_REVISION") == "" {
		// We're outside of the docker container, so invoke docker

		dockerRunArgs := []string{"--rm", "-i"}

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

		// Generate mountpoint args {{{
		mp := mountPoints{}

		// Note about mounts: we mount repo to a stable path (/app) as well as the
		// original path outside the container, whatever it may be, so that absolute
		// path references continue to work (e.g. Git submodules are known to use
		// abs. paths).
		mp.addMountPoint(appMountPath, dockerAppPath)
		mp.addMountPoint(mosDirEffectiveAbs, dockerMgosPath)
		mp.addMountPoint(mosDirEffectiveAbs, getPathForDocker(mosDirEffectiveAbs))

		// Mount build dir
		mp.addMountPoint(buildDirAbs, getPathForDocker(buildDirAbs))

		// Mount all dirs with source files
		for _, d := range appSourceDirs {
			mp.addMountPoint(d, getPathForDocker(d))
		}

		// Mount all dirs with filesystem files
		for _, d := range appFSDirs {
			mp.addMountPoint(d, getPathForDocker(d))
		}

		// Mount all dirs with binary libs
		for _, d := range appBinLibsDirs {
			mp.addMountPoint(d, getPathForDocker(d))
		}

		// If generated config schema file is present, mount its dir as well
		if curConfSchemaFName != "" {
			d := filepath.Dir(curConfSchemaFName)
			mp.addMountPoint(d, getPathForDocker(d))
		}

		for containerPath, hostPath := range mp {
			dockerRunArgs = append(dockerRunArgs, "-v", fmt.Sprintf("%s:%s", hostPath, containerPath))
		}
		// }}}

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

			dockerRunArgs = append(
				dockerRunArgs, "--user", fmt.Sprintf("%s:%s", userID, userID),
			)
		}

		// Add extra docker args
		if buildDockerExtra != nil {
			dockerRunArgs = append(dockerRunArgs, (*buildDockerExtra)...)
		}

		sdkVersionFile := filepath.Join(mosDirEffective, "fw/platforms", manifest.Platform, "sdk.version")

		// Get build image name and tag
		sdkVersionBytes, err := ioutil.ReadFile(sdkVersionFile)
		if err != nil {
			return errors.Annotatef(err, "failed to read sdk version file %q", sdkVersionFile)
		}

		sdkVersion := strings.TrimSpace(string(sdkVersionBytes))
		dockerRunArgs = append(dockerRunArgs, sdkVersion)

		makeArgs, err := getMakeArgs(
			fmt.Sprintf("%s%s", dockerAppPath, appSubdir),
			bParams.BuildTarget,
			manifest,
		)
		if err != nil {
			return errors.Trace(err)
		}
		dockerRunArgs = append(dockerRunArgs,
			"/bin/bash", "-c", "nice make '"+strings.Join(makeArgs, "' '")+"'",
		)

		if err := runDockerBuild(dockerRunArgs); err != nil {
			return errors.Trace(err)
		}
	} else {
		// We're already inside of the docker container, so invoke make directly

		manifest.BuildVars["MGOS_PATH"] = mosDirEffectiveAbs

		makeArgs, err := getMakeArgs(appPath, bParams.BuildTarget, manifest)
		if err != nil {
			return errors.Trace(err)
		}

		freportf(logWriter, "Make arguments: %s", strings.Join(makeArgs, " "))

		cmd := exec.Command("make", makeArgs...)
		err = runCmd(cmd, logWriter)
		if err != nil {
			return errors.Trace(err)
		}
	}
	// }}}

	if *buildTarget == moscommon.BuildTargetDefault {
		// We were building a firmware, so perform the required actions with moving
		// firmware around, etc.

		// Copy firmware to build/fw.zip
		err = ourio.LinkOrCopyFile(
			filepath.Join(fwDir, fmt.Sprintf("%s-%s-last.zip", appName, manifest.Platform)),
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
	}

	return nil
}

func runDockerBuild(dockerRunArgs []string) error {
	containerName := fmt.Sprint("mos_build_", time.Now().Format("2006-01-02T15-04-05-00"))

	dockerArgs := append(
		[]string{"run", "--name", containerName}, dockerRunArgs...,
	)

	freportf(logWriter, "Docker arguments: %s", strings.Join(dockerArgs, " "))

	// When make runs with -j and we interrupt the container with Ctrl+C, make
	// becomes a runaway process eating 100% of one CPU core. So far we failed
	// to fix it properly, so the workaround is to kill the container on the
	// reception of SIGINT or SIGTERM.
	signals := []os.Signal{syscall.SIGINT, syscall.SIGTERM}

	sigCh := make(chan os.Signal, 1)

	// Signal handler goroutine: on SIGINT and SIGTERM it will kill the container
	// and exit(1). When the sigCh is closed, goroutine returns.
	go func() {
		if _, ok := <-sigCh; !ok {
			return
		}

		freportf(logWriterStderr, "\nCleaning up the container %q...", containerName)
		cmd := exec.Command("docker", "kill", containerName)
		cmd.Run()

		os.Exit(1)
	}()

	signal.Notify(sigCh, signals...)
	defer func() {
		// Unsubscribe from the signals and close the channel so that the signal
		// handler goroutine is properly cleaned up
		signal.Reset(signals...)
		close(sigCh)
	}()

	cmd := exec.Command("docker", dockerArgs...)
	if err := runCmd(cmd, logWriter); err != nil {
		return errors.Trace(err)
	}

	return nil
}

// printConfSchemaWarn checks if APP_CONF_SCHEMA is set in the manifest
// manually, and prints a warning if so.
func printConfSchemaWarn(manifest *build.FWAppManifest) {
	if _, ok := manifest.BuildVars["APP_CONF_SCHEMA"]; ok {
		freportf(logWriterStderr, "===")
		freportf(logWriterStderr, "WARNING: Setting build variable %q in %q "+
			"is deprecated, use \"config_schema\" property instead.",
			"APP_CONF_SCHEMA", moscommon.GetManifestFilePath(""),
		)
		freportf(logWriterStderr, "===")
	}
}

func getMakeArgs(dir, target string, manifest *build.FWAppManifest) ([]string, error) {
	j := *buildParalellism
	if j == 0 {
		j = runtime.NumCPU()
	}

	// If target contains a slash, assume it's a path name, and absolutize it
	// (that's a requirement because in makefile paths are absolutized).
	// Actually, all file targets are going to begin with "build/", so this check
	// is reliable.
	if strings.Contains(target, "/") {
		var err error
		target, err = filepath.Abs(target)
		if err != nil {
			return nil, errors.Trace(err)
		}
	}

	makeArgs := []string{
		"-j", fmt.Sprintf("%d", j),
		"-C", dir,
		// NOTE that we use path instead of filepath, because it'll run in a docker
		// container, and thus will use Linux path separator
		"-f", path.Join(
			manifest.BuildVars["MGOS_PATH"],
			"fw/platforms",
			manifest.BuildVars["PLATFORM"],
			"Makefile.build",
		),
		target,
	}

	for k, v := range manifest.BuildVars {
		makeArgs = append(makeArgs, fmt.Sprintf("%s=%s", k, v))
	}

	// Add extra make args
	if buildCmdExtra != nil {
		makeArgs = append(makeArgs, (*buildCmdExtra)...)
	}

	return makeArgs, nil
}

// globify takes a list of paths, and for each of them which resolves to a
// directory adds each glob from provided globs. Other paths are added as they
// are.
func globify(srcPaths []string, globs []string) (sources []string, dirs []string, err error) {
	cwd, err := filepath.Abs(".")
	if err != nil {
		return nil, nil, errors.Trace(err)
	}

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

	// We want source paths to be absolute, but sources are globs, so we can't do
	// filepath.Abs on it. Instead, we can just do filepath.Join(cwd, s) if
	// the path is not absolute.
	for k, s := range sources {
		if !filepath.IsAbs(s) {
			sources[k] = filepath.Join(cwd, s)
		}
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
			name, moscommon.GetManifestFilePath(""),
		)
	}
	manifest.BuildVars[name] = value
	return nil
}

// runCmd runs given command and redirects its output to the given log file.
// if --verbose flag is set, then the output also goes to the stdout.
func runCmd(cmd *exec.Cmd, logWriter io.Writer) error {
	cmd.Stdout = logWriter
	cmd.Stderr = logWriter
	err := cmd.Run()
	if err != nil {
		return errors.Trace(err)
	}
	return nil
}

func getCodeDirAbs() (string, error) {
	absCodeDir, err := filepath.Abs(projectDir)
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
func readManifest(
	appDir string, bParams *buildParams, interp *interpreter.MosInterpreter,
) (*build.FWAppManifest, time.Time, error) {
	// Read root manifest from the asset
	rootManifest, _, err := readManifestFile(
		fmt.Sprint(assetPrefix, rootManifestAssetName), interp, true,
	)
	if err != nil {
		return nil, time.Time{}, errors.Trace(err)
	}

	manifestFullName := moscommon.GetManifestFilePath(appDir)
	manifest, mtime, err := readManifestFile(manifestFullName, interp, true)
	if err != nil {
		return nil, time.Time{}, errors.Trace(err)
	}

	// Override arch with the value given in command line
	if bParams != nil && bParams.Platform != "" {
		manifest.Platform = bParams.Platform
	}
	manifest.Platform = strings.ToLower(manifest.Platform)

	// Set the mos.platform variable
	interp.MVars.SetVar(interpreter.GetMVarNameMosPlatform(), manifest.Platform)

	// If type is omitted, assume "app"
	if manifest.Type == "" {
		manifest.Type = build.AppTypeApp
	}

	// We need everything under root manifest's conds to be already available,
	// so expand all conds there. It means that the conds in root manifest
	// should only depend on the stuff already defined (basically, only "mos.platform").
	//
	// TODO(dfrank): probably make it so that if conds expression fails to
	// evaluate, keep it unexpanded for now.
	if err := expandManifestLibsAndConds(rootManifest, interp); err != nil {
		return nil, time.Time{}, errors.Trace(err)
	}

	// Extend app manifest with the root manifest
	if err := extendManifest(manifest, rootManifest, manifest, "", "", interp); err != nil {
		return nil, time.Time{}, errors.Trace(err)
	}

	if manifest.Platform != "" {
		manifestArchFullName := moscommon.GetManifestArchFilePath(appDir, manifest.Platform)
		_, err := os.Stat(manifestArchFullName)
		if err == nil {
			// Arch-specific mos.yml does exist, so, handle it
			archManifest, archMtime, err := readManifestFile(manifestArchFullName, interp, false)
			if err != nil {
				return nil, time.Time{}, errors.Trace(err)
			}

			// We should return the latest modification date of all encountered
			// manifests, so let's see if we got the later mtime here
			if archMtime.After(mtime) {
				mtime = archMtime
			}

			// Extend common app manifest with arch-specific things.
			if err := extendManifest(manifest, manifest, archManifest, "", "", interp); err != nil {
				return nil, time.Time{}, errors.Trace(err)
			}
		} else if !os.IsNotExist(err) {
			// Some error other than non-existing mos_<arch>.yml; complain.
			return nil, time.Time{}, errors.Trace(err)
		}
	}

	if manifest.Platforms == nil {
		manifest.Platforms = []string{}
	}

	return manifest, mtime, nil
}

// readManifestFile reads single manifest file (which can be either "main" app
// or lib manifest, or some arch-specific adjustment manifest)
func readManifestFile(
	manifestFullName string, interp *interpreter.MosInterpreter, manifestVersionMandatory bool,
) (*build.FWAppManifest, time.Time, error) {
	var manifestSrc []byte
	var err error

	if !strings.HasPrefix(manifestFullName, assetPrefix) {
		// Reading regular file from the host filesystem
		manifestSrc, err = ioutil.ReadFile(manifestFullName)
	} else {
		// Reading the asset
		assetName := manifestFullName[len(assetPrefix):]
		manifestSrc, err = Asset(assetName)
	}
	if err != nil {
		return nil, time.Time{}, errors.Annotatef(err, "reading manifest %q", manifestFullName)
	}

	var manifest build.FWAppManifest
	if err := yaml.Unmarshal(manifestSrc, &manifest); err != nil {
		return nil, time.Time{}, errors.Annotatef(err, "parsing manifest %q", manifestFullName)
	}

	// If SkeletonVersion is specified, but ManifestVersion is not, then use the
	// former
	if manifest.ManifestVersion == "" && manifest.SkeletonVersion != "" {
		// TODO(dfrank): uncomment the warning below when our examples use
		// manifest_version
		//freportf(logWriterStderr, "WARNING: skeleton_version is deprecated and will be removed eventually, please rename it to manifest_version")
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

	// Normalize all Libs and Modules
	for i, _ := range manifest.Libs {
		manifest.Libs[i].Normalize()
	}

	for i, _ := range manifest.Modules {
		manifest.Modules[i].Normalize()
	}

	if manifest.BuildVars == nil {
		manifest.BuildVars = make(map[string]string)
	}

	if manifest.CDefs == nil {
		manifest.CDefs = make(map[string]string)
	}

	if manifest.MongooseOsVersion == "" {
		manifest.MongooseOsVersion = "master"
	}

	if manifest.Platform == "" && manifest.ArchOld != "" {
		manifest.Platform = manifest.ArchOld
	}

	// Convert init_after to weak deps
	for _, v := range manifest.InitAfter {
		manifest.Libs = append(manifest.Libs, build.SWModule{
			Name: v,
			Weak: true,
		})
	}
	manifest.InitAfter = nil

	manifest.MongooseOsVersion, err = expandVars(interp, manifest.MongooseOsVersion)
	if err != nil {
		return nil, time.Time{}, errors.Trace(err)
	}

	manifest.LibsVersion, err = expandVars(interp, manifest.LibsVersion)
	if err != nil {
		return nil, time.Time{}, errors.Trace(err)
	}

	manifest.ModulesVersion, err = expandVars(interp, manifest.ModulesVersion)
	if err != nil {
		return nil, time.Time{}, errors.Trace(err)
	}

	var modTime time.Time

	if !strings.HasPrefix(manifestFullName, assetPrefix) {
		stat, err := os.Stat(manifestFullName)
		if err != nil {
			return nil, time.Time{}, errors.Trace(err)
		}

		modTime = stat.ModTime()
	}

	return &manifest, modTime, nil
}

func buildRemote(bParams *buildParams) error {
	appDir, err := getCodeDirAbs()
	if err != nil {
		return errors.Trace(err)
	}

	buildDir := moscommon.GetBuildDir(projectDir)

	// We'll need to amend the sources significantly with all libs, so copy them
	// to temporary dir first
	tmpCodeDir, err := ioutil.TempDir(paths.TmpDir, "tmp_mos_src_")
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

	interp := interpreter.NewInterpreter(newMosVars())

	// Get manifest which includes stuff from all libs
	manifest, _, err := readManifestWithLibs(
		tmpCodeDir, bParams, logWriter, userLibsDir, interp,
		true /* require arch */, true /* skip clean */, false, /* finalize */
	)
	if err != nil {
		return errors.Trace(err)
	}

	// Copy all external code (which is outside of the appDir) under tmpCodeDir {{{
	if err := copyExternalCodeAll(&manifest.Sources, appDir, tmpCodeDir); err != nil {
		return errors.Trace(err)
	}

	if err := copyExternalCodeAll(&manifest.Filesystem, appDir, tmpCodeDir); err != nil {
		return errors.Trace(err)
	}

	if err := copyExternalCodeAll(&manifest.BinaryLibs, appDir, tmpCodeDir); err != nil {
		return errors.Trace(err)
	}
	// }}}

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

	// For all handled libs, fixup paths if local separator is different from
	// the Linux separator (because remote builder runs on linux)
	if filepath.Separator != '/' {
		for k, lh := range manifest.LibsHandled {
			manifest.LibsHandled[k].Path = strings.Replace(
				lh.Path, string(filepath.Separator), "/", -1,
			)
		}
	}

	// Write manifest yaml
	manifestData, err := yaml.Marshal(&manifest)
	if err != nil {
		return errors.Trace(err)
	}

	err = ioutil.WriteFile(
		moscommon.GetManifestFilePath(tmpCodeDir),
		manifestData,
		0666,
	)
	if err != nil {
		return errors.Trace(err)
	}

	// Craft file whitelist for zipping
	whitelist := map[string]bool{
		moscommon.GetManifestFilePath(""): true,
		localLibsDir:                      true,
		".":                               true,
	}
	for _, v := range manifest.Sources {
		whitelist[ourfilepath.GetFirstPathComponent(v)] = true
	}
	for _, v := range manifest.Filesystem {
		whitelist[ourfilepath.GetFirstPathComponent(v)] = true
	}
	for _, v := range manifest.BinaryLibs {
		whitelist[ourfilepath.GetFirstPathComponent(v)] = true
	}
	for _, v := range manifest.ExtraFiles {
		whitelist[ourfilepath.GetFirstPathComponent(v)] = true
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
	part, err := mpw.CreateFormFile(moscommon.FormSourcesZipName, "source.zip")
	if err != nil {
		return errors.Trace(err)
	}

	if _, err := part.Write(src); err != nil {
		return errors.Trace(err)
	}

	if *cleanBuild {
		if err := mpw.WriteField(moscommon.FormCleanName, "1"); err != nil {
			return errors.Trace(err)
		}
	}

	if err := mpw.WriteField(moscommon.FormBuildTargetName, bParams.BuildTarget); err != nil {
		return errors.Trace(err)
	}

	if data, err := ioutil.ReadFile(moscommon.GetBuildCtxFilePath(buildDir)); err == nil {
		// Successfully read build context name, transmit it to the remote builder
		if err := mpw.WriteField(moscommon.FormBuildCtxName, string(data)); err != nil {
			return errors.Trace(err)
		}
	}

	if data, err := ioutil.ReadFile(moscommon.GetBuildStatFilePath(buildDir)); err == nil {
		// Successfully read build stat, transmit it to the remote builder
		if err := mpw.WriteField(moscommon.FormBuildStatName, string(data)); err != nil {
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
	freportf(logWriterStderr, "Connecting to %s, user %s", server, buildUser)

	// invoke the fwbuild API (replace "master" with "latest")
	fwbuildVersion := version.GetMosVersion()

	if fwbuildVersion == "master" {
		fwbuildVersion = "latest"
	}

	uri := fmt.Sprintf("%s/api/fwbuild/%s/build", server, fwbuildVersion)

	freportf(logWriterStderr, "Uploading sources (%d bytes)", len(body.Bytes()))
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

		// Save local log
		ioutil.WriteFile(moscommon.GetBuildLogLocalFilePath(buildDir), logBuf.Bytes(), 0666)

		// print log in verbose mode or when build fails
		if *verbose || resp.StatusCode != http.StatusOK {
			log, err := os.Open(moscommon.GetBuildLogFilePath(buildDir))
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
		wd, err := getCodeDirAbs()
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
	dir string, bParams *buildParams,
	logWriter io.Writer, userLibsDir string, interp *interpreter.MosInterpreter,
	requireArch, skipClean, finalize bool,
) (*build.FWAppManifest, time.Time, error) {
	libsHandled := map[string]build.FWAppManifestLibHandled{}

	// Create a deps structure and add a root dep: an "app"
	deps := NewDeps()
	deps.AddNode(depsApp)

	manifest, mtime, err := readManifestWithLibs2(manifestParseContext{
		dir:        dir,
		rootAppDir: dir,

		bParams:     bParams,
		logWriter:   logWriter,
		userLibsDir: userLibsDir,
		skipClean:   skipClean,

		nodeName:    depsApp,
		deps:        deps,
		libsHandled: libsHandled,

		appManifest: nil,
		interp:      interp,

		requireArch: requireArch,
	})
	if err != nil {
		return nil, time.Time{}, errors.Trace(err)
	}

	// Get all deps in topological order
	topo, cycle := deps.Topological(true)
	if cycle != nil {
		return nil, time.Time{}, errors.Errorf(
			"dependency cycle: %v", strings.Join(cycle, " -> "),
		)
	}

	// Remove the last item from topo, which is depsApp
	//
	// TODO(dfrank): it would be nice to handle an app just another dependency
	// and generate init code for it, but it would be a breaking change, at least
	// because all libs init functions return bool, but mgos_app_init returns
	// enum mgos_app_init_result.
	topo = topo[0 : len(topo)-1]

	// Create a LibsHandled slice in topological order computed above
	manifest.LibsHandled = make([]build.FWAppManifestLibHandled, 0, len(topo))
	for _, v := range topo {
		lh, ok := libsHandled[v]
		if !ok {
			// topo contains v, but libsHandled doesn't: it happens when we skip
			// clean libs, it just means that the current lib v is not prepared,
			// thus we don't add it to manifest.LibsHandled.
			continue
		}

		// Move all sublibs to the app's manifest libs. It might happen when
		// we prepare the manifest to build remotely: some lib is not fetchable
		// from the Internet, so we handle it, but it might have other libs,
		// so they should be taken care of.
		for _, subLib := range lh.Manifest.Libs {
			manifest.Libs = append(manifest.Libs, subLib)
		}
		lh.Manifest.Libs = nil

		manifest.LibsHandled = append(manifest.LibsHandled, lh)
	}

	if finalize {
		if err := expandManifestLibsAndConds(manifest, interp); err != nil {
			return nil, time.Time{}, errors.Trace(err)
		}

		if err := expandManifestAllLibsPaths(manifest); err != nil {
			return nil, time.Time{}, errors.Trace(err)
		}
	}

	return manifest, mtime, nil
}

type manifestParseContext struct {
	// Manifest's directory
	dir string
	// Directory of the "root" app; for the app's manifest it's the same as dir
	rootAppDir string

	bParams     *buildParams
	logWriter   io.Writer
	userLibsDir string
	skipClean   bool

	nodeName    string
	deps        *Deps
	libsHandled map[string]build.FWAppManifestLibHandled

	appManifest *build.FWAppManifest
	interp      *interpreter.MosInterpreter

	requireArch bool
}

func readManifestWithLibs2(pc manifestParseContext) (*build.FWAppManifest, time.Time, error) {
	manifest, mtime, err := readManifest(pc.dir, pc.bParams, pc.interp)
	if err != nil {
		return nil, time.Time{}, errors.Trace(err)
	}

	if pc.requireArch && manifest.Platform == "" {
		return nil, time.Time{}, errors.Errorf("--platform must be specified or mos.yml should contain a platform key")
	}

	// If the given appManifest is nil, it means that we've just read one, so
	// remember it as such
	if pc.appManifest == nil {
		pc.appManifest = manifest
	}

	curDir, err := getCodeDirAbs()
	if err != nil {
		return nil, time.Time{}, errors.Trace(err)
	}

	// Backward compatibility with "Deps", deprecated since 03.06.2017
	for _, v := range manifest.Deps {
		manifest.LibsHandled = append(manifest.LibsHandled, build.FWAppManifestLibHandled{
			Name: v,
		})
	}

	// Take LibsHandled from manifest into account, add each lib to the current
	// pc.libsHandled map, and to deps.
	for _, lh := range manifest.LibsHandled {
		pc.libsHandled[lh.Name] = lh

		pc.deps.AddDep(pc.nodeName, lh.Name)
		pc.deps.AddNode(lh.Name)
		for _, dep := range lh.Deps {
			pc.deps.AddDep(lh.Name, dep)
		}
	}

	// Prepare all libs {{{
libs:
	for _, m := range manifest.Libs {
		name, err := m.GetName()
		if err != nil {
			return nil, time.Time{}, errors.Trace(err)
		}

		freportf(pc.logWriter, "Handling lib %q...", name)

		pc.deps.AddDep(pc.nodeName, name)

		if pc.deps.NodeExists(name) {
			freportf(pc.logWriter, "Already handled, skipping")
			continue libs
		}

		if m.Weak {
			freportf(pc.logWriter, "Optional, skipping")
			continue
		}

		libDirAbs, ok := pc.bParams.CustomLibLocations[name]

		if !ok {
			freportf(pc.logWriter, "The --lib flag was not given for it, checking repository")

			needPull := true

			if *noLibsUpdate {
				localDir, err := m.GetLocalDir(paths.LibsDir, pc.appManifest.LibsVersion)
				if err != nil {
					return nil, time.Time{}, errors.Trace(err)
				}

				if _, err := os.Stat(localDir); err == nil {
					freportf(pc.logWriter, "--no-libs-update was given, and %q exists: skipping update", localDir)
					libDirAbs = localDir
					needPull = false
				}
			}

			if needPull {
				// Note: we always call PrepareLocalDir for libsDir, but then,
				// if pc.userLibsDir is different, will need to copy it to the new location
				libDirAbs, err = m.PrepareLocalDir(paths.LibsDir, pc.logWriter, true, pc.appManifest.LibsVersion, *libsUpdateInterval)
				if err != nil {
					return nil, time.Time{}, errors.Annotatef(err, "preparing local copy of the lib %q", name)
				}

			}
		} else {
			freportf(pc.logWriter, "Using the location %q as is (given as a --lib flag)", libDirAbs)
		}

		freportf(pc.logWriter, "Prepared local dir: %q", libDirAbs)

		libDirForManifest := libDirAbs

		skip := false
		if pc.skipClean {
			isClean, err := m.IsClean(paths.LibsDir, pc.appManifest.LibsVersion)
			if err != nil {
				return nil, time.Time{}, errors.Trace(err)
			}

			skip = isClean
		}

		// If libs should be placed in some specific dir, copy the current lib
		// there (it will also affect the libs path used in resulting manifest)
		if !skip && pc.userLibsDir != paths.LibsDir {
			userLibsDirRel, err := filepath.Rel(pc.rootAppDir, pc.userLibsDir)
			if err != nil {
				return nil, time.Time{}, errors.Trace(err)
			}

			userLocalDir := filepath.Join(pc.userLibsDir, filepath.Base(libDirAbs))
			if err := ourio.CopyDir(libDirAbs, userLocalDir, []string{".git"}); err != nil {
				return nil, time.Time{}, errors.Trace(err)
			}
			libDirAbs = filepath.Join(pc.userLibsDir, filepath.Base(libDirAbs))
			libDirForManifest = filepath.Join(userLibsDirRel, filepath.Base(libDirAbs))
		}

		// Now that we know we need to handle current lib, add a node for it
		pc.deps.AddNode(name)

		os.Chdir(libDirAbs)

		// We need to create a copy of build params, and if arch is empty there,
		// set it from the outer manifest, because arch is used in libs to handle
		// arch-dependent submanifests, like mos_esp8266.yml.
		bParams2 := *pc.bParams
		if bParams2.Platform == "" {
			bParams2.Platform = manifest.Platform
		}

		pc2 := pc

		pc2.dir = libDirAbs
		pc2.bParams = &bParams2
		pc2.nodeName = name

		libManifest, libMtime, err := readManifestWithLibs2(pc2)
		if err != nil {
			return nil, time.Time{}, errors.Trace(err)
		}

		// We should return the latest modification date of all encountered
		// manifests, so let's see if we got the later mtime here
		if libMtime.After(mtime) {
			mtime = libMtime
		}

		// Add a build var and C macro MGOS_HAVE_<lib_name>
		haveName := fmt.Sprintf(
			"MGOS_HAVE_%s", strings.ToUpper(moscommon.IdentifierFromString(name)),
		)
		manifest.BuildVars[haveName] = "1"
		manifest.CDefs[haveName] = "1"

		if !skip {
			pc.libsHandled[name] = build.FWAppManifestLibHandled{
				Name:     name,
				Path:     libDirForManifest,
				Deps:     pc.deps.GetDeps(name),
				Manifest: libManifest,
			}
		}

		os.Chdir(curDir)
	}
	// }}}

	// Remove handled libs from manifest.Libs {{{
	// NOTE that this would be a bad idea to keep track of unhandled libs as we
	// go, and just assign manifest.Libs = cleanLibs here, because expansion of
	// some libs might result in new libs being added, and we should keep them.
	newLibs := []build.SWModule{}
	for _, l := range manifest.Libs {
		name, err := l.GetName()
		if err != nil {
			return nil, time.Time{}, errors.Trace(err)
		}

		if _, ok := pc.libsHandled[name]; !ok {
			if !l.Weak {
				newLibs = append(newLibs, l)
			}
		}
	}
	manifest.Libs = newLibs
	// }}}

	return manifest, mtime, nil
}

// expandManifestLibsAndConds takes a manifest and expands all LibsHandled
// and Conds inside all manifests (app and all libs). Since expanded
// conds should be applied in topological order, the process is a bit
// involved:
//
// 1. Create copy of the app manifest: commonManifest
// 2. Expand all libs into that commonManifest
// 3. If resulting manifest has no conds, we're done. Otherwise:
//   a. For each of the manifests (app and all libs), expand conds, but
//      evaluate cond expressions against the commonManifest
//   b. Go to step 1
func expandManifestLibsAndConds(
	manifest *build.FWAppManifest, interp *interpreter.MosInterpreter,
) error {

	for {
		// First, we build a chain of all manifests we have:
		//
		// - Dummy empty manifest (needed so that extendManifest() will be called
		//   with the actual first manifest as "m2", and thus will expand
		//   expressions in its BuildVars and CDefs)
		// - All libs (if any), starting from the one without any deps
		// - App
		allManifests := []build.FWAppManifestLibHandled{}
		allManifests = append(allManifests, build.FWAppManifestLibHandled{
			Name:     "dummy_empty_manifest",
			Path:     "",
			Manifest: &build.FWAppManifest{},
		})
		allManifests = append(allManifests, manifest.LibsHandled...)
		allManifests = append(allManifests, build.FWAppManifestLibHandled{
			Name:     "app",
			Path:     "",
			Manifest: manifest,
		})

		// Set commonManifest to the first manifest in the deps chain, which is
		// a dummy empty manifest.
		commonManifest := allManifests[0].Manifest

		// Iterate all the rest of the manifests, at every step extending the
		// current one with all previous manifests accumulated so far, and the
		// current one takes precedence.
		for k := 1; k < len(allManifests); k++ {
			lcur := allManifests[k]

			curManifest := *lcur.Manifest

			if err := extendManifest(
				&curManifest, commonManifest, &curManifest, "", lcur.Path, interp,
			); err != nil {
				return errors.Trace(err)
			}

			commonManifest = &curManifest
		}

		// Now, commonManifest contains app's manifest with all libs expanded.

		if len(commonManifest.Conds) == 0 {
			// No more conds in the common manifest, so cleanup all libs manifests,
			// and return commonManifest

			for k, _ := range commonManifest.LibsHandled {
				commonManifest.LibsHandled[k].Manifest = nil
			}
			*manifest = *commonManifest

			return nil
		}

		// There are some conds to be expanded. We can't expand them directly in
		// the common manifest, because items should be inserted in topological
		// order. Instead, we'll expand conds separately in the source app
		// manifest, and in each lib's manifests, but we'll execute the cond
		// expressions against the common manifest which we've just computed above,
		// so it already has everything properly overridden.
		//
		// When it's done, we'll expand all libs manifests again, etc, until there
		// are no conds left.

		if err := expandManifestConds(manifest, commonManifest, interp); err != nil {
			return errors.Trace(err)
		}

		for k := range manifest.LibsHandled {
			if manifest.LibsHandled[k].Manifest != nil {
				if err := expandManifestConds(
					manifest.LibsHandled[k].Manifest, commonManifest, interp,
				); err != nil {
					return errors.Trace(err)
				}
			}
		}
	}
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

type libsInitDataItem struct {
	Name string
	Deps []string
}

type libsInitData struct {
	Libs []libsInitDataItem
}

func getDepsInitCCode(manifest *build.FWAppManifest) ([]byte, error) {
	if len(manifest.LibsHandled) == 0 {
		return nil, nil
	}

	tplData := libsInitData{}
	for _, v := range manifest.LibsHandled {
		tplData.Libs = append(tplData.Libs, libsInitDataItem{
			Name: moscommon.IdentifierFromString(v.Name),
			Deps: v.Deps,
		})
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
func extendManifest(
	mMain, m1, m2 *build.FWAppManifest, m1Dir, m2Dir string,
	interp *interpreter.MosInterpreter,
) error {
	var err error

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
	// Extend binary libs
	mMain.BinaryLibs = append(
		prependPaths(m1.BinaryLibs, m1Dir),
		prependPaths(m2.BinaryLibs, m2Dir)...,
	)

	// Add modules and libs from lib
	mMain.Modules = append(m1.Modules, m2.Modules...)
	mMain.Libs = append(m1.Libs, m2.Libs...)
	mMain.ConfigSchema = append(m1.ConfigSchema, m2.ConfigSchema...)
	mMain.CFlags = append(m1.CFlags, m2.CFlags...)
	mMain.CXXFlags = append(m1.CXXFlags, m2.CXXFlags...)

	// m2.BuildVars and m2.CDefs can contain expressions which should be expanded
	// against manifest m1.
	if err := interpreter.SetManifestVars(interp.MVars, m1); err != nil {
		return errors.Trace(err)
	}

	mMain.BuildVars, err = mergeMapsString(m1.BuildVars, m2.BuildVars, interp)
	if err != nil {
		return errors.Trace(err)
	}

	mMain.CDefs, err = mergeMapsString(m1.CDefs, m2.CDefs, interp)
	if err != nil {
		return errors.Trace(err)
	}

	mMain.Platforms = mergeSupportedPlatforms(m1.Platforms, m2.Platforms)

	// Extend conds
	mMain.Conds = append(
		prependCondPaths(m1.Conds, m1Dir),
		prependCondPaths(m2.Conds, m2Dir)...,
	)

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

// prependCondPaths takes a slice of "conds", and for each of them which
// contains an "apply" clause (effectively, a submanifest), prepends paths of
// sources and filesystem with the given dir.
func prependCondPaths(conds []build.ManifestCond, dir string) []build.ManifestCond {
	ret := []build.ManifestCond{}
	for _, c := range conds {
		if c.Apply != nil {
			subManifest := *c.Apply
			subManifest.Sources = prependPaths(subManifest.Sources, dir)
			subManifest.Filesystem = prependPaths(subManifest.Filesystem, dir)
			subManifest.BinaryLibs = prependPaths(subManifest.BinaryLibs, dir)
			c.Apply = &subManifest
		}
		ret = append(ret, c)
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
// precedence over m1. Values of m2 can contain expressions which are expanded
// against the given interp.
func mergeMapsString(
	m1, m2 map[string]string, interp *interpreter.MosInterpreter,
) (map[string]string, error) {
	bv := make(map[string]string)

	for k, v := range m1 {
		bv[k] = v
	}
	for k, v := range m2 {
		var err error
		bv[k], err = expandVars(interp, v)
		if err != nil {
			return nil, errors.Trace(err)
		}
	}

	return bv, nil
}

// mergeSupportedPlatforms returns a slice of all strings which are contained
// in both p1 and p2, or if one of slices is empty, returns another one.
func mergeSupportedPlatforms(p1, p2 []string) []string {
	if len(p1) == 0 {
		return p2
	} else if len(p2) == 0 {
		return p1
	} else {
		m := map[string]struct{}{}
		for _, v := range p1 {
			m[v] = struct{}{}
		}

		ret := []string{}

		for _, v := range p2 {
			if _, ok := m[v]; ok {
				ret = append(ret, v)
			}
		}

		return ret
	}
}

// expandManifestConds expands all "conds" in the dstManifest, but all cond
// expressions are evaluated against the refManifest. Nested conds are
// not expanded: if there are some new conds left, a new refManifest should
// be computed by the caller, and expandManifestConds should be called again
// for each lib's manifest and for app's manifest.
func expandManifestConds(
	dstManifest, refManifest *build.FWAppManifest, interp *interpreter.MosInterpreter,
) error {

	// As we're expanding conds, we need to remove the conds themselves. But
	// extending manifest could cause new conds to be added, so we just save
	// current conds from the manifest in a separate variable, and clean the
	// manifest's conds. This way, newly added conds (if any) won't mess
	// with the old ones.
	conds := dstManifest.Conds
	dstManifest.Conds = nil

	if err := interpreter.SetManifestVars(interp.MVars, refManifest); err != nil {
		return errors.Trace(err)
	}

	for _, cond := range conds {
		res, err := interp.EvaluateExprBool(cond.When)
		if err != nil {
			return errors.Trace(err)
		}

		if !res {
			// The condition is false, skip handling
			continue
		}

		// If error is not an empty string, it means misconfiguration of
		// the current app, so, return an error
		if cond.Error != "" {
			return errors.New(cond.Error)
		}

		// Apply submanifest if present
		if cond.Apply != nil {
			if err := extendManifest(dstManifest, dstManifest, cond.Apply, "", "", interp); err != nil {
				return errors.Trace(err)
			}
		}
	}

	return nil
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

	manifest.BinaryLibs, err = expandAllLibsPaths(manifest.BinaryLibs, manifest.LibsHandled)
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

type mountPoints map[string]string

// addMountPoint adds a mount point from given hostPath to containerPath. If
// something is already mounted to the given containerPath, then it's compared
// to the new hostPath value; if they are not equal, an error is returned.
func (mp mountPoints) addMountPoint(hostPath, containerPath string) error {
	// Docker Toolbox hack: in docker toolbox on windows, the actual host paths
	// like C:\foo\bar don't work, this path becomes /c/foo/bar.
	if isInDockerToolbox() {
		hostPath = getPathForDocker(hostPath)
	}

	freportf(logWriter, "mount from %q to %q\n", hostPath, containerPath)
	if v, ok := mp[containerPath]; ok {
		if hostPath != v {
			return errors.Errorf("adding mount point from %q to %q, but it already mounted from %q", hostPath, containerPath, v)
		}
		// Mount point already exists and is right
		return nil
	}
	mp[containerPath] = hostPath

	return nil
}

// getPathForDocker replaces OS-dependent separators in a given path with "/"
func getPathForDocker(p string) string {
	ret := path.Join(strings.Split(p, string(filepath.Separator))...)
	if filepath.IsAbs(p) {
		if runtime.GOOS == "windows" && ret[1] == ':' {
			// Remove the colon after drive letter, also lowercase the drive letter
			// (the lowercasing part is important for docker toolbox: there, host
			// paths like C:\foo\bar don't work, this path becomse /c/foo/bar)
			ret = fmt.Sprint(strings.ToLower(ret[:1]), ret[2:])
		}
		ret = path.Join("/", ret)
	}
	return ret
}

// getPathsForDocker calls getPathForDocker for each paths in the slice,
// and returns modified slice
func getPathsForDocker(p []string) []string {
	ret := make([]string, len(p))
	for i, v := range p {
		ret[i] = getPathForDocker(v)
	}
	return ret
}

// copyExternalCode checks whether given path p is outside of appDir, and if
// so, copies its contents as a new directory under tmpCodeDir, and returns
// its name. If nothing was copied, returns an empty string.
func copyExternalCode(p, appDir, tmpCodeDir string) (string, error) {
	// Ensure we have relative path curPathRel which should start with ".." if
	// it's outside of the tmpCodeDir
	curPathAbs := p
	if !filepath.IsAbs(curPathAbs) {
		curPathAbs = filepath.Join(appDir, curPathAbs)
	}

	curPathAbs = filepath.Clean(curPathAbs)

	curPathRel, err := filepath.Rel(appDir, curPathAbs)
	if err != nil {
		return "", errors.Trace(err)
	}

	if len(curPathRel) > 0 && curPathRel[0] == '.' {
		// The path is outside of tmpCodeDir, so we should copy its contents
		// under tmpCodeDir

		// The path could end with a glob, so we need to get existing and
		// non-existing parts of the path
		//
		// TODO(dfrank): we should actually handle all the globs here in mos,
		// not in makefile.
		actualPart := curPathAbs
		globPart := ""

		if _, err := os.Stat(actualPart); err != nil {
			actualPart, globPart = filepath.Split(actualPart)
		}

		// Create a new directory named as a "blueprint" of the source directory:
		// full path with all separators replaced with "_".
		curTmpPathRel := strings.Replace(actualPart, string(filepath.Separator), "_", -1)

		curTmpPathAbs := filepath.Join(tmpCodeDir, curTmpPathRel)
		if err := os.MkdirAll(curTmpPathAbs, 0755); err != nil {
			return "", errors.Trace(err)
		}

		// Copy source files to that new dir
		// TODO(dfrank): ensure we don't copy too much
		freportf(logWriter, "Copying %q as %q", actualPart, curTmpPathAbs)
		err = ourio.CopyDir(actualPart, curTmpPathAbs, nil)
		if err != nil {
			return "", errors.Trace(err)
		}

		return filepath.Join(curTmpPathRel, globPart), nil
	}

	return "", nil
}

// copyExternalCodeAll calls copyExternalCode for each element of the paths
// slice, and for each affected path updates the item in the slice.
func copyExternalCodeAll(paths *[]string, appDir, tmpCodeDir string) error {
	for i, curPath := range *paths {
		newPath, err := copyExternalCode(curPath, appDir, tmpCodeDir)
		if err != nil {
			return errors.Trace(err)
		}

		if newPath != "" {
			(*paths)[i] = newPath
		}
	}

	return nil
}

func expandVars(interp *interpreter.MosInterpreter, s string) (string, error) {
	var errRet error
	result := varRegexp.ReplaceAllStringFunc(s, func(v string) string {
		expr := v[2 : len(v)-1]
		val, err := interp.EvaluateExprString(expr)
		if err != nil {
			errRet = errors.Trace(err)
		}
		return val
	})
	return result, errRet
}

func newMosVars() *interpreter.MosVars {
	ret := interpreter.NewMosVars()
	ret.SetVar(interpreter.GetMVarNameMosVersion(), version.GetMosVersion())
	return ret
}

func isInDockerToolbox() bool {
	return os.Getenv("DOCKER_HOST") != ""
}

func getMosDirEffective(mongooseOsVersion string, updateInterval time.Duration) (string, error) {
	var mosDirEffective string
	if *mosRepo != "" {
		freportf(logWriter, "Using mongoose-os located at %q", *mosRepo)
		mosDirEffective = *mosRepo
	} else {
		freportf(logWriter, "The flag --repo is not given, going to use mongoose-os repository")

		m := build.SWModule{
			Type: "git",
			// TODO(dfrank) get upstream repo URL from a flag
			// (and this flag needs to be forwarded to fwbuild as well, which should
			// forward it to the mos invocation)
			Location: "https://github.com/cesanta/mongoose-os",
			Version:  mongooseOsVersion,
		}

		var err error
		mosDirEffective, err = m.PrepareLocalDir(paths.ModulesDir, logWriter, true, "", updateInterval)
		if err != nil {
			return "", errors.Annotatef(err, "preparing local copy of the mongoose-os repo")
		}
	}

	return mosDirEffective, nil
}

func getMosRepoDir(ctx context.Context, devConn *dev.DevConn) error {
	logWriterStderr = io.MultiWriter(&logBuf, os.Stderr)
	logWriter = io.MultiWriter(&logBuf)
	if *verbose {
		logWriter = logWriterStderr
	}

	cll, err := getCustomLibLocations()
	if err != nil {
		return errors.Trace(err)
	}

	bParams := &buildParams{
		Platform:           *platform,
		CustomLibLocations: cll,
	}

	appDir, err := getCodeDirAbs()
	if err != nil {
		return errors.Trace(err)
	}

	interp := interpreter.NewInterpreter(newMosVars())

	manifest, _, err := readManifest(appDir, bParams, interp)
	if err != nil {
		return errors.Trace(err)
	}

	mosDirEffective, err := getMosDirEffective(manifest.MongooseOsVersion, time.Hour*99999)
	if err != nil {
		return errors.Trace(err)
	}

	mosDirEffectiveAbs, err := filepath.Abs(mosDirEffective)
	if err != nil {
		return errors.Annotatef(err, "getting absolute path of %q", mosDirEffective)
	}

	fmt.Println(mosDirEffectiveAbs)
	return nil
}
