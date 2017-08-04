package moscommon

import "strings"

// GetVersionSuffix returns suffix like "-1.5" or "-latest". See
// GetVersionSuffixTpl.
func GetVersionSuffix(version string) string {
	return GetVersionSuffixTpl(version, "-${version}")
}

// GetVersionSuffixTpl returns given template with "${version}" placeholder
// replaced with the actual given version. If given version is "master" or
// an empty string, "latest" is used instead.
func GetVersionSuffixTpl(version, template string) string {
	if version == "master" || version == "" {
		version = "latest"
	}
	return strings.Replace(template, "${version}", version, -1)
}
