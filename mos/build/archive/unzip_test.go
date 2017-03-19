package archive

import (
	"bytes"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"
	"testing"

	"github.com/cesanta/errors"
)

func TestUnzip(t *testing.T) {
	data := []byte{80, 75, 3, 4, 20, 0, 8, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 31, 0, 0, 0, 119, 115, 45, 66, 83, 55, 76, 81, 79, 74, 90, 47, 102, 115, 47, 99, 111, 110, 102, 95, 118, 101, 110, 100, 111, 114, 46, 106, 115, 111, 110, 170, 174, 5, 4, 0, 0, 255, 255, 80, 75, 7, 8, 67, 191, 166, 163, 8, 0, 0, 0, 2, 0, 0, 0, 80, 75, 3, 4, 20, 0, 8, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 21, 0, 0, 0, 119, 115, 45, 66, 83, 55, 76, 81, 79, 74, 90, 47, 115, 114, 99, 47, 97, 112, 112, 46, 99, 210, 215, 82, 136, 204, 47, 45, 82, 40, 45, 78, 45, 82, 72, 206, 79, 73, 85, 200, 72, 45, 74, 85, 208, 210, 7, 4, 0, 0, 255, 255, 80, 75, 7, 8, 58, 214, 68, 140, 31, 0, 0, 0, 25, 0, 0, 0, 80, 75, 3, 4, 20, 0, 8, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 22, 0, 0, 0, 119, 115, 45, 66, 83, 55, 76, 81, 79, 74, 90, 47, 115, 114, 99, 47, 97, 112, 112, 46, 106, 115, 210, 215, 82, 136, 204, 47, 45, 82, 240, 10, 86, 72, 206, 79, 73, 85, 200, 72, 45, 74, 85, 208, 210, 7, 4, 0, 0, 255, 255, 80, 75, 7, 8, 145, 115, 70, 124, 29, 0, 0, 0, 23, 0, 0, 0, 80, 75, 1, 2, 20, 0, 20, 0, 8, 0, 8, 0, 0, 0, 0, 0, 67, 191, 166, 163, 8, 0, 0, 0, 2, 0, 0, 0, 31, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 119, 115, 45, 66, 83, 55, 76, 81, 79, 74, 90, 47, 102, 115, 47, 99, 111, 110, 102, 95, 118, 101, 110, 100, 111, 114, 46, 106, 115, 111, 110, 80, 75, 1, 2, 20, 0, 20, 0, 8, 0, 8, 0, 0, 0, 0, 0, 58, 214, 68, 140, 31, 0, 0, 0, 25, 0, 0, 0, 21, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 85, 0, 0, 0, 119, 115, 45, 66, 83, 55, 76, 81, 79, 74, 90, 47, 115, 114, 99, 47, 97, 112, 112, 46, 99, 80, 75, 1, 2, 20, 0, 20, 0, 8, 0, 8, 0, 0, 0, 0, 0, 145, 115, 70, 124, 29, 0, 0, 0, 23, 0, 0, 0, 22, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 183, 0, 0, 0, 119, 115, 45, 66, 83, 55, 76, 81, 79, 74, 90, 47, 115, 114, 99, 47, 97, 112, 112, 46, 106, 115, 80, 75, 5, 6, 0, 0, 0, 0, 3, 0, 3, 0, 212, 0, 0, 0, 24, 1, 0, 0, 0, 0}

	contents := map[string]string{
		"/fs/conf_vendor.json": "{}",
		"/src/app.c":           "/* Your user code here */",
		"/src/app.js":          "/* Your JS code here */",
	}

	tempDir, err := ioutil.TempDir("", "fwbuild-")
	if err != nil {
		t.Errorf("cannot create temp dir: %s", err)
	}
	defer os.RemoveAll(tempDir)

	bytesReader := bytes.NewReader(data)
	if err := UnzipInto(bytesReader, bytesReader.Size(), tempDir, 1); err != nil {
		t.Errorf("cannot unzip: %s", err)
	}

	foundFiles := 0
	err = filepath.Walk(tempDir, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return errors.Trace(err)
		}
		if info.IsDir() {
			return nil
		}
		fn := strings.Replace(path, tempDir, "", 1)

		c, ok := contents[fn]
		if !ok {
			return errors.Errorf("unexpected file %q", fn)
		}
		body, err := ioutil.ReadFile(path)
		if err != nil {
			return errors.Trace(err)
		}
		if want, got := c, string(body); want != got {
			return errors.Errorf("contents for %q: want %q got %q", fn, want, got)
		}
		foundFiles++
		return nil
	})
	if err != nil {
		t.Errorf("error while walking: %s", err)
	}
	if want, got := len(contents), foundFiles; want != got {
		t.Errorf("want %d files, got %d files", want, got)
	}
}
