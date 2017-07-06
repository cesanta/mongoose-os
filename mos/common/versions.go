package moscommon

import "fmt"

// GetMosVersionSuffix returns an appropriate suffix depending on the given
// version string: for "latest" or "master" or "" it returns an empty string;
// for any other version it returns the version prepended with a dash, e.g.
// "-1.5".
func GetVersionSuffix(version string) string {
	if version == "master" || version == "latest" || version == "" {
		return ""
	}
	return fmt.Sprintf("-%s", version)
}
