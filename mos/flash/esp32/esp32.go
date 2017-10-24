package esp32

import (
	"fmt"

	"cesanta.com/mos/flash/esp"
	"github.com/cesanta/errors"
)

var (
	FlashSizeToId = map[string]int{
		// +1, to distinguish from null-value
		"8m":   1,
		"16m":  2,
		"32m":  3,
		"64m":  4,
		"128m": 5,
	}

	FlashSizes = map[int]int{
		0: 1048576,
		1: 2097152,
		2: 4194304,
		3: 8388608,
		4: 16777216,
	}
)

func GetChipDescr(rrw esp.RegReaderWriter) (string, error) {
	_, _, fusesByName, err := ReadFuses(rrw)
	if err != nil {
		return "", errors.Trace(err)
	}
	cver, err := fusesByName["chip_package"].Value(false)
	if err != nil {
		return "", errors.Trace(err)
	}
	chip_ver := ""
	switch cver.Uint64() {
	case 0:
		chip_ver = "ESP32D0WDQ6"
	case 1:
		chip_ver = "ESP32D0WDQ5"
	case 2:
		chip_ver = "ESP32D2WDQ5"
	case 5:
		chip_ver = "ESP32-PICO-D4"
	default:
		chip_ver = fmt.Sprintf("ESP32?%d", cver.Uint64())
	}

	crev, err := fusesByName["chip_version"].Value(false)
	if err != nil {
		return "", errors.Trace(err)
	}
	chip_rev := ""
	switch crev.Uint64() {
	case 0:
		chip_rev = "R0"
	case 8:
		chip_rev = "R1"
	default:
		chip_rev = fmt.Sprintf("R?%d", crev.Uint64())
	}

	return fmt.Sprintf("%s %s", chip_ver, chip_rev), nil
}
