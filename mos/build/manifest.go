package build

// AppManifest contains the common app manifest fields
type AppManifest struct {
	Name    string `yaml:"name,omitempty"`
	Type    string `yaml:"type,omitempty"`
	Version string `yaml:"version,omitempty"`
	Summary string `yaml:"summary,omitempty"`
}

// FWAppManifest is the app manifest for firmware apps
type FWAppManifest struct {
	AppManifest       `yaml:",inline"`
	Arch              string            `yaml:"arch"`
	MongooseOsVersion string            `yaml:"mongoose_os_version"`
	Sources           []string          `yaml:"sources"`
	Filesystem        []string          `yaml:"filesystem"`
	ExtraFiles        []string          `yaml:"extra_files"`
	EnableJavascript  bool              `yaml:"enable_javascript,omitempty"`
	SkeletonVersion   string            `yaml:"skeleton_version"`
	FFISymbols        []string          `yaml:"ffi_symbols"`
	Modules           []SWModule        `yaml:"modules,omitempty"`
	Libs              []SWModule        `yaml:"libs,omitempty"`
	BuildVars         map[string]string `yaml:"build_vars"`
}
