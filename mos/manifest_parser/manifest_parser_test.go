package manifest_parser

import (
	"bytes"
	"io/ioutil"
	"log"
	"os"
	"path"
	"path/filepath"
	"strings"
	"testing"

	yaml "gopkg.in/yaml.v2"

	"cesanta.com/mos/build"
	moscommon "cesanta.com/mos/common"
	"cesanta.com/mos/interpreter"
	"github.com/cesanta/errors"
)

const (
	testManifestsDir   = "test_manifests"
	appDir             = "app"
	expectedDir        = "expected"
	finalManifestName  = "mos_final.yml"
	testDescriptorName = "test_desc.yml"

	testPrefix    = "test_"
	testSetPrefix = "testset_"

	manifestParserRootPlaceholder = "__MANIFEST_PARSER_ROOT__"
	appRootPlaceholder            = "__APP_ROOT__"
)

var (
	manifestParserRoot = ""
)

type TestDescr struct {
	PreferBinaryLibs bool `yaml:"prefer_binary_libs"`
}

func init() {
	var err error
	manifestParserRoot, err = filepath.Abs(".")
	if err != nil {
		log.Fatal(err)
	}
}

func TestParser(t *testing.T) {
	//if err := singleManifestTest(t, "test_manifests/testset_02_conds_switch_without_arch_manifests/test_01_app_doesnt_override"); err != nil {
	//t.Fatal(errors.ErrorStack(err))
	//}

	ok := handleTestSet(t, testManifestsDir)

	if !ok {
		t.Fatal("failing due the errors above")
	}
}

func handleTestSet(t *testing.T, testSetPath string) bool {
	files, err := ioutil.ReadDir(testSetPath)
	if err != nil {
		t.Fatal(errors.ErrorStack(err))
	}

	ok := true

	for _, f := range files {
		if strings.HasPrefix(f.Name(), testPrefix) {
			if err := singleManifestTest(t, filepath.Join(testSetPath, f.Name())); err != nil {
				t.Log(errors.ErrorStack(err))
				ok = false
			}
		} else if strings.HasPrefix(f.Name(), testSetPrefix) {
			if !handleTestSet(t, filepath.Join(testSetPath, f.Name())) {
				ok = false
			}
		}
	}

	return ok
}

func singleManifestTest(t *testing.T, appPath string) error {
	// Create test descriptor with default values
	descr := TestDescr{}

	// If test descriptor exists for the current test app, read it
	descrFilename := filepath.Join(appPath, testDescriptorName)
	if _, err := os.Stat(descrFilename); err == nil {
		descrData, err := ioutil.ReadFile(descrFilename)
		if err != nil {
			return errors.Trace(err)
		}

		if err := yaml.Unmarshal(descrData, &descr); err != nil {
			return errors.Trace(err)
		}
	}

	platformFiles, err := ioutil.ReadDir(filepath.Join(appPath, expectedDir))
	if err != nil {
		return errors.Trace(err)
	}

	platforms := []string{}

	for _, v := range platformFiles {
		platforms = append(platforms, v.Name())
	}

	for _, platform := range platforms {
		logWriter := &bytes.Buffer{}
		interp := interpreter.NewInterpreter(newMosVars())

		t.Logf("testing %q for %q", appPath, platform)

		manifest, _, err := ReadManifestFinal(
			filepath.Join(appPath, appDir), platform, logWriter, interp,
			&ReadManifestCallbacks{ComponentProvider: &compProviderTest{}}, true, descr.PreferBinaryLibs,
		)

		if err != nil {
			return errors.Trace(err)
		}

		data, err := yaml.Marshal(manifest)
		if err != nil {
			return errors.Trace(err)
		}

		data = addPlaceholders(data, appPath)

		expectedFilename := filepath.Join(appPath, expectedDir, platform, finalManifestName)

		expectedData, err := ioutil.ReadFile(expectedFilename)
		if err != nil {
			return errors.Trace(err)
		}

		if bytes.Compare(expectedData, data) != 0 {
			buildDir := moscommon.GetBuildDir(filepath.Join(appPath, appDir))
			os.MkdirAll(buildDir, 0777)
			actualFilename := filepath.Join(buildDir, "mos_final_actual.yml")
			ioutil.WriteFile(actualFilename, data, 0644)
			return errors.Errorf("actual manifest %q doesn't match %q", actualFilename, expectedFilename)
		}
	}

	return nil
}

type compProviderTest struct{}

func (lpt *compProviderTest) GetLibLocalPath(
	m *build.SWModule, rootAppDir, libsDefVersion string,
) (string, error) {
	appName, err := m.GetName()
	if err != nil {
		return "", errors.Trace(err)
	}

	return filepath.Join(rootAppDir, "..", "libs", appName), nil
}

func (lpt *compProviderTest) GetModuleLocalPath(
	m *build.SWModule, rootAppDir, modulesDefVersion string,
) (string, error) {
	appName, err := m.GetName()
	if err != nil {
		return "", errors.Trace(err)
	}

	return filepath.Join(rootAppDir, "..", "modules", appName), nil
}

func (lpt *compProviderTest) GetMongooseOSLocalPath(
	rootAppDir, modulesDefVersion string,
) (string, error) {
	// This one doesn't actually exist, but we don't care
	return filepath.Join(rootAppDir, "..", "mongoose-os"), nil
}

func newMosVars() *interpreter.MosVars {
	ret := interpreter.NewMosVars()
	ret.SetVar(interpreter.GetMVarNameMosVersion(), "0.01")
	return ret
}

func addPlaceholders(data []byte, appPath string) []byte {
	data = []byte(strings.Replace(
		string(data),
		path.Join(manifestParserRoot, appPath),
		appRootPlaceholder,
		-1,
	))

	data = []byte(strings.Replace(
		string(data), manifestParserRoot, manifestParserRootPlaceholder, -1,
	))

	return data
}
