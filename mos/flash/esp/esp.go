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
	ROMBaudRate            uint
	FlasherBaudRate        uint
	InvertedControlLines   bool
	FlashParams            string
	EraseChip              bool
	EnableCompression      bool
	MinimizeWrites         bool
	BootFirmware           bool
	ESP32EncryptionKeyFile string
	ESP32FlashCryptConf    uint32
}

type RegReaderWriter interface {
	ReadReg(reg uint32) (uint32, error)
	WriteReg(reg, value uint32) error
	Disconnect()
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
