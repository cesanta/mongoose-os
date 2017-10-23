package esp8266

import (
	"cesanta.com/mos/flash/esp"
	"github.com/cesanta/errors"
)

var (
	FlashSizeToId = map[string]int{
		// +1, to distinguish from null-value
		"4m":   1,
		"2m":   2,
		"8m":   3,
		"16m":  4,
		"32m":  5,
		"64m":  9,
		"128m": 10,
	}

	FlashSizes = map[int]int{
		0: 524288,
		1: 262144,
		2: 1048576,
		3: 2097152,
		4: 4194304,
		8: 8388608,
		9: 16777216,
	}
)

func GetChipDescr(rrw esp.RegReaderWriter) (string, error) {
	efuse0, err := rrw.ReadReg(0x3ff00050)
	if err != nil {
		return "", errors.Annotatef(err, "failed to read eFuse")
	}
	efuse2, err := rrw.ReadReg(0x3ff00058)
	if err != nil {
		return "", errors.Annotatef(err, "failed to read eFuse")
	}
	if efuse0&(1<<4) != 0 || efuse2&(1<<16) != 0 {
		return "ESP8285", nil
	}
	return "ESP8266EX", nil
}
