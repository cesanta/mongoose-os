package cc3220

import (
	"cesanta.com/mos/flash/cc32xx"
	"cesanta.com/mos/flash/common"
	"github.com/cesanta/errors"
	"github.com/cesanta/go-serial/serial"
)

type FlashOpts struct {
	Port           string
	FormatSLFSSize int
}

const (
	baudRate = 921600
)

func Flash(fw *common.FirmwareBundle, opts *FlashOpts) error {
	common.Reportf("Opening %s...", opts.Port)
	s, err := serial.Open(serial.OpenOptions{
		PortName:              opts.Port,
		BaudRate:              baudRate,
		DataBits:              8,
		ParityMode:            serial.PARITY_NONE,
		StopBits:              1,
		InterCharacterTimeout: 200.0,
	})
	if err != nil {
		return errors.Annotatef(err, "failed to open %s", opts.Port)
	}
	defer s.Close()

	dc, err := NewCC3220DeviceControl(opts.Port)
	if err != nil {
		common.Reportf(
			"Failed to open device control interface (%s). "+
				"Make sure that device is in the boot loader mode (SOP2 = 1).", err)
	} else {
		defer dc.Close()
	}

	rc, err := cc32xx.NewROMClient(s, dc)
	if err != nil {
		return errors.Annotatef(err, "failed to connect to boot loader")
	}

	// At some point we may want to program on-chip flash before switching to NWP.
	rc.GetStorageInfo(0)
	rc.GetStorageInfo(1)
	rc.GetStorageInfo(2)
	rc.GetStorageInfo(4)

	if err := rc.SwitchToNWPLoader(); err != nil {
		return errors.Annotatef(err, "failed to connect to switch to NWP boot loader")
	}

	rc.GetStorageInfo(0)
	rc.GetStorageInfo(1)
	rc.GetStorageInfo(2)
	rc.GetStorageInfo(4)

	rc.GetDeviceInfo()

	mac, err := rc.GetMACAddress()
	if err != nil {
		return errors.Annotatef(err, "failed to get MAC address")
	}
	common.Reportf("  MAC: %s", mac)

	//opts.FormatSLFSSize = 4 * 1024 * 1024
	if opts.FormatSLFSSize > 0 {
		common.Reportf("Formatting SFLASH file system (%d)...", opts.FormatSLFSSize)
		err = rc.FormatSLFS(opts.FormatSLFSSize)
		if err != nil {
			return errors.Annotatef(err, "failed to format SLFS")
		}
	}

	return errors.NotImplementedf("flashing")
}
