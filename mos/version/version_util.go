package version

import (
	"regexp"

	moscommon "cesanta.com/mos/common"
)

type VersionJson struct {
	BuildId        string `json:"build_id"`
	BuildTimestamp string `json:"build_timestamp"`
	BuildVersion   string `json:"build_version"`
}

var (
	regexpVersionNumber = regexp.MustCompile(`^\d+\.[0-9.]*$`)
	regexpBuildId       = regexp.MustCompile(
		`^(?P<datetime>[^/]+)\/(?P<symbolic>[^@]+)\@(?P<hash>.+)$`,
	)
)

// GetMosVersion checks symbolic part of the build id, and if it looks like a
// version number (i.e. starts with a digit and contains only digits and dots),
// then returns it; otherwise returns "latest".
func GetMosVersion() string {
	ver, err := GetMosVersionByBuildId(BuildId)
	if err != nil {
		panic(err.Error())
	}

	return ver
}

func GetMosVersionByBuildId(buildId string) (string, error) {
	matches := regexpBuildId.FindStringSubmatch(buildId)
	if matches == nil {
		// We failed to parse build id; it typically happens when it looks like
		// "20170721-002340/???". For now, assume latest
		// TODO(dfrank): use Version for that
		return "latest", nil
	}

	symbolic := matches[2]

	if LooksLikeVersionNumber(symbolic) {
		return symbolic, nil
	}

	return "latest", nil
}

// GetMosVersionSuffix returns an empty string if mos version is "latest";
// otherwise returns the mos version prepended with a dash, like "-1.6".
func GetMosVersionSuffix() string {
	return moscommon.GetVersionSuffix(GetMosVersion())
}

func LooksLikeVersionNumber(s string) bool {
	return regexpVersionNumber.MatchString(s)
}
