package main

import (
	"bytes"
	"fmt"
	"io/ioutil"
	"net/http"
	"os"
	"path/filepath"

	"golang.org/x/net/context"

	"cesanta.com/common/go/ourutil"
	"cesanta.com/mos/build/archive"
	moscommon "cesanta.com/mos/common"
	"cesanta.com/mos/dev"
	"github.com/cesanta/errors"
)

func initFW(ctx context.Context, devConn *dev.DevConn) error {

	// Make sure current directory is empty
	empty, err := isDirEmpty(".")
	if err != nil {
		return errors.Trace(err)
	}

	if !empty {
		if !*force {
			return errors.Errorf("refuse to init source tree in non-empty directory")
		}
	}

	// Download zip data
	fmt.Println("Downloading empty app...")

	url := fmt.Sprintf("https://github.com/mongoose-os-apps/empty/archive/master.zip")
	resp, err := http.Get(url)
	if err != nil {
		return errors.Trace(err)
	}

	if resp.StatusCode != http.StatusOK {
		return errors.Errorf("bad response %d on %q", resp.StatusCode, url)
	}

	defer resp.Body.Close()

	// We have to create a new reader since resp.Body doesn't implement ReadAt
	// which is needed for unzip.UnzipInto.
	zipData, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return errors.Trace(err)
	}

	zipReader := bytes.NewReader(zipData)

	fmt.Println("Unpacking...")
	if err := archive.UnzipInto(zipReader, zipReader.Size(), ".", 1); err != nil {
		return errors.Trace(err)
	}

	// Remove LICENSE file, ignore any errors
	os.RemoveAll(filepath.Join(".", "LICENSE"))

	// If platform was provided, update yaml.
	// Note that we shouldn't unmarshal yaml, set platform and marshal it back,
	// because it strips all comments and uglifies mos.yml in other ways.
	// Instead, we just insert "platform: foo" as a first line.
	if *platform != "" {
		ourutil.Reportf("Setting platform %q...", *platform)
		manifestFilename := moscommon.GetManifestFilePath(".")

		manifestData, err := ioutil.ReadFile(manifestFilename)
		if err != nil {
			return errors.Trace(err)
		}

		manifestString := fmt.Sprintf("platform: %s\n%s", *platform, string(manifestData))

		err = ioutil.WriteFile(manifestFilename, []byte(manifestString), 0644)
		if err != nil {
			return errors.Trace(err)
		}
	}

	return nil
}

func isDirEmpty(dirName string) (bool, error) {
	dir, err := os.Open(".")
	if err != nil {
		return false, errors.Annotatef(err, "opening %q", dirName)
	}
	defer dir.Close()

	entries, err := dir.Readdir(-1)
	if err != nil {
		return false, errors.Annotatef(err, "reading contents of %q", dirName)
	}

	return len(entries) == 0, nil
}
