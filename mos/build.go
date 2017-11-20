package main

import (
	"archive/zip"
	"bytes"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"math/rand"
	"mime/multipart"
	"net/http"
	"net/url"
	"os"
	"os/exec"
	"os/signal"
	"path"
	"path/filepath"
	"runtime"
	"strings"
	"sync"
	"syscall"
	"time"
	"unicode"

	"golang.org/x/net/context"

	"cesanta.com/common/go/multierror"
	"cesanta.com/common/go/ourfilepath"
	"cesanta.com/common/go/ourio"
	"cesanta.com/common/go/ourutil"
	"cesanta.com/mos/build"
	"cesanta.com/mos/build/archive"
	moscommon "cesanta.com/mos/common"
	"cesanta.com/mos/common/paths"
	"cesanta.com/mos/dev"
	"cesanta.com/mos/flash/common"
	"cesanta.com/mos/interpreter"
	"cesanta.com/mos/manifest_parser"
	"cesanta.com/mos/mosgit"
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
	cflagsExtra      = flag.StringSlice("cflags-extra", []string{}, "extra C flag, appended to the \"cflags\" in the manifest. Can be used multiple times.")
	cxxflagsExtra    = flag.StringSlice("cxxflags-extra", []string{}, "extra C++ flag, appended to the \"cxxflags\" in the manifest. Can be used multiple times.")
	buildParalellism = flag.Int("build-parallelism", 0, "build parallelism. default is to use number of CPUs.")
	saveBuildStat    = flag.Bool("save-build-stat", true, "save build statistics")

	preferPrebuiltLibs = flag.Bool("prefer-prebuilt-libs", false, "if both sources and prebuilt binary of a lib exists, use the binary")

	buildVarsSlice []string

	noLibsUpdate  = flag.Bool("no-libs-update", false, "if true, never try to pull existing libs (treat existing default locations as if they were given in --lib)")
	skipCleanLibs = flag.Bool("skip-clean-libs", true, "if false, then during the remote build all libs will be uploaded to the builder")

	// In-memory buffer containing all the log messages.  It has to be
	// thread-safe, because it's used in compProviderReal, which is an
	// implementation of the manifest_parser.ComponentProvider interface, whose
	// methods are called concurrently.
	logBuf threadSafeBuffer

	// Log writer which always writes to the build.log file, os.Stderr and logBuf
	logWriterStderr io.Writer

	// The same as logWriterStderr, but skips os.Stderr unless --verbose is given
	logWriter io.Writer
)

const (
	projectDir = "."

	localLibsDir = "local_libs"
)

type buildParams struct {
	Platform              string
	BuildTarget           string
	CustomLibLocations    map[string]string
	CustomModuleLocations map[string]string
}

func init() {
	hiddenFlags = append(hiddenFlags, "docker_images")

	flag.StringSliceVar(&buildVarsSlice, "build-var", []string{}, "build variable in the format \"NAME:VALUE\" Can be used multiple times.")
}

// Build {{{

// Build command handler {{{
func buildHandler(ctx context.Context, devConn *dev.DevConn) error {
	// Create map of given lib locations, via --lib flag(s)
	cll, err := getCustomLibLocations()
	if err != nil {
		return errors.Trace(err)
	}

	// Create map of given module locations, via --module flag(s)
	cml := map[string]string{}
	for _, m := range *modules {
		parts := strings.SplitN(m, ":", 2)
		cml[parts[0]] = parts[1]
	}

	bParams := buildParams{
		Platform:              *platform,
		BuildTarget:           *buildTarget,
		CustomLibLocations:    cll,
		CustomModuleLocations: cml,
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

	if bParams.BuildTarget == moscommon.BuildTargetDefault {
		// We were building a firmware, so perform the required actions with moving
		// firmware around, etc.
		fwFilename := moscommon.GetFirmwareZipFilePath(buildDir)

		fw, err := common.NewZipFirmwareBundle(fwFilename)
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
	} else if p := moscommon.GetOrigLibArchiveFilePath(buildDir, bParams.Platform); bParams.BuildTarget == p {
		freportf(logWriterStderr, "Lib saved to %s", moscommon.GetLibArchiveFilePath(buildDir))
	} else {
		// We were building some custom target, so just report that we succeeded.
		freportf(logWriterStderr, "Target %s is built successfully", bParams.BuildTarget)
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

// }}}

// Local build {{{
func buildLocal(ctx context.Context, bParams *buildParams) (err error) {
	if isInDockerToolbox() {
		freportf(logWriterStderr, "Docker Toolbox detected")
	}

	gitinst := mosgit.NewOurGit()

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

	objsDir := moscommon.GetObjectDir(buildDirAbs)
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

	compProvider := compProviderReal{
		bParams:   bParams,
		logWriter: logWriter,
	}

	interp := interpreter.NewInterpreter(newMosVars())

	appDir, err := getCodeDirAbs()
	if err != nil {
		return errors.Trace(err)
	}

	buildVarsCli, err := getBuildVarsFromCLI()
	if err != nil {
		return errors.Trace(err)
	}

	manifest, fp, err := manifest_parser.ReadManifestFinal(
		appDir, &manifest_parser.ManifestAdjustments{
			Platform:  bParams.Platform,
			BuildVars: buildVarsCli,
		}, logWriter, interp,
		&manifest_parser.ReadManifestCallbacks{ComponentProvider: &compProvider}, true, *preferPrebuiltLibs,
	)
	if err != nil {
		return errors.Trace(err)
	}

	switch manifest.Type {
	case build.AppTypeApp:
		// Fine
	case build.AppTypeLib:
		bParams.BuildTarget = moscommon.GetOrigLibArchiveFilePath(buildDir, manifest.Platform)
		if manifest.Platform == "esp32" {
			*buildCmdExtra = append(*buildCmdExtra, "MGOS_MAIN_COMPONENT=moslib")
		}
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
		if err := os.Chtimes(curConfSchemaFName, fp.MTime, fp.MTime); err != nil {
			return errors.Trace(err)
		}
	}

	// Check if the app supports the given arch
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

	// Amend cflags and cxxflags with the values given in command line
	manifest.CFlags = append(manifest.CFlags, *cflagsExtra...)
	manifest.CXXFlags = append(manifest.CXXFlags, *cxxflagsExtra...)

	appSources, err := absPathSlice(manifest.Sources)
	if err != nil {
		return errors.Trace(err)
	}

	appIncludes, err := absPathSlice(manifest.Includes)
	if err != nil {
		return errors.Trace(err)
	}

	appFSFiles, err := absPathSlice(manifest.Filesystem)
	if err != nil {
		return errors.Trace(err)
	}

	appBinLibs, err := absPathSlice(manifest.BinaryLibs)
	if err != nil {
		return errors.Trace(err)
	}

	appSourceDirs, err := absPathSlice(fp.AppSourceDirs)
	if err != nil {
		return errors.Trace(err)
	}

	appFSDirs, err := absPathSlice(fp.AppFSDirs)
	if err != nil {
		return errors.Trace(err)
	}

	appBinLibDirs, err := absPathSlice(fp.AppBinLibDirs)
	if err != nil {
		return errors.Trace(err)
	}

	freportf(logWriter, "Sources: %v", appSources)
	freportf(logWriter, "Include dirs: %v", appIncludes)
	freportf(logWriter, "Binary libs: %v", appBinLibs)

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
		"APP_INCLUDES":   strings.Join(getPathsForDocker(appIncludes), " "),
		"APP_FS_FILES":   strings.Join(getPathsForDocker(appFSFiles), " "),
		"APP_BIN_LIBS":   strings.Join(getPathsForDocker(appBinLibs), " "),
		"FFI_SYMBOLS":    strings.Join(manifest.FFISymbols, " "),
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

	appPath, err := getCodeDirAbs()
	if err != nil {
		return errors.Trace(err)
	}

	// Invoke actual build (docker or make) {{{
	if os.Getenv("MGOS_SDK_REVISION") == "" && os.Getenv("MIOT_SDK_REVISION") == "" {
		// We're outside of the docker container, so invoke docker

		dockerRunArgs := []string{"--rm", "-i"}

		gitToplevelDir, _ := gitinst.GetToplevelDir(appPath)

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
		mp.addMountPoint(fp.MosDirEffective, dockerMgosPath)
		mp.addMountPoint(fp.MosDirEffective, getPathForDocker(fp.MosDirEffective))

		// Mount build dir
		mp.addMountPoint(buildDirAbs, getPathForDocker(buildDirAbs))

		// Mount all dirs with source files
		for _, d := range appSourceDirs {
			mp.addMountPoint(d, getPathForDocker(d))
		}

		// Mount all include paths
		for _, d := range appIncludes {
			mp.addMountPoint(d, getPathForDocker(d))
		}

		// Mount all dirs with filesystem files
		for _, d := range appFSDirs {
			mp.addMountPoint(d, getPathForDocker(d))
		}

		// Mount all dirs with binary libs
		for _, d := range appBinLibDirs {
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

		sdkVersionFile := filepath.Join(fp.MosDirEffective, "fw/platforms", manifest.Platform, "sdk.version")

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

		manifest.BuildVars["MGOS_PATH"] = fp.MosDirEffective

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

	if bParams.BuildTarget == moscommon.BuildTargetDefault {
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
	} else if p := moscommon.GetOrigLibArchiveFilePath(buildDir, manifest.Platform); bParams.BuildTarget == p {
		// Copy lib to build/lib.a
		err = ourio.LinkOrCopyFile(
			p, moscommon.GetLibArchiveFilePath(buildDir),
		)
		if err != nil {
			return errors.Trace(err)
		}
	}

	return nil
}

// }}}

// Remote build {{{
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

	buildVarsCli, err := getBuildVarsFromCLI()
	if err != nil {
		return errors.Trace(err)
	}

	interp := interpreter.NewInterpreter(newMosVars())

	manifest, _, err := manifest_parser.ReadManifest(tmpCodeDir, &manifest_parser.ManifestAdjustments{
		Platform:  bParams.Platform,
		BuildVars: buildVarsCli,
	}, interp)
	if err != nil {
		return errors.Trace(err)
	}

	if manifest.Platform == "" {
		return errors.Errorf("--platform must be specified or mos.yml should contain a platform key")
	}

	// Set the mos.platform variable
	interp.MVars.SetVar(interpreter.GetMVarNameMosPlatform(), manifest.Platform)

	// We still need to expand some conds we have so far, at least to ensure that
	// manifest.Sources contain all the app's sources we need to build, so that
	// they will be whitelisted (see whitelisting logic below) and thus uploaded
	// to the remote builder.
	if err := manifest_parser.ExpandManifestConds(manifest, manifest, interp); err != nil {
		return errors.Trace(err)
	}

	switch manifest.Type {
	case build.AppTypeApp:
		// Fine
	case build.AppTypeLib:
		bParams.BuildTarget = moscommon.GetOrigLibArchiveFilePath(buildDir, manifest.Platform)
	default:
		return errors.Errorf("invalid project type: %q", manifest.Type)
	}

	// Copy all external code (which is outside of the appDir) under tmpCodeDir {{{
	if err := copyExternalCodeAll(&manifest.Sources, appDir, tmpCodeDir); err != nil {
		return errors.Trace(err)
	}

	if err := copyExternalCodeAll(&manifest.Includes, appDir, tmpCodeDir); err != nil {
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

	// Amend cflags and cxxflags with the values given in command line
	manifest.CFlags = append(manifest.CFlags, *cflagsExtra...)
	manifest.CXXFlags = append(manifest.CXXFlags, *cxxflagsExtra...)

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
	for _, v := range manifest.Includes {
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

	pbValue := "0"
	if *preferPrebuiltLibs {
		pbValue = "1"
	}

	if err := mpw.WriteField(moscommon.FormPreferPrebuildLibsName, pbValue); err != nil {
		return errors.Trace(err)
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

// }}}

// }}}

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
		makeArgs = append(makeArgs, fmt.Sprintf(
			"%s=%s",
			k,
			strings.Replace(strings.Replace(v, "\n", " ", -1), "\r", " ", -1),
		))
	}

	// Add extra make args
	if buildCmdExtra != nil {
		makeArgs = append(makeArgs, (*buildCmdExtra)...)
	}

	return makeArgs, nil
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

func getBuildVarsFromCLI() (map[string]string, error) {
	m := make(map[string]string)

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
			return nil, errors.Errorf("invalid --build-var spec: %q", v)
		}
		m[pp[0]] = pp[1]
	}

	return m, nil
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

type fileTransformer func(r io.ReadCloser) (io.ReadCloser, error)

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

func getCodeDirAbs() (string, error) {
	absCodeDir, err := filepath.Abs(projectDir)
	if err != nil {
		return "", errors.Trace(err)
	}

	absCodeDir, err = filepath.EvalSymlinks(absCodeDir)
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

func identityTransformer(r io.ReadCloser) (io.ReadCloser, error) {
	return r, nil
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

func generateCflags(cflags []string, cdefs map[string]string) string {
	for k, v := range cdefs {
		cflags = append(cflags, fmt.Sprintf("-D%s=%s", k, v))
	}

	return strings.Join(append(cflags), " ")
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

func newMosVars() *interpreter.MosVars {
	ret := interpreter.NewMosVars()
	ret.SetVar(interpreter.GetMVarNameMosVersion(), version.GetMosVersion())
	return ret
}

func isInDockerToolbox() bool {
	return os.Getenv("DOCKER_HOST") != ""
}

func absPathSlice(slice []string) ([]string, error) {
	ret := make([]string, len(slice))
	for i, v := range slice {
		var err error
		if !filepath.IsAbs(v) {
			ret[i], err = filepath.Abs(v)
			if err != nil {
				return nil, errors.Trace(err)
			}
		} else {
			ret[i] = v
		}
	}
	return ret, nil
}

func getGithubLibAssetUrl(repoUrl, platform, version string) (string, error) {
	u, err := url.Parse(repoUrl)
	if err != nil {
		return "", errors.Trace(err)
	}

	_, name := path.Split(u.Path)

	return fmt.Sprintf("%s/releases/download/%s/lib%s-%s.a", repoUrl, version, name, platform), nil
}

// Docker utils {{{

// Docker mount points {{{
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

	freportf(logWriter, "mount from %q to %q", hostPath, containerPath)
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

// }}}

// Docker paths {{{
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

// }}}

// Docker build {{{
func runDockerBuild(dockerRunArgs []string) error {
	containerName := fmt.Sprintf(
		"mos_build_%s_%d", time.Now().Format("2006-01-02T15-04-05-00"), rand.Int(),
	)

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

// }}}

// }}}

// manifest_parser.ComponentProvider implementation {{{
type compProviderReal struct {
	bParams   *buildParams
	logWriter io.Writer
}

func (lpr *compProviderReal) GetLibLocalPath(
	m *build.SWModule, rootAppDir, libsDefVersion, platform string,
) (string, error) {
	gitinst := mosgit.NewOurGit()

	name, err := m.GetName()
	if err != nil {
		return "", errors.Trace(err)
	}

	appDir, err := getCodeDirAbs()
	if err != nil {
		return "", errors.Trace(err)
	}

	libsDir := getDepsDir(appDir)

	libDirAbs, ok := lpr.bParams.CustomLibLocations[name]
	if !ok {

		for {
			ourutil.Freportf(lpr.logWriter, "The --lib flag was not given for it, checking repository")

			needUpdate := true

			localDir, err := m.GetLocalDir(libsDir, libsDefVersion)
			if err != nil {
				return "", errors.Trace(err)
			}

			if _, err := os.Stat(localDir); err == nil {
				// lib's local dir already exists

				if *noLibsUpdate {
					ourutil.Freportf(lpr.logWriter, "--no-libs-update was given, and %q exists: skipping update", localDir)
					libDirAbs = localDir
					needUpdate = false
				}
			}

			if needUpdate {

				// Try to get current hash, ignoring errors
				curHash := ""
				if m.GetType() == build.SWModuleTypeGithub {
					curHash, _ = gitinst.GetCurrentHash(localDir)
				}

				libDirAbs, err = m.PrepareLocalDir(libsDir, lpr.logWriter, true, libsDefVersion, *libsUpdateInterval)
				if err != nil {
					if m.Version == "" && libsDefVersion != "latest" {
						// We failed to fetch lib at the default version (mos.version),
						// which is not "latest", and the lib in manifest does not have
						// version specified explicitly. This might happen when some
						// latest app is built with older mos tool.

						serverVersion := libsDefVersion
						v, err := update.GetServerMosVersion(update.GetUpdateChannel())
						if err == nil {
							serverVersion = version.GetMosVersionFromBuildId(v.BuildId)
						}

						ourutil.Freportf(logWriterStderr,
							"WARNING: the lib %q does not have version %s. Resorting to latest, but the build might fail.\n"+
								"It usually happens if you clone the latest version of some example app, and try to build it with the mos tool which is older than the lib (in this case, %q).", name, libsDefVersion, name,
						)

						if serverVersion != version.GetMosVersion() {
							// There is a newer version of the mos tool available, so
							// suggest upgrading.

							ourutil.Freportf(logWriterStderr,
								"There is a newer version of the mos tool available: %s, try to update mos tool (mos update), and build again. "+
									"Alternatively, you can build the version %s of the app (git checkout %s).", serverVersion, libsDefVersion, libsDefVersion,
							)
						} else {
							// Current mos is at the newest released version, so the only
							// alternatives are: build older (released) version of the app,
							// or use latest mos.

							ourutil.Freportf(logWriterStderr,
								"Consider using the version %s of the app (git checkout %s), or using latest mos tool (mos update latest).", libsDefVersion, libsDefVersion,
							)
						}

						// In any case, retry with the latest lib version and cross fingers.

						libsDefVersion = "latest"
						continue
					}
					return "", errors.Annotatef(err, "preparing local copy of the lib %q", name)
				}

				if m.GetType() == build.SWModuleTypeGithub {
					if newHash, err := gitinst.GetCurrentHash(localDir); err == nil && newHash != curHash {
						freportf(logWriter, "Hash is updated: %q -> %q", curHash, newHash)
						// The current repo hash has changed after the pull, so we need to
						// vanish the lib we might have downloaded before
						os.RemoveAll(moscommon.GetBinaryLibsDir(localDir))

						// But in case the lib dir is a part of the repo itself, we have to
						// do "git checkout ." on the repo. We shouldn't be afraid of
						// losing user's local changes, because the fact that hash has
						// changed means that the repo was clean anyway.
						gitinst.ResetHard(localDir)
					}
				}

				// Check if prebuilt binary exists, and if not, try to fetch it
				prebuiltFilePath := moscommon.GetBinaryLibFilePath(localDir, name, platform)

				if _, err := os.Stat(prebuiltFilePath); err != nil {
					// Prebuilt binary doesn't exist; let's see if we can fetch it
					err = fetchPrebuiltBinary(m, platform, prebuiltFilePath)
					if err == nil {
						ourutil.Freportf(lpr.logWriter, "Successfully fetched prebuilt binary for %q to %q", name, prebuiltFilePath)
					} else {
						ourutil.Freportf(lpr.logWriter, "Falling back to sources for %q (failed to fetch prebuilt binary: %s)", name, err.Error())
					}
				} else {
					ourutil.Freportf(lpr.logWriter, "Prebuilt binary for %q already exists", name)
				}
			}

			break
		}
	} else {
		ourutil.Freportf(lpr.logWriter, "Using the location %q as is (given as a --lib flag)", libDirAbs)
	}
	ourutil.Freportf(lpr.logWriter, "Prepared local dir: %q", libDirAbs)

	return libDirAbs, nil
}

func (lpr *compProviderReal) GetModuleLocalPath(
	m *build.SWModule, rootAppDir, modulesDefVersion, platform string,
) (string, error) {
	name, err := m.GetName()
	if err != nil {
		return "", errors.Trace(err)
	}

	targetDir, ok := lpr.bParams.CustomModuleLocations[name]
	if !ok {
		// Custom module location wasn't provided in command line, so, we'll
		// use the module name and will clone/pull it if necessary
		freportf(logWriter, "The flag --module is not given for the module %q, going to use the repository", name)

		var err error
		targetDir, err = m.PrepareLocalDir(paths.ModulesDir, logWriter, true, modulesDefVersion, *libsUpdateInterval)
		if err != nil {
			return "", errors.Annotatef(err, "preparing local copy of the module %q", name)
		}
	} else {
		freportf(logWriter, "Using module %q located at %q", name, targetDir)
	}

	return targetDir, nil
}

func (lpr *compProviderReal) GetMongooseOSLocalPath(
	rootAppDir, modulesDefVersion string,
) (string, error) {
	targetDir, err := getMosDirEffective(modulesDefVersion, *libsUpdateInterval)
	if err != nil {
		return "", errors.Trace(err)
	}

	return targetDir, nil
}

func getDepsDir(projectDir string) string {
	if paths.LibsDir != "" {
		return paths.LibsDir
	} else {
		return moscommon.GetDepsDir(projectDir)
	}
}

func getMosDirEffective(mongooseOsVersion string, updateInterval time.Duration) (string, error) {
	var mosDirEffective string
	if *mosRepo != "" {
		freportf(logWriter, "Using mongoose-os located at %q", *mosRepo)
		mosDirEffective = *mosRepo
	} else {
		freportf(logWriter, "The flag --repo is not given, going to use mongoose-os repository")

		m := build.SWModule{
			// TODO(dfrank) get upstream repo URL from a flag
			// (and this flag needs to be forwarded to fwbuild as well, which should
			// forward it to the mos invocation)
			Location:  "https://github.com/cesanta/mongoose-os",
			Version:   mongooseOsVersion,
			SuffixTpl: manifest_parser.SwmodSuffixTpl,
		}

		var err error
		mosDirEffective, err = m.PrepareLocalDir(paths.ModulesDir, logWriter, true, "", updateInterval)
		if err != nil {
			return "", errors.Annotatef(err, "preparing local copy of the mongoose-os repo")
		}
	}

	return mosDirEffective, nil
}

func fetchPrebuiltBinary(m *build.SWModule, platform, tgt string) error {
	switch m.GetType() {
	case build.SWModuleTypeGithub:
		assetUrl, err := getGithubLibAssetUrl(m.Location, platform, version.GetMosVersion())
		if err != nil {
			return errors.Trace(err)
		}

		resp, err := http.Get(assetUrl)
		if err != nil {
			return errors.Trace(err)
		}

		defer resp.Body.Close()

		if resp.StatusCode != http.StatusOK {
			return errors.Errorf("got %d status code when accessed %s", resp.StatusCode, assetUrl)
		}

		// Fetched the asset successfully
		data, err := ioutil.ReadAll(resp.Body)
		if err != nil {
			return errors.Trace(err)
		}

		if err := os.MkdirAll(filepath.Dir(tgt), 0755); err != nil {
			return errors.Trace(err)
		}

		if err := ioutil.WriteFile(tgt, data, 0644); err != nil {
			return errors.Trace(err)
		}

	default:
		return errors.Errorf("unable to fetch library for swmodule of type %v", m.GetType())
	}

	return nil
}

// }}}

// Thread-safe bytes.Buffer {{{

type threadSafeBuffer struct {
	buf bytes.Buffer
	mtx sync.Mutex
}

func (b *threadSafeBuffer) Write(p []byte) (n int, err error) {
	b.mtx.Lock()
	defer b.mtx.Unlock()

	return b.buf.Write(p)
}

func (b *threadSafeBuffer) Bytes() []byte {
	b.mtx.Lock()
	defer b.mtx.Unlock()

	return b.buf.Bytes()
}

// }}}
