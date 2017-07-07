package main

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"net/http"
	"os"
	"path"
	"runtime"

	"cesanta.com/mos/cfgfile"
	"github.com/cesanta/errors"
	"github.com/kardianos/osext"
	flag "github.com/spf13/pflag"

	"cesanta.com/mos/dev"
)

type versionJson struct {
	BuildId        string `json:"build_id"`
	BuildTimestamp string `json:"build_timestamp"`
	BuildVersion   string `json:"build_version"`
}

func getMosURL(p string) string {
	return "https://" + path.Join(
		fmt.Sprintf("mongoose-os.com/downloads/mos%s", cfgfile.GetMosVersionSuffix()),
		p,
	)
}

func getServerMosVersion() (*versionJson, error) {
	versionUrl := getMosURL("version.json")
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
	if len(args) >= 2 {
		// mos_version is given: need to update config
		newMosVersion := args[1]

		if cfgfile.MosConfigCurrent.MosVersion != newMosVersion {
			reportf("Changing update channel from %q to %q",
				cfgfile.MosConfigCurrent.MosVersion, newMosVersion,
			)

			cfgfile.MosConfigCurrent.MosVersion = newMosVersion
			if err := cfgfile.MosConfigSave(); err != nil {
				return errors.Trace(err)
			}
		}
	}

	var mosUrls = map[string]string{
		"windows": getMosURL("win/mos.exe"),
		"linux":   getMosURL("linux/mos"),
		"darwin":  getMosURL("mac/mos"),
	}

	// Check the available version on the server
	serverVersion, err := getServerMosVersion()
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
