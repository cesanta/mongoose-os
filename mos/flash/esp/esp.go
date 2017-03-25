package esp

import "fmt"

type ChipType int

const (
	ChipESP8266 ChipType = iota
	ChipESP32
)

type FlashOpts struct {
	ControlPort            string
	DataPort               string
	BaudRate               uint
	FlashParams            string
	EraseChip              bool
	MinimizeWrites         bool
	BootFirmware           bool
	ESP32EncryptionKeyFile string
	ESP32FlashCryptConf    uint32
}

func (ct ChipType) String() string {
	switch ct {
	case ChipESP8266:
		return "ESP8266"
	case ChipESP32:
		return "ESP32"
	default:
		return fmt.Sprintf("???(%d)", ct)
	}
}
