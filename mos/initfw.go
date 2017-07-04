package main

import (
	"bytes"
	"context"
	"fmt"
	"io/ioutil"
	"net/http"
	"os"
	"path/filepath"

	"cesanta.com/mos/build"
	"cesanta.com/mos/build/archive"
	moscommon "cesanta.com/mos/common"
	"cesanta.com/mos/dev"
	"github.com/cesanta/errors"
	yaml "gopkg.in/yaml.v2"
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

	// If arch was provided, update yaml
	if *arch != "" {
		fmt.Printf("Setting arch %q...\n", *arch)
		manifestFilename := moscommon.GetManifestFilePath(".")

		manifestData, err := ioutil.ReadFile(manifestFilename)
		if err != nil {
			return errors.Trace(err)
		}

		var manifest build.FWAppManifest
		if err := yaml.Unmarshal(manifestData, &manifest); err != nil {
			return errors.Trace(err)
		}

		manifest.Arch = *arch

		manifestData, err = yaml.Marshal(&manifest)
		if err != nil {
			return errors.Trace(err)
		}

		err = ioutil.WriteFile(manifestFilename, manifestData, 0644)
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
