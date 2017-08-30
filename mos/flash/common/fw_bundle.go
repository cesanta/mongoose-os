package common

import (
	"crypto/sha1"
	"encoding/hex"
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"

	moscommon "cesanta.com/mos/common"
	"github.com/cesanta/errors"
	"github.com/golang/glog"
)

type FirmwareBundle struct {
	FirmwareManifest

	Blobs map[string][]byte

	tempDir string
}

type FirmwareManifest struct {
	Name           string                   `json:"name"`
	Platform       string                   `json:"platform"`
	Description    string                   `json:"description,omitempty"`
	Version        string                   `json:"version"`
	BuildID        string                   `json:"build_id,omitempty"`
	BuildTimestamp string                   `json:"build_timestamp,omitempty"` // TODO(rojer): Parse as time.Time
	Parts          map[string]*FirmwarePart `json:"parts"`
}

type FirmwarePart struct {
	Name         string `json:"-"`
	Type         string `json:"type,omitempty"`
	Src          string `json:"src,omitempty"`
	Size         uint32 `json:"size,omitempty"`
	Fill         *uint8 `json:"fill,omitempty"`
	ChecksumSHA1 string `json:"cs_sha1,omitempty"`
	// For SPIFFS images.
	FSSize      uint32 `json:"fs_size,omitempty"`
	FSBlockSize uint32 `json:"fs_block_size,omitempty"`
	FSEraseSize uint32 `json:"fs_erase_size,omitempty"`
	FSPageSize  uint32 `json:"fs_page_size,omitempty"`
	// Platform-specific stuff:
	// ESP32, ESP8266
	ESPFlashAddress uint32 `json:"addr,omitempty"`
	ESP32Encrypt    bool   `json:"encrypt,omitempty"`
	// CC32xx
	CC32XXFileAllocSize    int    `json:"falloc,omitempty"`
	CC32XXFileSignatureOld string `json:"sign,omitempty"` // Deprecated since 2017/08/22
	CC32XXFileSignature    string `json:"sig,omitempty"`
	CC32XXSigningCert      string `json:"sig_cert,omitempty"`
}

func (fw *FirmwareBundle) GetTempDir() (string, error) {
	if fw.tempDir == "" {
		td, err := ioutil.TempDir("", fmt.Sprintf("%s_%s_%s_",
			moscommon.FileNameFromString(fw.Name),
			moscommon.FileNameFromString(fw.Platform),
			moscommon.FileNameFromString(fw.Version)))
		if err != nil {
			return "", errors.Annotatef(err, "failed to create temp dir")
		}
		fw.tempDir = td
	}

	return fw.tempDir, nil
}

func (fw *FirmwareBundle) GetPartData(name string) ([]byte, error) {
	var data []byte
	p := fw.Parts[name]
	if p == nil {
		return nil, errors.Errorf("%q: no such part", name)
	}
	if p.Src != "" {
		data = fw.Blobs[p.Src]
		if data == nil {
			return nil, errors.Errorf("%q: %s is not present in the bundle", name, p.Src)
		}
		if p.ChecksumSHA1 != "" {
			digest := sha1.Sum(data)
			digestHex := strings.ToLower(hex.EncodeToString(digest[:]))
			expectedDigestHex := strings.ToLower(p.ChecksumSHA1)
			if digestHex != expectedDigestHex {
				return nil, errors.Errorf("%q: digest mismatch: expected %s, got %s", name, expectedDigestHex, digestHex)
			}
		}
	} else if p.Fill != nil && p.Size >= 0 {
		data = make([]byte, p.Size)
		for i, _ := range data {
			data[i] = *p.Fill
		}
	} else {
		return nil, errors.Errorf("%q: no source or filler specified", name)
	}
	return data, nil
}

func (fw *FirmwareBundle) GetPartDataFile(name string) (string, int, error) {
	data, err := fw.GetPartData(name)
	if err != nil {
		return "", -1, errors.Trace(err)
	}

	td, err := fw.GetTempDir()
	if err != nil {
		return "", -1, errors.Trace(err)
	}

	fname := filepath.Join(td, moscommon.FileNameFromString(name))

	err = ioutil.WriteFile(fname, data, 0644)

	glog.V(3).Infof("Wrote %q to %q (%d bytes)", name, fname, len(data))

	if err != nil {
		return "", -1, errors.Annotatef(err, "failed to write fw part data")
	}

	return fname, len(data), nil
}

func (fw *FirmwareBundle) Cleanup() {
	if fw.tempDir != "" {
		glog.Infof("Cleaning up %q", fw.tempDir)
		os.RemoveAll(fw.tempDir)
	}
}
