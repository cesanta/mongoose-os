package build

import (
	"bytes"
	"encoding/json"

	"github.com/cesanta/errors"
)

const (
	AppTypeApp = "app"
	AppTypeLib = "lib"
)

// ManifestCond represents a conditional addition to the manifest.
type ManifestCond struct {
	// The whole cond structure is considered if only When expression evaluates
	// to true (see EvaluateExprBool())
	When string `yaml:"when,omitempty" json:"when,omitempty"`

	// If non-nil, outer manifest gets extended with this one.
	Apply *FWAppManifest `yaml:"apply,omitempty" json:"apply,omitempty"`

	// If not an empty string, results in an error being returned.
	Error string `yaml:"error,omitempty" json:"error,omitempty"`
}

// AppManifest contains the common app manifest fields
type AppManifest struct {
	Name    string `yaml:"name,omitempty" json:"name"`
	Type    string `yaml:"type,omitempty" json:"type"`
	Version string `yaml:"version,omitempty" json:"version"`
	Summary string `yaml:"summary,omitempty" json:"summary"`
}

type FWAppManifestLibHandled struct {
	Name     string         `yaml:"name,omitempty" json:"name"`
	Path     string         `yaml:"path,omitempty" json:"path"`
	Deps     []string       `yaml:"deps,omitempty" json:"deps"`
	Manifest *FWAppManifest `yaml:"manifest,omitempty" json:"manifest"`
	Sources  []string       `yaml:"sources,omitempty" json:"sources"`
}

// FWAppManifest is the app manifest for firmware apps
type FWAppManifest struct {
	AppManifest `yaml:",inline"`
	// arch was deprecated at 2017/08/15 and should eventually be removed.
	ArchOld      string             `yaml:"arch,omitempty" json:"arch"`
	Platform     string             `yaml:"platform,omitempty" json:"platform"`
	Platforms    []string           `yaml:"platforms,omitempty" json:"platforms"`
	Author       string             `yaml:"author,omitempty" json:"author"`
	Description  string             `yaml:"description,omitempty" json:"description"`
	Sources      []string           `yaml:"sources,omitempty" json:"sources"`
	Includes     []string           `yaml:"includes,omitempty" json:"includes"`
	Filesystem   []string           `yaml:"filesystem,omitempty" json:"filesystem"`
	BinaryLibs   []string           `yaml:"binary_libs,omitempty" json:"binary_libs"`
	ExtraFiles   []string           `yaml:"extra_files,omitempty" json:"extra_files"`
	FFISymbols   []string           `yaml:"ffi_symbols,omitempty" json:"ffi_symbols"`
	Tests        []string           `yaml:"tests,omitempty" json:"tests"`
	Modules      []SWModule         `yaml:"modules,omitempty" json:"modules"`
	Libs         []SWModule         `yaml:"libs,omitempty" json:"libs"`
	InitAfter    []string           `yaml:"init_after,omitempty" json:"init_after"`
	ConfigSchema []ConfigSchemaItem `yaml:"config_schema,omitempty" json:"config_schema"`
	BuildVars    map[string]string  `yaml:"build_vars,omitempty" json:"build_vars"`
	CFlags       []string           `yaml:"cflags,omitempty" json:"cflags"`
	CXXFlags     []string           `yaml:"cxxflags,omitempty" json:"cxxflags"`
	CDefs        map[string]string  `yaml:"cdefs,omitempty" json:"cdefs"`
	Tags         []string           `yaml:"tags,omitempty" json:"tags"`

	LibsVersion       string `yaml:"libs_version,omitempty" json:"libs_version"`
	ModulesVersion    string `yaml:"modules_version,omitempty" json:"modules_version"`
	MongooseOsVersion string `yaml:"mongoose_os_version,omitempty" json:"mongoose_os_version"`

	Conds []ManifestCond `yaml:"conds,omitempty" json:"conds"`

	ManifestVersion string `yaml:"manifest_version,omitempty" json:"manifest_version"`
	// SkeletonVersion is deprecated since 05.06.2017 (all existing manifests
	// are updated at 18.08.2017)
	SkeletonVersion string `yaml:"skeleton_version,omitempty" json:"skeleton_version"`

	// are names of the libraries which need to be initialized before the
	// application. The user doesn't have to set this field manually, it's set
	// automatically during libs "expansion" (see Libs above)
	LibsHandled []FWAppManifestLibHandled `yaml:"libs_handled,omitempty" json:"libs_handled"`
}

// ConfigSchemaItem represents a single config schema item, like this:
//
//     ["foo.bar", "default value"]
//
// or this:
//
//     ["foo.bar", "o", {"title": "Some title"}]
//
// Unfortunately we can't just use []interface{}, because
// {"title": "Some title"} gets unmarshaled as map[interface{}]interface{},
// which is an invalid type for JSON, so we have to create a custom type which
// implements json.Marshaler interface.
type ConfigSchemaItem []interface{}

func (c ConfigSchemaItem) MarshalJSON() ([]byte, error) {
	var data bytes.Buffer

	if _, err := data.WriteString("["); err != nil {
		return nil, errors.Trace(err)
	}

	for idx, v := range c {

		if idx > 0 {
			if _, err := data.WriteString(","); err != nil {
				return nil, errors.Trace(err)
			}
		}

		switch v2 := v.(type) {
		case string, bool, float64, int:
			// Primitives are marshaled as is
			d, err := json.Marshal(v2)
			if err != nil {
				return nil, errors.Trace(err)
			}

			data.Write(d)

		case map[interface{}]interface{}:
			// map[interface{}]interface{} needs to be converted to
			// map[string]interface{} before marshaling
			vjson := map[string]interface{}{}

			for k, v := range v2 {
				kstr, ok := k.(string)
				if !ok {
					return nil, errors.Errorf("invalid key: %v (must be a string)", k)
				}

				vjson[kstr] = v
			}

			d, err := json.Marshal(vjson)
			if err != nil {
				return nil, errors.Trace(err)
			}

			data.Write(d)

		default:
			return nil, errors.Errorf("invalid schema value: %v (type: %T)", v, v)
		}
	}

	if _, err := data.WriteString("]"); err != nil {
		return nil, errors.Trace(err)
	}

	return data.Bytes(), nil
}
