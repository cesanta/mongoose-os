package build

import (
	"bytes"
	"encoding/json"
	"regexp"

	"github.com/cesanta/errors"
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
	Name    string `yaml:"name,omitempty" json:"name,omitempty"`
	Type    string `yaml:"type,omitempty" json:"type,omitempty"`
	Version string `yaml:"version,omitempty" json:"version,omitempty"`
	Summary string `yaml:"summary,omitempty" json:"summary,omitempty"`
}

type FWAppManifestLibHandled struct {
	Name string   `yaml:"name" json:"name,omitempty"`
	Path string   `yaml:"path" json:"path,omitempty"`
	Deps []string `yaml:"deps" json:"deps"`
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
	FFISymbols        []string           `yaml:"ffi_symbols" json:"ffi_symbols,omitempty"`
	Modules           []SWModule         `yaml:"modules,omitempty" json:"modules,omitempty"`
	Libs              []SWModule         `yaml:"libs,omitempty" json:"libs,omitempty"`
	ConfigSchema      []ConfigSchemaItem `yaml:"config_schema" json:"config_schema,omitempty"`
	BuildVars         map[string]string  `yaml:"build_vars" json:"build_vars,omitempty"`
	CFlags            []string           `yaml:"cflags" json:"cflags,omitempty"`
	CXXFlags          []string           `yaml:"cxxflags" json:"cxxflags,omitempty"`
	CDefs             map[string]string  `yaml:"cdefs" json:"cdefs,omitempty"`
	Tags              []string           `yaml:"tags" json:"tags,omitempty"`

	Conds []ManifestCond `yaml:"conds" json:"conds,omitempty"`

	ManifestVersion string `yaml:"manifest_version" json:"manifest_version,omitempty"`
	// SkeletonVersion is deprecated since 05.06.2017
	SkeletonVersion string `yaml:"skeleton_version" json:"skeleton_version,omitempty"`

	// Old form of LibsHandled, deprecated since 03.06.2017
	// TODO: remove
	Deps []string `yaml:"deps" json:"deps,omitempty"`

	// are names of the libraries which need to be initialized before the
	// application. The user doesn't have to set this field manually, it's set
	// automatically during libs "expansion" (see Libs above)
	LibsHandled []FWAppManifestLibHandled `yaml:"libs_handled" json:"libs_handled,omitempty"`
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

// EvaluateExprBool can evaluate expressions of the following form:
//
//   build_vars.FOO == "bar"
//   build_vars.FOO != "bar"
//
// In the future it will be refactored into a proper expression parsing and
// evaluation, but so far it's a quick hack which solves the problem at hand.
func (m *FWAppManifest) EvaluateExprBool(expr string) (bool, error) {
	re := regexp.MustCompile(
		`^\s*build_vars\.(?P<name>[a-zA-Z0-9_]+)\s*(?P<op>\S+)\s*\"(?P<value>[^"]*)\"`,
	)

	matches := re.FindStringSubmatch(expr)
	if matches == nil {
		return false, errors.Errorf("%q is not a valid expression", expr)
	}

	name := matches[1]
	op := matches[2]
	val := matches[3]

	switch op {
	case "==":
		return m.BuildVars[name] == val, nil
	case "!=":
		return m.BuildVars[name] != val, nil
	default:
		return false, errors.Errorf("%q is not a valid operation", op)
	}
}
