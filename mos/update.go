package main

import (
	"encoding/json"
	"fmt"
	"golang.org/x/net/context"
	"io"
	"io/ioutil"
	"net/http"
	"os"
	"path"
	"regexp"
	"runtime"

	moscommon "cesanta.com/mos/common"
	"github.com/cesanta/errors"
	"github.com/kardianos/osext"
	flag "github.com/spf13/pflag"

	"cesanta.com/mos/dev"
)

var (
	regexpVersionNumber = regexp.MustCompile(`^\d[0-9.]*$`)
	regexpBuildId       = regexp.MustCompile(
		`^(?P<datetime>[^/]+)\/(?P<symbolic>[^@]+)\@(?P<hash>.+)$`,
	)
)

type versionJson struct {
	BuildId        string `json:"build_id"`
	BuildTimestamp string `json:"build_timestamp"`
	BuildVersion   string `json:"build_version"`
}

// mosVersion can be either exact mos version like "1.6", or update channel
// like "latest" or "release".
func getMosURL(p, mosVersion string) string {
	return "https://" + path.Join(
		fmt.Sprintf("mongoose-os.com/downloads/mos%s", moscommon.GetVersionSuffix(mosVersion)),
		p,
	)
}

// mosVersion can be either exact mos version like "1.6", or update channel
// like "latest" or "release".
func getServerMosVersion(mosVersion string) (*versionJson, error) {
	versionUrl := getMosURL("version.json", mosVersion)
	resp, err := http.Get(versionUrl)
	if err != nil {
		return nil, errors.Trace(err)
	}
	if resp.StatusCode != http.StatusOK {
		return nil, errors.Errorf("got %d when accessing %s", resp.StatusCode, versionUrl)
	}

	defer resp.Body.Close()

	var serverVersion versionJson

	decoder := json.NewDecoder(resp.Body)
	decoder.Decode(&serverVersion)

	return &serverVersion, nil
}

func update(ctx context.Context, devConn *dev.DevConn) error {
	args := flag.Args()

	// updChannel and newUpdChannel are needed for the logging, so that it's
	// clear for the user which update channel is used
	updChannel := getUpdateChannel()
	newUpdChannel := updChannel

	// newMosVersion is the version which will be fetched from the server;
	// by default it's equal to the current update channel.
	newMosVersion := updChannel

	if len(args) >= 2 {
		// Desired mos version is given
		newMosVersion = args[1]
		newUpdChannel = getUpdateChannelByMosVersion(newMosVersion)
	}

	if updChannel != newUpdChannel {
		reportf("Changing update channel from %q to %q", updChannel, newUpdChannel)
	} else {
		reportf("Update channel: %s", updChannel)
	}

	var mosUrls = map[string]string{
		"windows": getMosURL("win/mos.exe", newMosVersion),
		"linux":   getMosURL("linux/mos", newMosVersion),
		"darwin":  getMosURL("mac/mos", newMosVersion),
	}

	// Check the available version on the server
	serverVersion, err := getServerMosVersion(newMosVersion)
	if err != nil {
		return errors.Trace(err)
	}

	if serverVersion.BuildId != BuildId {
		// Versions are different, perform update
		reportf("Current version: %s, available version: %s.",
			BuildId, serverVersion.BuildId,
		)

		// Determine the right URL for the current platform
		mosUrl, ok := mosUrls[runtime.GOOS]
		if !ok {
			keys := make([]string, len(mosUrls))

			i := 0
			for k := range mosUrls {
				keys[i] = k
				i++
			}

			return errors.Errorf("unsupported OS: %s (supported values are: %v)",
				runtime.GOOS, keys,
			)
		}

		// Create temp file to save downloaded data into
		// (we should create it in the same dir as the executable to be updated,
		// just in case /tmp and executable are on different devices)
		executableDir, err := osext.ExecutableFolder()
		if err != nil {
			return errors.Trace(err)
		}

		tmpfile, err := ioutil.TempFile(executableDir, "mos_update_")
		if err != nil {
			return errors.Trace(err)
		}
		defer tmpfile.Close()

		// Fetch data from the server and save it into the temp file
		resp, err := http.Get(mosUrl)
		if err != nil {
			return errors.Trace(err)
		}
		defer resp.Body.Close()

		reportf("Downloading from %s...", mosUrl)
		n, err := io.Copy(tmpfile, resp.Body)
		if err != nil {
			return errors.Trace(err)
		}

		// Check saved length
		if n != resp.ContentLength {
			return errors.Errorf("expected to write %d bytes, %d written",
				resp.ContentLength, n,
			)
		}
		tmpfile.Close()

		// Determine names for the executable and backup
		executable, err := osext.Executable()
		if err != nil {
			return errors.Trace(err)
		}

		bak := fmt.Sprintf("%s.bak", executable)

		reportf("Renaming old binary as %s...", bak)
		if err := os.Rename(executable, bak); err != nil {
			return errors.Trace(err)
		}

		reportf("Saving new binary as %s...", executable)
		if err := os.Rename(tmpfile.Name(), executable); err != nil {
			return errors.Trace(err)
		}

		// Make sure the new binary is, indeed, executable
		if err := os.Chmod(executable, 0755); err != nil {
			return errors.Trace(err)
		}

		reportf("Done.")
	} else {
		reportf("Up to date.")
	}

	return nil
}

// getMosVersion checks symbolic part of the build id, and if it looks like a
// version number (i.e. starts with a digit and contains only digits and dots),
// then returns it; otherwise returns "latest".
func getMosVersion() string {
	ver, err := getMosVersionByBuildId(BuildId)
	if err != nil {
		panic(err.Error())
	}

	return ver
}

func getMosVersionByBuildId(buildId string) (string, error) {
	matches := regexpBuildId.FindStringSubmatch(buildId)
	if matches == nil {
		// We failed to parse build id; it typically happens when it looks like
		// "20170721-002340/???". For now, assume latest
		// TODO(dfrank): use Version for that
		return "latest", nil
	}

	symbolic := matches[2]

	if regexpVersionNumber.MatchString(symbolic) {
		return symbolic, nil
	}

	return "latest", nil
}

// getMosVersionSuffix returns an empty string if mos version is "latest";
// otherwise returns the mos version prepended with a dash, like "-1.6".
func getMosVersionSuffix() string {
	return moscommon.GetVersionSuffix(getMosVersion())
}

// getUpdateChannel returns update channel (either "latest" or "release")
// depending on current mos version.
func getUpdateChannel() string {
	return getUpdateChannelByMosVersion(getMosVersion())
}

// getUpdateChannelByMosVersion returns update channel (either "latest" or
// "release") depending on the given mos version.
func getUpdateChannelByMosVersion(mosVersion string) string {
	if mosVersion == "master" || mosVersion == "latest" {
		return "latest"
	}
	return "release"
}

// getUpdateChannelSuffix returns an empty string if update channel is
// "latest"; otherwise returns "-release".
func getUpdateChannelSuffix() string {
	return moscommon.GetVersionSuffix(getUpdateChannel())
}
