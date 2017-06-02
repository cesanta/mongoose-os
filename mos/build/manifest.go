package build

import (
	"bytes"
	"encoding/json"

	"github.com/cesanta/errors"
)

// AppManifest contains the common app manifest fields
type AppManifest struct {
	Name    string `yaml:"name,omitempty" json:"name,omitempty"`
	Type    string `yaml:"type,omitempty" json:"type,omitempty"`
	Version string `yaml:"version,omitempty" json:"version,omitempty"`
	Summary string `yaml:"summary,omitempty" json:"summary,omitempty"`
}

// FWAppManifest is the app manifest for firmware apps
type FWAppManifest struct {
	AppManifest       `yaml:",inline"`
	Arch              string             `yaml:"arch" json:"arch,omitempty"`
	Author            string             `yaml:"author" json:"author,omitempty"`
	Description       string             `yaml:"description" json:"description,omitempty"`
	MongooseOsVersion string             `yaml:"mongoose_os_version" json:"mongoose_os_version,omitempty"`
	Sources           []string           `yaml:"sources" json:"sources,omitempty"`
	Filesystem        []string           `yaml:"filesystem" json:"filesystem,omitempty"`
	ExtraFiles        []string           `yaml:"extra_files" json:"extra_files,omitempty"`
	EnableJavascript  bool               `yaml:"enable_javascript,omitempty" json:"enable_javascript,omitempty"`
	SkeletonVersion   string             `yaml:"skeleton_version" json:"skeleton_version,omitempty"`
	FFISymbols        []string           `yaml:"ffi_symbols" json:"ffi_symbols,omitempty"`
	Modules           []SWModule         `yaml:"modules,omitempty" json:"modules,omitempty"`
	Libs              []SWModule         `yaml:"libs,omitempty" json:"libs,omitempty"`
	ConfigSchema      []ConfigSchemaItem `yaml:"config_schema" json:"config_schema,omitempty"`
	BuildVars         map[string]string  `yaml:"build_vars" json:"build_vars,omitempty"`
	Tags              []string           `yaml:"tags" json:"tags,omitempty"`

	// Deps are names of the libraries which need to be initialized before the
	// application. The user doesn't have to set this field manually, it's set
	// automatically during libs "expansion" (see Libs above)
	Deps []string `yaml:"deps" json:"deps,omitempty"`
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
