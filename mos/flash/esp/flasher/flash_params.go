package flasher

import (
	"fmt"
	"strconv"
	"strings"

	"cesanta.com/mos/flash/esp"
	"cesanta.com/mos/flash/esp32"
	"cesanta.com/mos/flash/esp8266"
	"github.com/cesanta/errors"
	"github.com/golang/glog"
)

var (
	flashModes = map[string]int{
		// +1, to distinguish from null-value
		"qio":  1,
		"qout": 2,
		"dio":  3,
		"dout": 4,
	}
	flashModeIdToMode = map[int]string{0: "qio", 1: "qout", 2: "dio", 3: "dout"}
	flashFreqs        = map[string]int{
		// +1, to distinguish from null-value
		"40m": 1,
		"26m": 2,
		"20m": 3,
		"80m": 0x10,
	}
	flashFreqIdToFreq = map[int]string{0: "40m", 1: "26m", 2: "20m", 0xf: "80m"}
)

type flashParams struct {
	ct     esp.ChipType
	sizeId int
	modeId int
	freqId int
}

func getFlashSizeId(ct esp.ChipType, s string) int {
	switch ct {
	case esp.ChipESP8266:
		return esp8266.FlashSizeToId[s] - 1
	case esp.ChipESP32:
		return esp32.FlashSizeToId[s] - 1
	}
	return -1
}

func getFlashSize(ct esp.ChipType, sizeId int) int {
	switch ct {
	case esp.ChipESP8266:
		return esp8266.FlashSizes[sizeId]
	case esp.ChipESP32:
		return esp32.FlashSizes[sizeId]
	}
	return 0
}

func (fp *flashParams) ParseString(ct esp.ChipType, ps string) error {
	parts := strings.Split(ps, ",")
	flashModeId := -1
	flashSizeId := -1
	flashFreqId := -1
	switch len(parts) {
	case 1: // a number
		if len(parts[0]) > 0 {
			p64, err := strconv.ParseInt(parts[0], 0, 16)
			if err != nil {
				return errors.Trace(err)
			}
			return fp.ParseBytes(ct, uint8(p64>>8), uint8(p64))
		} else {
			// Empty string, all fields unset.
		}

	case 3: // a mode,size,freq triplet
		if len(parts[0]) > 0 {
			flashModeId = flashModes[parts[0]] - 1
			if flashModeId < 0 {
				return errors.Errorf("invalid flash mode %q", parts[0])
			}
		}
		if len(parts[1]) > 0 {
			flashSizeId = getFlashSizeId(ct, parts[1])
			if flashSizeId < 0 {
				return errors.Errorf("invalid flash size %q", parts[1])
			}
		}
		if len(parts[2]) > 0 {
			flashFreqId = flashFreqs[parts[2]] - 1
			if flashFreqId < 0 {
				return errors.Errorf("invalid flash freq %q", parts[2])
			}
		}
	default:
		return errors.Errorf("invalid flash params format")
	}
	fp.ct = ct
	fp.modeId = flashModeId
	fp.sizeId = flashSizeId
	fp.freqId = flashFreqId
	glog.Infof("%s -> %d, %d, %d", ps, fp.modeId, fp.sizeId, fp.freqId)
	return nil
}

func (fp *flashParams) ParseBytes(ct esp.ChipType, b1, b2 uint8) error {
	flashModeId := int(b1)
	if flashModeIdToMode[flashModeId] == "" {
		return errors.Errorf("infalid flash mode %d", flashModeId)
	}
	flashSizeId := int((b2 >> 4) & 0xf)
	if getFlashSize(ct, flashSizeId) == 0 {
		return errors.Errorf("invalid flash size %d", flashSizeId)
	}
	flashFreqId := int(b2 & 0xf)
	if flashFreqIdToFreq[flashFreqId] == "" {
		return errors.Errorf("infalid flash freq %d", flashFreqId)
	}
	fp.ct = ct
	fp.modeId = flashModeId
	fp.sizeId = flashSizeId
	fp.freqId = flashFreqId
	glog.Infof("%02x, %02x -> %d, %d, %d", b1, b2, fp.modeId, fp.sizeId, fp.freqId)
	return nil
}

func (fp *flashParams) Mode() string {
	return flashModeIdToMode[fp.modeId]
}

func (fp *flashParams) SetMode(mode string) error {
	modeId := flashModes[mode] - 1
	if modeId < 0 {
		return errors.Errorf("invalid flash mode %q", mode)
	}
	fp.modeId = modeId
	return nil
}

func (fp *flashParams) Size() int {
	return getFlashSize(fp.ct, fp.sizeId)
}

func (fp *flashParams) SetSize(size int) error {
	var flashSizes map[int]int
	switch fp.ct {
	case esp.ChipESP8266:
		flashSizes = esp8266.FlashSizes
	case esp.ChipESP32:
		flashSizes = esp32.FlashSizes
	}
	for sizeId, s := range flashSizes {
		if s == size {
			fp.sizeId = sizeId
			return nil
		}
	}
	return errors.Errorf("invalid flash size %d", size)
}

func (fp *flashParams) Freq() string {
	return flashFreqIdToFreq[fp.freqId]
}

func (fp *flashParams) SetFreq(freq string) error {
	freqId := flashFreqs[freq] - 1
	if freqId < 0 {
		return errors.Errorf("invalid flash freq %q", freq)
	}
	fp.freqId = freqId
	return nil
}

func (fp flashParams) Bytes() (uint8, uint8) {
	return uint8(fp.modeId), ((uint8(fp.sizeId) & 0xf) << 4) | (uint8(fp.freqId) & 0xf)
}

func (fp flashParams) String() string {
	b1, b2 := fp.Bytes()
	sizeStr := ""
	var flashSizeToId map[string]int
	switch fp.ct {
	case esp.ChipESP8266:
		flashSizeToId = esp8266.FlashSizeToId
	case esp.ChipESP32:
		flashSizeToId = esp32.FlashSizeToId
	}
	for k, v := range flashSizeToId {
		if v-1 == fp.sizeId {
			sizeStr = k
		}
	}
	return fmt.Sprintf("0x%02x%02x (%s,%s,%s)", b1, b2, flashModeIdToMode[fp.modeId], sizeStr, flashFreqIdToFreq[fp.freqId])
}
