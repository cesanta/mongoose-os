package common

import (
	"crypto/sha1"
	"encoding/hex"
	"strings"

	"github.com/cesanta/errors"
)

type FirmwareBundle struct {
	FirmwareManifest

	Blobs map[string][]byte
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
	// CC3200
	CC3200FileAllocSize uint32 `json:"falloc,omitempty"`
	CC3200FileSignature string `json:"sign,omitempty"`
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
	} else if p.Fill != nil && p.Size > 0 {
		data = make([]byte, p.Size)
		for i, _ := range data {
			data[i] = *p.Fill
		}
	} else {
		return nil, errors.Errorf("%q: no source or filler specified", name)
	}
	return data, nil
}
