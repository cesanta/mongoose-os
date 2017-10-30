//go:generate go-bindata -pkg manifest_parser -nocompress -modtime 1 -mode 420 data/
//go:generate go-bindata-assetfs -pkg manifest_parser -nocompress -modtime 1 -mode 420 data/

// Check README.md for detailed explanation of parsing steps, limitations etc.

package manifest_parser

import (
	"bytes"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"sync"
	"text/template"
	"time"

	"cesanta.com/common/go/ourutil"
	"cesanta.com/mos/build"
	moscommon "cesanta.com/mos/common"
	"cesanta.com/mos/interpreter"
	"github.com/cesanta/errors"
	flag "github.com/spf13/pflag"
	yaml "gopkg.in/yaml.v2"
)

const (
	// Manifest version changes:
	//
	// - 2017-06-03: added support for @all_libs in filesystem and sources
	// - 2017-06-16: added support for conds with very basic expressions
	//               (only build_vars)
	// - 2017-09-29: added support for includes
	minManifestVersion = "2017-03-17"
	maxManifestVersion = "2017-09-29"

	depsApp = "app"

	allLibsKeyword = "@all_libs"

	assetPrefix           = "asset://"
	rootManifestAssetName = "data/root_manifest.yml"

	SwmodSuffixTpl = "-${version}"
)

var (
	sourceGlobs = flag.StringSlice("source-glob", []string{"*.c", "*.cpp"}, "glob to use for source dirs. Can be used multiple times.")
)

type ComponentProvider interface {
	// GetLibLocalPath returns local path to the given software module.
	// NOTE that this method can be called concurrently for different modules.
	GetLibLocalPath(
		m *build.SWModule, rootAppDir, libsDefVersion string,
	) (string, error)

	GetModuleLocalPath(
		m *build.SWModule, rootAppDir, modulesDefVersion string,
	) (string, error)

	GetMongooseOSLocalPath(rootAppDir, mongooseOSVersion string) (string, error)
}

type ReadManifestCallbacks struct {
	ComponentProvider ComponentProvider
}

type RMFOut struct {
	MTime time.Time

	MosDirEffective string

	AppSourceDirs []string
	AppFSDirs     []string
	AppBinLibDirs []string
}

type libPrepareResult struct {
	mtime time.Time
	err   error
}

func ReadManifestFinal(
	dir, platform string,
	logWriter io.Writer, interp *interpreter.MosInterpreter,
	cbs *ReadManifestCallbacks,
	requireArch, preferPrebuiltLibs bool,
) (*build.FWAppManifest, *RMFOut, error) {
	interp = interp.Copy()
	fp := &RMFOut{}
	buildDirAbs, err := filepath.Abs(moscommon.GetBuildDir(dir))
	if err != nil {
		return nil, nil, errors.Trace(err)
	}

	manifest, mtime, err := readManifestWithLibs(
		dir, platform, logWriter, interp, cbs, requireArch,
	)
	if err != nil {
		return nil, nil, errors.Trace(err)
	}

	// Set the mos.platform variable
	interp.MVars.SetVar(interpreter.GetMVarNameMosPlatform(), manifest.Platform)

	if err := interpreter.SetManifestVars(interp.MVars, manifest); err != nil {
		return nil, nil, errors.Trace(err)
	}

	// Prepare local copies of all sw modules {{{
	for _, m := range manifest.Modules {
		name, err := m.GetName()
		if err != nil {
			return nil, nil, errors.Trace(err)
		}

		moduleDir, err := cbs.ComponentProvider.GetModuleLocalPath(&m, dir, manifest.ModulesVersion)
		if err != nil {
			return nil, nil, errors.Trace(err)
		}

		interpreter.SetModuleVars(interp.MVars, name, moduleDir)
	}
	// }}}

	// Determine mongoose-os dir (fp.MosDirEffective) {{{
	fp.MosDirEffective, err = cbs.ComponentProvider.GetMongooseOSLocalPath(
		dir, manifest.MongooseOsVersion,
	)
	if err != nil {
		return nil, nil, errors.Trace(err)
	}

	fp.MosDirEffective, err = filepath.Abs(fp.MosDirEffective)
	if err != nil {
		return nil, nil, errors.Annotatef(err, "getting absolute path of %q", fp.MosDirEffective)
	}

	interpreter.SetModuleVars(interp.MVars, "mongoose-os", fp.MosDirEffective)
	// }}}

	// Get sources and filesystem files from the manifest, expanding expressions {{{
	manifest.Sources, err = interpreter.ExpandVarsSlice(interp, manifest.Sources, false)
	if err != nil {
		return nil, nil, errors.Trace(err)
	}

	for k, v := range manifest.LibsHandled {
		manifest.LibsHandled[k].Sources, err = interpreter.ExpandVarsSlice(interp, v.Sources, false)
		if err != nil {
			return nil, nil, errors.Trace(err)
		}
	}

	manifest.Includes, err = interpreter.ExpandVarsSlice(interp, manifest.Includes, false)
	if err != nil {
		return nil, nil, errors.Trace(err)
	}

	// AppSourceDirs will be populated later, needed to mount those paths to the
	// docker container
	fp.AppSourceDirs = []string{}

	manifest.Filesystem, err = interpreter.ExpandVarsSlice(interp, manifest.Filesystem, false)
	if err != nil {
		return nil, nil, errors.Trace(err)
	}

	manifest.BinaryLibs, err = interpreter.ExpandVarsSlice(interp, manifest.BinaryLibs, false)
	if err != nil {
		return nil, nil, errors.Trace(err)
	}

	manifest.Tests, err = interpreter.ExpandVarsSlice(interp, manifest.Tests, false)
	if err != nil {
		return nil, nil, errors.Trace(err)
	}

	manifest.Sources = prependPaths(manifest.Sources, dir)
	manifest.Includes = prependPaths(manifest.Includes, dir)
	manifest.Filesystem = prependPaths(manifest.Filesystem, dir)
	manifest.BinaryLibs = prependPaths(manifest.BinaryLibs, dir)

	manifest.Tests = prependPaths(manifest.Tests, dir)
	// }}}

	if manifest.Type == build.AppTypeApp {
		// Generate deps_init C code, and if it's not empty, write it to the temp
		// file and add to sources
		depsCCode, err := getDepsInitCCode(manifest)
		if err != nil {
			return nil, nil, errors.Trace(err)
		}

		if len(depsCCode) != 0 {
			fname := moscommon.GetDepsInitCFilePath(buildDirAbs)

			if err := os.MkdirAll(moscommon.GetGeneratedFilesDir(buildDirAbs), 0777); err != nil {
				return nil, nil, errors.Trace(err)
			}

			if err = ioutil.WriteFile(fname, depsCCode, 0666); err != nil {
				return nil, nil, errors.Trace(err)
			}

			// The modification time of autogenerated file should be set to that of
			// the manifest itself, so that make handles dependencies correctly.
			if err := os.Chtimes(fname, mtime, mtime); err != nil {
				return nil, nil, errors.Trace(err)
			}

			manifest.Sources = append(manifest.Sources, fname)
		}
	}

	// Convert manifest.Sources into paths to concrete existing source files.
	manifest.Sources, fp.AppSourceDirs, err = resolvePaths(manifest.Sources, *sourceGlobs)
	if err != nil {
		return nil, nil, errors.Trace(err)
	}

	manifest.Filesystem, fp.AppFSDirs, err = resolvePaths(manifest.Filesystem, []string{"*"})
	if err != nil {
		return nil, nil, errors.Trace(err)
	}

	// When building an app, also add all libs' sources or prebuilt binaries.
	if manifest.Type == build.AppTypeApp {
		for k, lcur := range manifest.LibsHandled {
			libSourceDirs := []string{}

			// Convert dirs and globs to actual files
			manifest.LibsHandled[k].Sources, libSourceDirs, err = resolvePaths(lcur.Sources, *sourceGlobs)
			if err != nil {
				return nil, nil, errors.Trace(err)
			}

			// Check if binary version of the lib exists. If so, maybe use it instead
			// of sources.
			binaryLib := ""
			bl := moscommon.GetBinaryLibFilePath(lcur.Path, lcur.Name, manifest.Platform)
			if _, err := os.Stat(bl); err == nil {
				bl, err := filepath.Abs(bl)
				if err != nil {
					return nil, nil, errors.Trace(err)
				}

				// Prebuilt lib exists: use it if either preferPrebuiltLibs is true
				// (so that we prefer binary) or if there are no sources.
				if len(manifest.LibsHandled[k].Sources) == 0 || preferPrebuiltLibs {
					binaryLib = bl
				}
			}

			if binaryLib != "" {
				// We should use binary lib instead of sources
				manifest.LibsHandled[k].Sources = []string{}
				manifest.BinaryLibs = append(manifest.BinaryLibs, binaryLib)
			} else {
				// Use lib sources, not prebuilt binary
				manifest.Sources = append(manifest.Sources, manifest.LibsHandled[k].Sources...)
			}

			fp.AppSourceDirs = append(fp.AppSourceDirs, libSourceDirs...)
		}
	}

	manifest.BinaryLibs, fp.AppBinLibDirs, err = resolvePaths(manifest.BinaryLibs, []string{"*.a"})
	if err != nil {
		return nil, nil, errors.Trace(err)
	}

	manifest.Tests, _, err = resolvePaths(manifest.Tests, []string{"*"})
	if err != nil {
		return nil, nil, errors.Trace(err)
	}

	allPlatforms, err := getAllSupportedPlatforms(fp.MosDirEffective)
	if err != nil {
		return nil, nil, errors.Trace(err)
	}

	manifest.Platforms = mergeSupportedPlatforms(manifest.Platforms, allPlatforms)
	sort.Strings(manifest.Platforms)

	fp.MTime = mtime

	return manifest, fp, nil
}

// readManifestWithLibs reads manifest from the provided dir, "expands" all
// libs (so that the returned manifest does not really contain any libs),
// and also returns the most recent modification time of all encountered
// manifests.
func readManifestWithLibs(
	dir, platform string,
	logWriter io.Writer, interp *interpreter.MosInterpreter,
	cbs *ReadManifestCallbacks,
	requireArch bool,
) (*build.FWAppManifest, time.Time, error) {
	interp = interp.Copy()
	libsHandled := map[string]build.FWAppManifestLibHandled{}

	// Create a deps structure and add a root node: an "app"
	deps := NewDeps()
	deps.AddNode(depsApp)

	manifest, mtime, err := readManifestWithLibs2(manifestParseContext{
		dir:        dir,
		rootAppDir: dir,

		platform:  platform,
		logWriter: logWriter,

		nodeName:    depsApp,
		deps:        deps,
		libsHandled: libsHandled,

		appManifest: nil,
		interp:      interp,

		requireArch: requireArch,

		cbs: cbs,

		mtx:     &sync.Mutex{},
		flagSet: newStringFlagSet(),
	})
	if err != nil {
		return nil, time.Time{}, errors.Trace(err)
	}

	// Set the mos.platform variable
	interp.MVars.SetVar(interpreter.GetMVarNameMosPlatform(), manifest.Platform)

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
		manifest.LibsHandled = append(manifest.LibsHandled, libsHandled[v])
	}

	if err := expandManifestLibsAndConds(manifest, interp); err != nil {
		return nil, time.Time{}, errors.Trace(err)
	}

	if err := expandManifestAllLibsPaths(manifest); err != nil {
		return nil, time.Time{}, errors.Trace(err)
	}

	return manifest, mtime, nil
}

type manifestParseContext struct {
	// Manifest's directory
	dir string

	// Directory of the "root" app; for the app's manifest it's the same as dir.
	// Might be a temporary directory
	rootAppDir string

	platform  string
	logWriter io.Writer

	nodeName    string
	deps        *Deps
	libsHandled map[string]build.FWAppManifestLibHandled

	appManifest *build.FWAppManifest
	interp      *interpreter.MosInterpreter

	cbs *ReadManifestCallbacks

	requireArch bool

	mtx     *sync.Mutex
	flagSet *stringFlagSet
}

func readManifestWithLibs2(pc manifestParseContext) (*build.FWAppManifest, time.Time, error) {
	manifest, mtime, err := ReadManifest(pc.dir, pc.platform, pc.interp)
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

	// Prepare all libs {{{
	wg := &sync.WaitGroup{}
	wg.Add(len(manifest.Libs))

	lpres := make(chan libPrepareResult)

	for _, m := range manifest.Libs {
		go prepareLib(m, manifest, &pc, lpres, wg)
	}

	// Closer goroutine
	go func() {
		wg.Wait()
		close(lpres)
	}()
	// }}}

	// Handle all lib prepare results
	for res := range lpres {
		if res.err != nil {
			return nil, time.Time{}, errors.Trace(res.err)
		}

		// We should return the latest modification date of all encountered
		// manifests, so let's see if we got the later mtime here
		if res.mtime.After(mtime) {
			mtime = res.mtime
		}
	}

	manifest.Libs = nil

	return manifest, mtime, nil
}

func prepareLib(
	m build.SWModule,
	manifest *build.FWAppManifest,
	pc *manifestParseContext,
	lpres chan libPrepareResult,
	wg *sync.WaitGroup,
) {
	defer wg.Done()
	m.SuffixTpl = ""

	name, err := m.GetName()
	if err != nil {
		lpres <- libPrepareResult{
			err: errors.Trace(err),
		}
		return
	}

	pc.mtx.Lock()
	pc.deps.AddDep(pc.nodeName, name)
	pc.mtx.Unlock()

	if m.Weak {
		ourutil.Freportf(pc.logWriter, "Lib %q is optional, skipping", name)
		return
	}

	if !pc.flagSet.Add(name) {
		// That library is already handled by someone else
		ourutil.Freportf(pc.logWriter, "Lib %q is already handled, skipping", name)
		return
	}

	ourutil.Freportf(pc.logWriter, "Handling lib %q...", name)

	libLocalDir, err := pc.cbs.ComponentProvider.GetLibLocalPath(
		&m, pc.rootAppDir, pc.appManifest.LibsVersion,
	)
	if err != nil {
		lpres <- libPrepareResult{
			err: errors.Trace(err),
		}
		return
	}

	libLocalDir, err = filepath.Abs(libLocalDir)
	if err != nil {
		lpres <- libPrepareResult{
			err: errors.Trace(err),
		}
		return
	}

	// Now that we know we need to handle current lib, add a node for it
	pc.mtx.Lock()
	pc.deps.AddNode(name)
	pc.mtx.Unlock()

	pc2 := manifestParseContext{}
	pc2 = *pc

	pc2.dir = libLocalDir
	pc2.nodeName = name

	// If platform is empty in pc2, we need to set it from the outer manifest,
	// because arch is used in libs to handle arch-dependent submanifests, like
	// mos_esp8266.yml.
	if pc2.platform == "" {
		pc2.platform = manifest.Platform
	}

	libManifest, libMtime, err := readManifestWithLibs2(pc2)
	if err != nil {
		lpres <- libPrepareResult{
			err: errors.Trace(err),
		}
		return
	}

	// Add a build var and C macro MGOS_HAVE_<lib_name>
	haveName := fmt.Sprintf(
		"MGOS_HAVE_%s", strings.ToUpper(moscommon.IdentifierFromString(name)),
	)
	pc.mtx.Lock()
	manifest.BuildVars[haveName] = "1"
	manifest.CDefs[haveName] = "1"

	pc.libsHandled[name] = build.FWAppManifestLibHandled{
		Name:     name,
		Path:     libLocalDir,
		Deps:     pc.deps.GetDeps(name),
		Manifest: libManifest,
	}
	pc.mtx.Unlock()

	lpres <- libPrepareResult{
		mtime: libMtime,
	}
}

// ReadManifest reads manifest file(s) from the specific directory; if the
// manifest or given BuildParams have arch specified, then the returned
// manifest will contain all arch-specific adjustments (if any)
func ReadManifest(
	appDir, platform string, interp *interpreter.MosInterpreter,
) (*build.FWAppManifest, time.Time, error) {
	interp = interp.Copy()

	manifestFullName := moscommon.GetManifestFilePath(appDir)
	manifest, mtime, err := ReadManifestFile(manifestFullName, interp, true)
	if err != nil {
		return nil, time.Time{}, errors.Trace(err)
	}

	// Override arch with the value given in command line
	if platform != "" {
		manifest.Platform = platform
	}
	manifest.Platform = strings.ToLower(manifest.Platform)

	// Set the mos.platform variable
	interp.MVars.SetVar(interpreter.GetMVarNameMosPlatform(), manifest.Platform)

	// If type is omitted, assume "app"
	if manifest.Type == "" {
		manifest.Type = build.AppTypeApp
	}

	if manifest.Platform != "" {
		manifestArchFullName := moscommon.GetManifestArchFilePath(appDir, manifest.Platform)
		_, err := os.Stat(manifestArchFullName)
		if err == nil {
			// Arch-specific mos.yml does exist, so, handle it
			archManifest, archMtime, err := ReadManifestFile(manifestArchFullName, interp, false)
			if err != nil {
				return nil, time.Time{}, errors.Trace(err)
			}

			// We should return the latest modification date of all encountered
			// manifests, so let's see if we got the later mtime here
			if archMtime.After(mtime) {
				mtime = archMtime
			}

			// Extend common app manifest with arch-specific things.
			if err := extendManifest(manifest, manifest, archManifest, "", "", interp, &extendManifestOptions{
				skipFailedExpansions: true,
			}); err != nil {
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

// ReadManifestFile reads single manifest file (which can be either "main" app
// or lib manifest, or some arch-specific adjustment manifest)
func ReadManifestFile(
	manifestFullName string, interp *interpreter.MosInterpreter, manifestVersionMandatory bool,
) (*build.FWAppManifest, time.Time, error) {
	interp = interp.Copy()
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
		//ourutil.Freportf(logWriterStderr, "WARNING: skeleton_version is deprecated and will be removed eventually, please rename it to manifest_version")
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
		manifest.Modules[i].SuffixTpl = SwmodSuffixTpl
	}

	if manifest.BuildVars == nil {
		manifest.BuildVars = make(map[string]string)
	}

	if manifest.CDefs == nil {
		manifest.CDefs = make(map[string]string)
	}

	if manifest.MongooseOsVersion == "" {
		manifest.MongooseOsVersion = interpreter.WrapMosExpr(interpreter.GetMVarNameMosVersion())
	}

	if manifest.LibsVersion == "" {
		manifest.LibsVersion = interpreter.WrapMosExpr(interpreter.GetMVarNameMosVersion())
	}

	if manifest.ModulesVersion == "" {
		manifest.ModulesVersion = interpreter.WrapMosExpr(interpreter.GetMVarNameMosVersion())
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

	manifest.MongooseOsVersion, err = interpreter.ExpandVars(interp, manifest.MongooseOsVersion, false)
	if err != nil {
		return nil, time.Time{}, errors.Trace(err)
	}

	manifest.LibsVersion, err = interpreter.ExpandVars(interp, manifest.LibsVersion, false)
	if err != nil {
		return nil, time.Time{}, errors.Trace(err)
	}

	manifest.ModulesVersion, err = interpreter.ExpandVars(interp, manifest.ModulesVersion, false)
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
	interp = interp.Copy()

	// First of all, read root manifest since it should be the first manifest
	// in the chain (see below)
	rootManifest, _, err := ReadManifestFile(
		fmt.Sprint(assetPrefix, rootManifestAssetName), interp, true,
	)
	if err != nil {
		return errors.Trace(err)
	}

	// We need everything under root manifest's conds to be already available, so
	// expand all conds there. It means that the conds in root manifest should
	// only depend on the stuff already defined (basically, only "mos.platform").
	//
	// TODO(dfrank): probably make it so that if conds expression fails to
	// evaluate, keep it unexpanded for now.
	if err := ExpandManifestConds(rootManifest, rootManifest, interp); err != nil {
		return errors.Trace(err)
	}

	for {
		// First, we build a chain of all manifests we have:
		//
		// - Dummy empty manifest (needed so that extendManifest() will be called
		//   with the actual first manifest as "m2", and thus will expand
		//   expressions in its BuildVars and CDefs)
		// - Root manifest
		// - All libs (if any), starting from the one without any deps
		// - App
		allManifests := []*build.FWAppManifestLibHandled{}
		allManifests = append(allManifests, &build.FWAppManifestLibHandled{
			Name:     "dummy_empty_manifest",
			Path:     "",
			Manifest: &build.FWAppManifest{},
		})

		allManifests = append(allManifests, &build.FWAppManifestLibHandled{
			Name:     "root_manifest",
			Path:     "",
			Manifest: rootManifest,
		})

		//allManifests = append(allManifests, manifest.LibsHandled...)
		for k, _ := range manifest.LibsHandled {
			allManifests = append(allManifests, &manifest.LibsHandled[k])
		}
		allManifests = append(allManifests, &build.FWAppManifestLibHandled{
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

			lcur.Sources = prependPaths(curManifest.Sources, lcur.Path)

			if err := extendManifest(
				&curManifest, commonManifest, &curManifest, "", lcur.Path, interp, &extendManifestOptions{
					skipSources: true,
				},
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

		if err := ExpandManifestConds(manifest, commonManifest, interp); err != nil {
			return errors.Trace(err)
		}

		for k := range manifest.LibsHandled {
			if manifest.LibsHandled[k].Manifest != nil {
				if err := ExpandManifestConds(
					manifest.LibsHandled[k].Manifest, commonManifest, interp,
				); err != nil {
					return errors.Trace(err)
				}
			}
		}
	}
}

// expandManifestAllLibsPaths expands "@all_libs" for manifest's Sources
// and Filesystem paths
func expandManifestAllLibsPaths(manifest *build.FWAppManifest) error {
	var err error

	manifest.Sources, err = expandAllLibsPaths(manifest.Sources, manifest.LibsHandled)
	if err != nil {
		return errors.Trace(err)
	}

	manifest.Includes, err = expandAllLibsPaths(manifest.Includes, manifest.LibsHandled)
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

// ExpandManifestConds expands all "conds" in the dstManifest, but all cond
// "when" expressions are evaluated against the refManifest. Nested conds are
// not expanded: if there are some new conds left, a new refManifest should be
// computed by the caller, and ExpandManifestConds should be called again for
// each lib's manifest and for app's manifest.
//
// NOTE that although cond "when" expressions are evaluated against refManifest,
// expressions inside of the conditionally-applied manifest (like
// `${build_vars.FOO} bar`) are expanded against dstManifest. See README.md,
// Step 3 for details.
func ExpandManifestConds(
	dstManifest, refManifest *build.FWAppManifest, interp *interpreter.MosInterpreter,
) error {
	interp = interp.Copy()

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
			if err := extendManifest(dstManifest, dstManifest, cond.Apply, "", "", interp, &extendManifestOptions{
				skipFailedExpansions: true,
			}); err != nil {
				return errors.Trace(err)
			}
		}
	}

	return nil
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
	interp *interpreter.MosInterpreter, opts *extendManifestOptions,
) error {
	interp = interp.Copy()

	if opts == nil {
		opts = &extendManifestOptions{}
	}

	// Extend sources
	if !opts.skipSources {
		mMain.Sources = append(
			prependPaths(m1.Sources, m1Dir),
			prependPaths(m2.Sources, m2Dir)...,
		)
	}

	// Extend include paths
	mMain.Includes = append(
		prependPaths(m1.Includes, m1Dir),
		prependPaths(m2.Includes, m2Dir)...,
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

	var err error

	mMain.BuildVars, err = mergeMapsString(m1.BuildVars, m2.BuildVars, interp, opts.skipFailedExpansions)
	if err != nil {
		return errors.Trace(err)
	}

	mMain.CDefs, err = mergeMapsString(m1.CDefs, m2.CDefs, interp, opts.skipFailedExpansions)
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

type extendManifestOptions struct {
	skipSources          bool
	skipFailedExpansions bool
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
			subManifest.Includes = prependPaths(subManifest.Includes, dir)
			subManifest.Filesystem = prependPaths(subManifest.Filesystem, dir)
			subManifest.BinaryLibs = prependPaths(subManifest.BinaryLibs, dir)
			c.Apply = &subManifest
		}
		ret = append(ret, c)
	}
	return ret
}

// mergeMapsString merges two map[string]string into a new one; m2 takes
// precedence over m1. Values of m2 can contain expressions which are expanded
// against the given interp.
func mergeMapsString(
	m1, m2 map[string]string, interp *interpreter.MosInterpreter, skipFailed bool,
) (map[string]string, error) {
	bv := make(map[string]string)

	for k, v := range m1 {
		bv[k] = v
	}
	for k, v := range m2 {
		var err error
		bv[k], err = interpreter.ExpandVars(interp, v, skipFailed)
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

// resolvePaths takes a list of paths as they are in manifest, globs like
// []string{"*.c", "*.h"}, and converts those paths into paths to concrete
// existing files.
//
// There are three kinds of paths which can be present in the input srcPaths:
// - Globs, like "foo/bar/*.c". Those get expanded to the list of concrete files.
// - Paths to dirs. Those get appended all the given globs, and then treated
//   as the globs above
// - Paths to concrete files. Those stay unchanged.
func resolvePaths(srcPaths []string, globs []string) (files []string, dirs []string, err error) {
	var fileGlobs []string
	fileGlobs, dirs, err = globify(srcPaths, globs)
	if err != nil {
		return nil, nil, errors.Trace(err)
	}

	for _, g := range fileGlobs {
		matches, err := filepath.Glob(g)
		if err != nil {
			return nil, nil, errors.Trace(err)
		}

		files = append(files, matches...)
	}

	return files, dirs, nil
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

func getAllSupportedPlatforms(mosDir string) ([]string, error) {
	paths, err := filepath.Glob(filepath.Join(mosDir, "fw", "platforms", "*", "sdk.version"))
	if err != nil {
		return nil, errors.Trace(err)
	}

	ret := []string{}

	for _, p := range paths {
		p1, _ := filepath.Split(p)
		_, p2 := filepath.Split(p1[:len(p1)-1])
		ret = append(ret, p2)
	}

	sort.Strings(ret)

	return ret, nil
}
