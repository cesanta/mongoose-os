package flasher

import (
	"io/ioutil"
	"strconv"
	"strings"

	"cesanta.com/mos/flash/esp"
	"cesanta.com/mos/flash/esp/rom_client"
	"cesanta.com/mos/flash/esp32"
	"cesanta.com/mos/flash/esp8266"
	"github.com/cesanta/errors"
	"github.com/golang/glog"
)

type cfResult struct {
	rc                 *rom_client.ROMClient
	fc                 *FlasherClient
	flashSize          int
	flashParams        int
	esp32EncryptionKey []byte
}

func ConnectToFlasherClient(ct esp.ChipType, opts *esp.FlashOpts) (*cfResult, error) {
	var err error
	r := &cfResult{}

	if opts.BaudRate < 0 || opts.BaudRate > 4000000 {
		return nil, errors.Errorf("invalid flashing baud rate (%d)", opts.BaudRate)
	}
	if len(opts.FlashParams) == 0 {
		return nil, errors.Errorf("flash params not provided")
	}
	r.flashParams, r.flashSize, err = parseFlashParams(ct, opts.FlashParams)
	if err != nil {
		return nil, errors.Annotatef(err, "invalid flash params (%q)", opts.FlashParams)
	}

	if ct == esp.ChipESP32 && opts.ESP32EncryptionKeyFile != "" {
		r.esp32EncryptionKey, err = ioutil.ReadFile(opts.ESP32EncryptionKeyFile)
		if err != nil {
			return nil, errors.Annotatef(err, "failed to read encryption key")
		}
		if len(r.esp32EncryptionKey) != 32 {
			return nil, errors.Errorf("encryption key must be 32 bytes, got %d", len(r.esp32EncryptionKey))
		}
	}

	r.rc, err = rom_client.ConnectToROM(ct, opts)
	if err != nil {
		return nil, errors.Trace(err)
	}
	ownROMClient := true
	defer func() {
		if ownROMClient {
			r.rc.Disconnect()
		}
	}()

	r.fc, err = NewFlasherClient(ct, r.rc, opts.BaudRate)
	if err != nil {
		return nil, errors.Annotatef(err, "failed to run flasher")
	}
	if r.flashSize <= 0 {
		r.flashSize, err = detectFlashSize(r.fc)
		if err != nil {
			return nil, errors.Annotatef(err, "flash size is not specified and could not be detected")
		}
		if r.flashSize > 4194304 {
			glog.Warningf("Clamping flash size to 32m (actual: %d)", r.flashSize)
			r.flashSize = 4194304
		}
		var flashSizes []int
		switch ct {
		case esp.ChipESP8266:
			flashSizes = esp8266.FlashSizes
		case esp.ChipESP32:
			flashSizes = esp32.FlashSizes
		}
		for i, s := range flashSizes {
			if s == r.flashSize {
				r.flashParams |= (i << 4)
				break
			}
		}
	}
	ownROMClient = false
	return r, nil
}

func detectFlashSize(fc *FlasherClient) (int, error) {
	chipID, err := fc.GetFlashChipID()
	if err != nil {
		return 0, errors.Annotatef(err, "failed to get flash chip id")
	}
	// Parse the JEDEC ID.
	mfg := (chipID & 0xff0000) >> 16
	sizeExp := (chipID & 0xff)
	glog.V(2).Infof("Flash chip ID: 0x%08x, mfg: 0x%02x, sizeExp: %d", chipID, mfg, sizeExp)
	if mfg == 0 || sizeExp < 19 || sizeExp > 32 {
		return 0, errors.Errorf("invalid chip id: 0x%08x", chipID)
	}
	// Capacity is the power of two.
	return (1 << sizeExp), nil
}

var (
	flashModes = map[string]int{
		// +1, to distinguish from null-value
		"qio":  1,
		"qout": 2,
		"dio":  3,
		"dout": 4,
	}
	flashFreqs = map[string]int{
		// +1, to distinguish from null-value
		"40m": 1,
		"26m": 2,
		"20m": 3,
		"80m": 0x10,
	}
)

func parseFlashParams(ct esp.ChipType, ps string) (int, int, error) {
	var flashSizeToId map[string]int
	var flashSizes []int
	switch ct {
	case esp.ChipESP8266:
		flashSizeToId = esp8266.FlashSizeToId
		flashSizes = esp8266.FlashSizes
	case esp.ChipESP32:
		flashSizeToId = esp32.FlashSizeToId
		flashSizes = esp32.FlashSizes
	}
	parts := strings.Split(ps, ",")
	switch len(parts) {
	case 1: // a number
		p64, err := strconv.ParseInt(ps, 0, 16)
		if err != nil {
			return -1, -1, errors.Trace(err)
		}
		flashSizeId := (p64 >> 4) & 0xf
		if flashSizeId > 7 {
			return -1, -1, errors.Errorf("invalid flash size (%d)", flashSizeId)
		}
		return int(p64), flashSizes[flashSizeId], nil
	case 3: // a mode,size,freq triplet
		flashMode := flashModes[parts[0]]
		if flashMode == 0 {
			return -1, -1, errors.Errorf("invalid flash mode (%q)", parts[0])
		}
		flashMode--
		flashSizeId := 0
		flashSize := 0
		if len(parts[1]) > 0 {
			flashSizeId = flashSizeToId[parts[1]]
			if flashSizeId == 0 {
				return -1, -1, errors.Errorf("invalid flash size (%q)", parts[1])
			}
			flashSizeId--
			flashSize = flashSizes[flashSizeId]
		}
		flashFreq := flashFreqs[parts[2]]
		if flashFreq == 0 {
			return -1, -1, errors.Errorf("invalid flash freq (%q)", parts[2])
		}
		flashFreq--
		return ((flashMode << 8) | (flashSizeId << 4) | flashFreq), flashSize, nil
	default:
		return -1, -1, errors.Errorf("invalid flash params format")
	}
}
