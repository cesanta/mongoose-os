// +build linux windows darwin

package cc3200

import (
	"time"

	"cesanta.com/mos/flash/cc32xx"
	"github.com/cesanta/errors"
)

type FTDI interface {
	SetBitBangMode(mask byte) error
	WriteByte(data byte) error
	Close()
}

type launchXLDeviceControl struct {
	ftdi FTDI
}

const (
	vendorTI        = 0x0451
	productLaunchXL = 0xc32a
)

const (
	debugBit byte = 0x40 // 0 - debug led on, 1 - debug led off
	resetBit      = 0x20 // 0 - RST low (device in reset), 1 - RST high (running)
	sop2Bit       = 0x01 // TCK jump-wired to SOP2; 0 - SOP2 low, 1 - SOP2 high
)

func NewCC3200DeviceControl(port string) (cc32xx.DeviceControl, error) {
	// Try to get serial number of this device but proceed without it in case of failure.
	sn, _ := cc32xx.GetUSBSerialNumberForPort(port)
	ftdi, err := openFTDI(vendorTI, productLaunchXL, sn)
	if err != nil {
		return nil, errors.Annotatef(err, "failed to open FTDI")
	}
	if err = ftdi.SetBitBangMode(debugBit | resetBit | sop2Bit); err != nil {
		return nil, errors.Annotatef(err, "failed to set bitbang mode")
	}
	// Start with device in reset, SOP2 low, debug led on.
	if err = ftdi.WriteByte(0); err != nil {
		return nil, errors.Annotatef(err, "failed to set bit values")
	}
	return &launchXLDeviceControl{ftdi: ftdi}, nil
}

func (dc *launchXLDeviceControl) EnterBootLoader() error {
	// Enter reset and set SOP2 high.
	if err := dc.ftdi.WriteByte(sop2Bit); err != nil {
		return errors.Annotatef(err, "failed to enter reset state")
	}
	time.Sleep(50 * time.Millisecond)
	// Release reset, keep SOP2 high.
	if err := dc.ftdi.WriteByte(resetBit | sop2Bit); err != nil {
		return errors.Annotatef(err, "failed to leave reset state")
	}
	return nil
}

func (dc *launchXLDeviceControl) BootFirmware() error {
	// Enter reset with SOP2 low.
	if err := dc.ftdi.WriteByte(0); err != nil {
		return errors.Annotatef(err, "failed to enter reset state")
	}
	time.Sleep(50 * time.Millisecond)
	// Release control of all pins which will return reset to high.
	if err := dc.ftdi.SetBitBangMode(0); err != nil {
		return errors.Annotatef(err, "failed to set bitbang mode")
	}
	return nil
}

func (dc *launchXLDeviceControl) Close() {
	dc.ftdi.Close()
}
