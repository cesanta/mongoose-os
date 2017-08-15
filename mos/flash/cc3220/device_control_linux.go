// +build !no_libudev

package cc3220

import (
	"time"

	"cesanta.com/mos/flash/cc3220/xds110"
	"cesanta.com/mos/flash/cc32xx"
	"cesanta.com/mos/flash/common"
	"github.com/cesanta/errors"
)

type xds110DeviceControl struct {
	xc *xds110.XDS110Client
}

func NewCC3220DeviceControl(port string) (cc32xx.DeviceControl, error) {
	// Try to get serial number of this device but proceed without it in case of failure.
	sn, _ := cc32xx.GetUSBSerialNumberForPort(port)
	common.Reportf("Using XDS110 debug probe to control the device...")
	xc, err := xds110.NewXDS110Client(sn)
	if err != nil {
		return nil, errors.Annotatef(err, "failed to open XDS110")
	}
	vi, err := xc.GetVersionInfo()
	if err != nil {
		return nil, errors.Annotatef(err, "failed to get version")
	}
	common.Reportf("  XDS110 %d.%d.%d.%d HW %d S/N %s", vi.V1, vi.V2, vi.V3, vi.V4, vi.HWVersion, xc.GetSerialNumber())
	err = xc.Connect()
	if err != nil {
		return nil, errors.Annotatef(err, "failed to enable the probe")
	}
	return &xds110DeviceControl{xc: xc}, nil
}

func (dc *xds110DeviceControl) EnterBootLoader() error {
	if err := dc.xc.SetSRST(true); err != nil {
		return errors.Trace(err)
	}
	time.Sleep(50 * time.Millisecond)
	if err := dc.xc.SetSRST(false); err != nil {
		return errors.Trace(err)
	}
	return nil
}

func (dc *xds110DeviceControl) BootFirmware() error {
	return dc.EnterBootLoader()
}

func (dc *xds110DeviceControl) Close() {
	dc.xc.Disconnect()
}
