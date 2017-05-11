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
	userpkg "os/user"
	"path"
	"path/filepath"
	"runtime"
	"strings"
	"unicode"

	"cesanta.com/common/go/multierror"
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
)

const (
	buildDir = "build"
	codeDir  = "."

	buildLog = "build.log"

	minSkeletonVersion = "2017-03-17"
	maxSkeletonVersion = "2017-03-17"
)

func init() {
	hiddenFlags = append(hiddenFlags, "docker_images")

	flag.StringSliceVar(&buildVarsSlice, "build-var", []string{}, "build variable in the format \"NAME:VALUE\" Can be used multiple times.")

	// deprecated since 2017/05/11
	flag.StringSliceVar(&buildVarsSlice, "build_var", []string{}, "deprecated, use --build-var")
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
		fmt.Printf("Success, built %s/%s version %s (%s).\n", fw.Name, fw.Platform, fw.Version, fw.BuildID)
	}

	fmt.Printf("Firmware saved to %s\n", fwFilename)

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

	manifest, err := readManifest()
	if err != nil {
		return errors.Trace(err)
	}

	archEffective, err := detectArch(manifest)
	if err != nil {
		return errors.Trace(err)
	}

	// Create map of given module locations, via --module flag(s)
	customModuleLocations := map[string]string{}
	for _, m := range *modules {
		parts := strings.SplitN(m, ":", 2)
		customModuleLocations[parts[0]] = parts[1]
	}

	mVars := NewManifestVars()
	mVars.SetVar("arch", archEffective)

	var mosDirEffective string
	if *mosRepo != "" {
		fmt.Printf("Using mongoose-os located at %q\n", *mosRepo)
		mosDirEffective = *mosRepo
	} else {
		fmt.Printf("The flag --repo is not given, going to use mongoose-os repository\n")
		mosDirEffective = "mongoose-os"

		m := build.SWModule{
			Type: "git",
			// TODO(dfrank) get upstream repo URL from a flag
			// (and this flag needs to be forwarded to fwbuild as well, which should
			// forward it to the mos invocation)
			Origin:  "https://github.com/cesanta/mongoose-os",
			Version: manifest.MongooseOsVersion,
		}

		if err := m.PrepareLocalCopy(mosDirEffective, logFile, true); err != nil {
			return errors.Trace(err)
		}
	}
	setModuleVars(mVars, "mongoose-os", mosDirEffective)

	mosDirEffectiveAbs, err := filepath.Abs(mosDirEffective)
	if err != nil {
		return errors.Annotatef(err, "getting absolute path of %q", mosDirEffective)
	}

	for _, m := range manifest.Modules {
		name, err := m.GetName()
		if err != nil {
			return errors.Trace(err)
		}

		targetDir, ok := customModuleLocations[name]
		if !ok {
			// Custom module location wasn't provided in command line, so, we'll
			// use the module name and will clone/pull it if necessary
			fmt.Printf("The flag --module is not given for the module %q, going to use the repository\n", name)
			targetDir = name

			if err := m.PrepareLocalCopy(targetDir, logFile, true); err != nil {
				return errors.Trace(err)
			}
		} else {
			fmt.Printf("Using module %q located at %q\n", name, targetDir)
		}

		setModuleVars(mVars, name, targetDir)
	}

	// Get sources and filesystem files from the manifest, expanding placeholders
	appSources := []string{}
	for _, s := range manifest.Sources {
		appSources = append(appSources, mVars.ExpandVars(s))
	}

	appFSFiles := []string{}
	for _, s := range manifest.Filesystem {
		appFSFiles = append(appFSFiles, mVars.ExpandVars(s))
	}

	// Makefile expects globs, not dir names, so we convert source and filesystem
	// dirs to the appropriate globs. Non-dir items will stay intact.
	appSources = globify(appSources, []string{"*.c", "*.cpp"})
	appFSFiles = globify(appFSFiles, []string{"*"})

	ffiSymbols := manifest.FFISymbols

	fmt.Printf("Building...\n")

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

		// On Windows and Mac, run container as root since volume sharing on those
		// OSes doesn't play nice with unprivileged user.
		//
		// On other OSes, run it as the current user.
		if runtime.GOOS != "windows" && runtime.GOOS != "darwin" {
			curUser, err := userpkg.Current()
			if err != nil {
				return errors.Trace(err)
			}

			dockerArgs = append(
				dockerArgs, "--user", fmt.Sprintf("%s:%s", curUser.Uid, curUser.Gid),
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
			fmt.Printf("Docker arguments: %s\n", strings.Join(dockerArgs, " "))
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
			fmt.Printf("Make arguments: %s\n", strings.Join(makeArgs, " "))
		}

		cmd := exec.Command("make", makeArgs...)
		err = runCmd(cmd, logFile)
		if err != nil {
			return errors.Trace(err)
		}
	}

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
		"-j",
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
func globify(srcPaths []string, globs []string) []string {
	ret := []string{}
	for _, p := range srcPaths {
		finfo, err := os.Stat(p)
		if err == nil && finfo.IsDir() {
			for _, glob := range globs {
				ret = append(ret, filepath.Join(p, glob))
			}
		} else {
			ret = append(ret, p)
		}
	}
	return ret
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

func readManifest() (*build.FWAppManifest, error) {
	wd, err := getCodeDir()
	if err != nil {
		return nil, errors.Trace(err)
	}

	manifestSrc, err := ioutil.ReadFile(filepath.Join(wd, build.ManifestFileName))
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
			"too old skeleton_version %q in %s (oldest supported is %q)",
			manifest.SkeletonVersion, build.ManifestFileName, minSkeletonVersion,
		)
	}

	if manifest.SkeletonVersion > maxSkeletonVersion {
		return nil, errors.Errorf(
			"too new skeleton_version %q in %s (latest supported is %q)",
			manifest.SkeletonVersion, build.ManifestFileName, maxSkeletonVersion,
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
	manifest, err := readManifest()
	if err != nil {
		return errors.Trace(err)
	}

	whitelist := map[string]bool{
		build.ManifestFileName: true,
		".": true,
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

	// We need to preprocess mos.yml (see setManifestArch())
	transformers[build.ManifestFileName] = func(r io.ReadCloser) (io.ReadCloser, error) {
		var buildVars map[string]string
		if len(buildVarsSlice) > 0 {
			buildVars = make(map[string]string)
			for _, v := range buildVarsSlice {
				parts := strings.SplitN(v, ":", 2)
				buildVars[parts[0]] = parts[1]
			}
		}
		return setManifestArch(r, *arch, buildVars)
	}

	// create a zip out of the current dir
	src, err := zipUp(".", whitelist, transformers)
	if err != nil {
		return errors.Trace(err)
	}
	if glog.V(2) {
		glog.V(2).Infof("zip:", hex.Dump(src))
	}

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
	fmt.Printf("Connecting to %s, user %s\n", server, buildUser)

	// invoke the fwbuild API
	uri := fmt.Sprintf("%s/api/%s/firmware/build", server, buildUser)

	fmt.Printf("Uploading sources (%d bytes)\n", len(body.Bytes()))
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

// setManifestArch takes manifest data, replaces architecture with the given
// value if it's not empty, sets app name to the current directory name if
// original value is empty, and returns resulting manifest data
func setManifestArch(
	r io.ReadCloser, arch string, buildVars map[string]string,
) (io.ReadCloser, error) {
	manifestData, err := ioutil.ReadAll(r)
	if err != nil {
		return nil, errors.Trace(err)
	}

	var manifest build.FWAppManifest
	if err := yaml.Unmarshal(manifestData, &manifest); err != nil {
		return nil, errors.Trace(err)
	}

	if arch != "" {
		manifest.Arch = arch
	}
	manifest.Arch = strings.ToLower(manifest.Arch)

	if buildVars != nil {
		if manifest.BuildVars == nil {
			manifest.BuildVars = make(map[string]string)
		}
		for k, v := range buildVars {
			manifest.BuildVars[k] = v
		}
	}

	manifest.Name, err = fixupAppName(manifest.Name)
	if err != nil {
		return nil, errors.Trace(err)
	}

	manifestData, err = yaml.Marshal(&manifest)
	if err != nil {
		return nil, errors.Trace(err)
	}

	return struct {
		io.Reader
		io.Closer
	}{bytes.NewReader(manifestData), r}, nil
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
