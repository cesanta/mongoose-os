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
	"runtime"
	"strings"
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
	cleanBuild = flag.Bool("clean", false, "perform a clean build, wipe the previous build state")
	modules    = flag.StringSlice("module", []string{}, "location of the module from mos.yaml, in the format: \"module_name:/path/to/location\". Can be used multiple times.")

	buildDockerExtra = flag.StringSlice("build-docker-extra", []string{}, "extra docker flags, added before image name. Can be used multiple times: e.g. --build-docker-extra -v --build-docker-extra /foo:/bar.")
	buildCmdExtra    = flag.StringSlice("build-cmd-extra", []string{}, "extra make flags, added at the end of the make command. Can be used multiple times.")

	buildVarsSlice []string

	libsDir    = ""
	modulesDir = ""
)

const (
	buildDir = "build"
	codeDir  = "."

	buildLog = "build.log"

	minSkeletonVersion = "2017-03-17"
	maxSkeletonVersion = "2017-05-16"

	localLibsDir = "local_libs"
)

func init() {
	hiddenFlags = append(hiddenFlags, "docker_images")

	flag.StringSliceVar(&buildVarsSlice, "build-var", []string{}, "build variable in the format \"NAME:VALUE\" Can be used multiple times.")

	// deprecated since 2017/05/11
	flag.StringSliceVar(&buildVarsSlice, "build_var", []string{}, "deprecated, use --build-var")

	flag.StringVar(&libsDir, "libs-dir", "~/.mos/libs", "Directory to store libraries into")
	flag.StringVar(&modulesDir, "modules-dir", "~/.mos/modules", "Directory to store modules into")

	// Unfortunately user.Current() doesn't play nicely with static build, so
	// we have to get home directory from the environment

	homeEnvName := "HOME"
	if runtime.GOOS == "windows" {
		homeEnvName = "USERPROFILE"
	}

	homeDir := os.Getenv(homeEnvName)
	// Replace tilda with the actual path to home directory
	if libsDir[0] == '~' {
		libsDir = homeDir + libsDir[1:]
	}
	if modulesDir[0] == '~' {
		modulesDir = homeDir + modulesDir[1:]
	}
}

func doBuild(ctx context.Context, devConn *dev.DevConn) error {
	var err error
	if *local {
		err = buildLocal(ctx)
	} else {
		err = buildRemote()
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

func buildLocal(ctx context.Context) (err error) {
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

	manifest, err := readManifestWithLibs(appDir, nil, logFile, libsDir, false /* skip clean */)
	if err != nil {
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

	archEffective, err := detectArch(manifest)
	if err != nil {
		return errors.Trace(err)
	}
	mVars.SetVar("arch", archEffective)

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
		appSources = append(appSources, mVars.ExpandVars(s))
	}

	appFSFiles := []string{}
	for _, s := range manifest.Filesystem {
		appFSFiles = append(appFSFiles, mVars.ExpandVars(s))
	}
	// }}}

	appSourceDirs := []string{}
	appFSDirs := []string{}

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

	ffiSymbols := manifest.FFISymbols

	reportf("Sources: %v", appSources)

	reportf("Building...")

	defer os.RemoveAll(fwDir)

	appName, err := fixupAppName(manifest.Name)
	if err != nil {
		return errors.Trace(err)
	}

	var errs error
	for k, v := range map[string]string{
		"MGOS_PATH":      dockerMgosPath,
		"PLATFORM":       archEffective,
		"BUILD_DIR":      objsDirDocker,
		"FW_DIR":         fwDirDocker,
		"GEN_DIR":        path.Join(buildDir, "gen"),
		"FS_STAGING_DIR": path.Join(buildDir, "fs"),
		"APP":            appName,
		"APP_VERSION":    manifest.Version,
		"APP_SOURCES":    strings.Join(appSources, " "),
		"APP_FS_FILES":   strings.Join(appFSFiles, " "),
		"FFI_SYMBOLS":    strings.Join(ffiSymbols, " "),
	} {
		err := addBuildVar(manifest, k, v)
		if err != nil {
			errs = multierror.Append(errs, err)
		}
	}
	if errs != nil {
		return errors.Trace(errs)
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

		for _, d := range appSourceDirs {
			dockerArgs = append(dockerArgs, "-v", fmt.Sprintf("%s:%s", d, d))
		}

		for _, d := range appFSDirs {
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
		sdkVersionBytes, err := ioutil.ReadFile(filepath.Join(mosDirEffective, "fw/platforms", archEffective, "sdk.version"))
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

	// Move firmware as build/fw.zip
	err = os.Rename(
		filepath.Join(fwDir, fmt.Sprintf("%s-%s-last.zip", appName, archEffective)),
		fwFilename,
	)
	if err != nil {
		return errors.Trace(err)
	}

	// Move elf as fw.elf
	err = os.Rename(
		filepath.Join(objsDir, fmt.Sprintf("%s.elf", appName)), elfFilename,
	)
	if err != nil {
		return errors.Trace(err)
	}

	return nil
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
			for _, glob := range globs {
				sources = append(sources, filepath.Join(p, glob))
			}
			curDir = p
		} else {
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

// addBuildVar adds a given build variable to manifest.BuildVars,
// but if the variable already exists, returns an error.
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

func detectArch(manifest *build.FWAppManifest) (string, error) {
	a := *arch
	if a == "" {
		a = manifest.Arch
	}

	if a == "" {
		return "", errors.Errorf("--arch must be specified or mos.yml should contain an arch key")
	}
	return strings.ToLower(a), nil
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

func readManifest(appDir string) (*build.FWAppManifest, error) {
	manifestFullName := filepath.Join(appDir, build.ManifestFileName)
	manifestSrc, err := ioutil.ReadFile(manifestFullName)
	if err != nil {
		return nil, errors.Trace(err)
	}

	var manifest build.FWAppManifest
	if err := yaml.Unmarshal(manifestSrc, &manifest); err != nil {
		return nil, errors.Trace(err)
	}

	// Check if manifest skeleton version is supported by the mos tool
	if manifest.SkeletonVersion < minSkeletonVersion {
		return nil, errors.Errorf(
			"too old skeleton_version %q in %q (oldest supported is %q)",
			manifest.SkeletonVersion, manifestFullName, minSkeletonVersion,
		)
	}

	if manifest.SkeletonVersion > maxSkeletonVersion {
		return nil, errors.Errorf(
			"too new skeleton_version %q in %q (latest supported is %q)",
			manifest.SkeletonVersion, manifestFullName, maxSkeletonVersion,
		)
	}

	if manifest.BuildVars == nil {
		manifest.BuildVars = make(map[string]string)
	}

	if manifest.MongooseOsVersion == "" {
		manifest.MongooseOsVersion = "master"
	}

	return &manifest, nil
}

func buildRemote() error {
	appDir, err := getCodeDir()
	if err != nil {
		return errors.Trace(err)
	}

	// We'll need to amend the sources significantly with all libs, so copy them
	// to temporary dir first
	tmpCodeDir, err := ioutil.TempDir("", "tmp_mos_src_")
	if err != nil {
		return errors.Trace(err)
	}
	defer os.RemoveAll(tmpCodeDir)

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
	manifest, err := readManifestWithLibs(tmpCodeDir, nil, os.Stdout, userLibsDir, true /* skip clean */)
	if err != nil {
		return errors.Trace(err)
	}

	// Override arch with the value given in command line
	if *arch != "" {
		manifest.Arch = *arch
	}
	manifest.Arch = strings.ToLower(manifest.Arch)

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
	mv.subst[fmt.Sprintf("${%s}", name)] = value
}

func (mv *manifestVars) ExpandVars(s string) string {
	for k, v := range mv.subst {
		s = strings.Replace(s, k, v, -1)
	}
	return s
}

func setModuleVars(mVars *manifestVars, moduleName, path string) {
	mVars.SetVar(fmt.Sprintf("%s_path", cleanupModuleName(moduleName)), path)
}

func cleanupModuleName(name string) string {
	ret := ""
	for _, c := range name {
		if !unicode.IsLetter(c) {
			c = '_'
		}
		ret += string(c)
	}
	return ret
}

func readManifestWithLibs(
	dir string, visitedDirs []string, logFile io.Writer,
	userLibsDir string, skipClean bool,
) (*build.FWAppManifest, error) {
	for _, v := range visitedDirs {
		if dir == v {
			return nil, errors.Errorf("cyclic dependency of the lib %q", dir)
		}
	}

	manifest, err := readManifest(dir)
	if err != nil {
		return nil, errors.Trace(err)
	}

	curDir, err := getCodeDir()
	if err != nil {
		return nil, errors.Trace(err)
	}

	// Prepare all libs {{{
	var cleanLibs []build.SWModule
	for _, m := range manifest.Libs {
		name, err := m.GetName()
		if err != nil {
			return nil, errors.Trace(err)
		}

		reportf("Handling lib %q...", name)

		if skipClean {
			isClean, err := m.IsClean(libsDir)
			if err != nil {
				return nil, errors.Trace(err)
			}

			if isClean {
				reportf("Clean, skipping (will be handled remotely)")
				cleanLibs = append(cleanLibs, m)
				continue
			}
		}

		// Note: we always call PrepareLocalDir for libsDir, but then,
		// if userLibsDir is different, will need to copy it to the new location
		libDirAbs, err := m.PrepareLocalDir(libsDir, os.Stdout, true)
		libDirForManifest := libDirAbs
		if err != nil {
			return nil, errors.Trace(err)
		}

		// If libs should be placed in some specific dir, copy the current lib
		// there (it will also affect the libs path used in resulting manifest)
		if userLibsDir != libsDir {
			userLibsDirRel, err := filepath.Rel(dir, userLibsDir)
			if err != nil {
				return nil, errors.Trace(err)
			}

			userLocalDir := filepath.Join(userLibsDir, filepath.Base(libDirAbs))
			if err := ourio.CopyDir(libDirAbs, userLocalDir, []string{".git"}); err != nil {
				return nil, errors.Trace(err)
			}
			libDirAbs = filepath.Join(userLibsDir, filepath.Base(libDirAbs))
			libDirForManifest = filepath.Join(userLibsDirRel, filepath.Base(libDirAbs))
		}

		os.Chdir(libDirAbs)

		reportf("Prepared local dir: %q", libDirAbs)

		libManifest, err := readManifestWithLibs(
			libDirAbs, append(visitedDirs, dir), logFile, userLibsDir, skipClean,
		)
		if err != nil {
			return nil, errors.Trace(err)
		}

		// Extend manifest with libManifest {{{
		for _, s := range libManifest.Sources {
			// If the path is not absolute, and does not start with the variable,
			// prepend it with the library's path
			if s[0] != '$' && !filepath.IsAbs(s) {
				s = filepath.Join(libDirForManifest, s)
			}
			manifest.Sources = append(manifest.Sources, s)
		}

		for _, s := range libManifest.Filesystem {
			// If the path is not absolute, and does not start with the variable,
			// prepend it with the library's path
			if s[0] != '$' && !filepath.IsAbs(s) {
				s = filepath.Join(libDirForManifest, s)
			}
			manifest.Filesystem = append(manifest.Filesystem, s)
		}

		for _, m := range libManifest.Modules {
			manifest.Modules = append(manifest.Modules, m)
		}

		for k, s := range libManifest.BuildVars {
			switch k {
			case "APP_CONF_SCHEMA":
				if s[0] != '$' && !filepath.IsAbs(s) {
					s = filepath.Join(libDirForManifest, s)
				}

				manifest.BuildVars[k] += " " + s
			default:
				if _, ok := manifest.BuildVars[k]; !ok {
					manifest.BuildVars[k] = s
				}
			}
		}
		// }}}

		os.Chdir(curDir)
	}
	// }}}

	manifest.Libs = cleanLibs

	return manifest, nil
}
