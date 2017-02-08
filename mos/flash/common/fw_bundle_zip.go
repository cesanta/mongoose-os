package common

import (
	"archive/zip"
	"bytes"
	"encoding/json"
	"io/ioutil"
	"net/http"
	"path"
	"strings"

	"github.com/cesanta/errors"
)

const (
	manifestFileName = "manifest.json"
)

func NewZipFirmwareBundle(fname string) (*FirmwareBundle, error) {
	var r *zip.Reader
	var err error
	if strings.HasPrefix(fname, "http://") || strings.HasPrefix(fname, "https://") {
		Reportf("Fetching %s...", fname)
		resp, err := http.Get(fname)
		if err != nil {
			return nil, errors.Annotatef(err, "%s: failed to fetch", fname)
		}
		defer resp.Body.Close()
		if resp.StatusCode != http.StatusOK {
			return nil, errors.Errorf("%s: failed to fetch: %s", fname, resp.Status)
		}
		b, err := ioutil.ReadAll(resp.Body)
		if err != nil {
			return nil, errors.Annotatef(err, "%s: failed to fetch body", fname)
		}
		r, err = zip.NewReader(bytes.NewReader(b), int64(len(b)))
	} else {
		rc, err2 := zip.OpenReader(fname)
		if err2 == nil {
			defer rc.Close()
			r = &rc.Reader
		} else {
			err = err2
		}
	}
	if err != nil {
		return nil, errors.Annotatef(err, "%s: invalid firmware file", fname)
	}

	fwb := &FirmwareBundle{Blobs: make(map[string][]byte)}
	for _, f := range r.File {
		rc, err := f.Open()
		if err != nil {
			return nil, errors.Annotatef(err, "%s: failed to open", fname)
		}
		data, err := ioutil.ReadAll(rc)
		if err != nil {
			return nil, errors.Annotatef(err, "%s: failed to read", fname)
		}
		rc.Close()
		fwb.Blobs[path.Base(f.Name)] = data
	}
	if fwb.Blobs[manifestFileName] == nil {
		return nil, errors.Errorf("%s: no %s in the archive", fname, manifestFileName)
	}
	err = json.Unmarshal(fwb.Blobs[manifestFileName], &fwb.FirmwareManifest)
	if err != nil {
		return nil, errors.Annotatef(err, "%s: failed to parse manifest", fname)
	}
	for n, p := range fwb.FirmwareManifest.Parts {
		p.Name = n
	}
	return fwb, nil
}
