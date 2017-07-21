package ourfilepath

import (
	"path/filepath"
	"strings"
)

// GetFirstPathComponent returns first component of the given path. If given
// an empty string, it's returned back.
func GetFirstPathComponent(p string) string {
	parts := strings.Split(p, string(filepath.Separator))
	if len(parts) > 0 {
		return parts[0]
	}

	return ""
}
