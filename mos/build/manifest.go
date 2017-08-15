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
	Name     string         `yaml:"name" json:"name"`
	Path     string         `yaml:"path" json:"path"`
	Deps     []string       `yaml:"deps" json:"deps"`
	Manifest *FWAppManifest `yaml:"manifest,omitempty", json:"manifest"`
}

// FWAppManifest is the app manifest for firmware apps
type FWAppManifest struct {
	AppManifest `yaml:",inline"`
	// arch was deprecated at 2017/08/15 and should eventually be removed.
	ArchOld           string             `yaml:"arch" json:"arch"`
	Platform          string             `yaml:"platform" json:"platform"`
	Platforms         []string           `yaml:"platforms" json:"platforms"`
	Author            string             `yaml:"author" json:"author"`
	Description       string             `yaml:"description" json:"description"`
	MongooseOsVersion string             `yaml:"mongoose_os_version" json:"mongoose_os_version"`
	Sources           []string           `yaml:"sources" json:"sources"`
	Filesystem        []string           `yaml:"filesystem" json:"filesystem"`
	BinaryLibs        []string           `yaml:"binary_libs" json:"binary_libs"`
	ExtraFiles        []string           `yaml:"extra_files" json:"extra_files"`
	EnableJavascript  bool               `yaml:"enable_javascript" json:"enable_javascript"`
	FFISymbols        []string           `yaml:"ffi_symbols" json:"ffi_symbols"`
	Modules           []SWModule         `yaml:"modules,omitempty" json:"modules"`
	Libs              []SWModule         `yaml:"libs,omitempty" json:"libs"`
	ConfigSchema      []ConfigSchemaItem `yaml:"config_schema" json:"config_schema"`
	BuildVars         map[string]string  `yaml:"build_vars" json:"build_vars"`
	CFlags            []string           `yaml:"cflags" json:"cflags"`
	CXXFlags          []string           `yaml:"cxxflags" json:"cxxflags"`
	CDefs             map[string]string  `yaml:"cdefs" json:"cdefs"`
	Tags              []string           `yaml:"tags" json:"tags"`

	LibsVersion    string `yaml:"libs_version" json:"libs_version"`
	ModulesVersion string `yaml:"modules_version" json:"modules_version"`

	Conds []ManifestCond `yaml:"conds" json:"conds"`

	ManifestVersion string `yaml:"manifest_version" json:"manifest_version"`
	// SkeletonVersion is deprecated since 05.06.2017
	SkeletonVersion string `yaml:"skeleton_version" json:"skeleton_version"`

	// Old form of LibsHandled, deprecated since 03.06.2017
	// TODO: remove
	Deps []string `yaml:"deps" json:"deps"`

	// are names of the libraries which need to be initialized before the
	// application. The user doesn't have to set this field manually, it's set
	// automatically during libs "expansion" (see Libs above)
	LibsHandled []FWAppManifestLibHandled `yaml:"libs_handled" json:"libs_handled"`
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
