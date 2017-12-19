package version

//go:generate sh -c "../../common/tools/fw_meta.py gen_build_info --version=`[ -f version ] && cat version` --tag_as_version=true --id=`[ -f build_id ] && cat build_id` --go_output=version.go  --json_output=version.json"

import (
	"regexp"
	"strings"

	"cesanta.com/common/go/ourutil"
	moscommon "cesanta.com/mos/common"
)

type VersionJson struct {
	BuildId        string `json:"build_id"`
	BuildTimestamp string `json:"build_timestamp"`
	BuildVersion   string `json:"build_version"`
}

const (
	brewDistrName = "brew"
)

var (
	regexpVersionNumber = regexp.MustCompile(`^\d+\.[0-9.]*$`)
	regexpBuildId       = regexp.MustCompile(
		`^(?P<datetime>[^/]+)\/(?P<symbolic>[^@]+)\@(?P<hash>.+)$`,
	)
	regexpBuildIdDistr = regexp.MustCompile(
		`^(?P<version>[^+]+)\+(?P<hash>[^~]+)\~(?P<distr>.+)$`,
	)

	debianDistrNames = []string{"xenial", "zesty", "artful"}
)

// GetMosVersion returns this binary's version, or "latest" if it's not a release build.
func GetMosVersion() string {
	if LooksLikeVersionNumber(Version) {
		return Version
	}
	return GetMosVersionFromBuildId(BuildId)
}

func GetMosVersionFromBuildId(buildId string) string {
	// E.g.: 20170804-194202/1.234@e1eb86a3 => 1.234
	matches := regexpBuildId.FindStringSubmatch(buildId)
	if matches != nil && LooksLikeVersionNumber(matches[2]) {
		return matches[2]
	}
	return "latest"
}

// GetMosVersionSuffix returns an empty string if mos version is "latest";
// otherwise returns the mos version prepended with a dash, like "-1.6".
func GetMosVersionSuffix() string {
	return moscommon.GetVersionSuffix(GetMosVersion())
}

func LooksLikeVersionNumber(s string) bool {
	return regexpVersionNumber.MatchString(s)
}

func LooksLikeDebianBuildId(s string) bool {
	return GetDebianPackageName(s) != ""
}

func LooksLikeBrewBuildId(s string) bool {
	matches := ourutil.FindNamedSubmatches(regexpBuildIdDistr, s)
	return matches != nil && matches["distr"] == brewDistrName
}

// GetDebianPackageName parses given build id string, and if it looks like a
// debian build id, returns either "mos-latest" or "mos". Otherwise, returns
// an empty string.
func GetDebianPackageName(buildId string) string {
	matches := ourutil.FindNamedSubmatches(regexpBuildIdDistr, buildId)
	if matches != nil {
		for _, v := range debianDistrNames {
			if strings.HasPrefix(matches["distr"], v) {
				if LooksLikeVersionNumber(matches["version"]) {
					return "mos"
				} else {
					return "mos-latest"
				}
			}
		}

		// Some non-debian distro name
		return ""
	} else {
		// Doesn't look like distro build id
		return ""
	}
}
