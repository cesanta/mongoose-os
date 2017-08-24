/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

//go:generate go-bindata -pkg cc3220 -nocompress -modtime 1 -mode 420 data/

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
	BPIBinary      string
}

const (
	baudRate = 921600
	// From cc3220_embedded_programming/sources/ImageProgramming.py
	flashPatchWriteLocation = 33*0x1000 + 8
)

func Flash(fw *common.FirmwareBundle, opts *FlashOpts) error {
	if opts.BPIBinary == "" {
		bpib, err := findBPIBinary()
		if err != nil {
			return errors.Annotatef(err, "path to BuildProgrammingImage is not specified and it could not be found in the usual places. Make sure UniFlash 4.x is installed.")
		}
		common.Reportf("Found BPI binary: %s", bpib)
		opts.BPIBinary = bpib
	}

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

	if err := rc.SwitchToNWPLoader(); err != nil {
		return errors.Annotatef(err, "failed to connect to switch to NWP boot loader")
	}

	mac, err := rc.GetMACAddress()
	if err != nil {
		return errors.Annotatef(err, "failed to get MAC address")
	}
	common.Reportf("  MAC: %s", mac)

	// Upload programming code patches first.
	common.Reportf("Applying boot loader patches...")
	ramPatch := MustAsset("data/BTL_ram.ptc")
	if err := rc.RawEraseAndWrite(cc32xx.StorageRAM, 0, ramPatch); err != nil {
		return errors.Annotatef(err, "failed to apply RAM patch")
	}
	if err := rc.ExecuteFromRAM(); err != nil {
		return errors.Annotatef(err, "failed to apply RAM patch")
	}
	vi, err := rc.GetVersionInfo()
	if err != nil {
		return errors.Annotatef(err, "failed to get patched loader version info")
	}
	common.Reportf("  RAM patch applied, new version: %s", vi.BootLoaderVersionString())
	flashPatch := MustAsset("data/BTL_sflash.ptc")
	if err := rc.RawEraseAndWrite(cc32xx.StorageSFlash, flashPatchWriteLocation, flashPatch); err != nil {
		return errors.Annotatef(err, "failed to apply RAM patch")
	}
	common.Reportf("  Flash patch applied")

	flashSize := 4 * 1024 * 1024 // TODO(rojer): detect

	common.Reportf("Generating UCF image for %s (flash size: %d)", mac, flashSize)
	imgfn, imgfs, err := buildUCFImageFromFirmwareBundle(fw, opts.BPIBinary, mac, flashSize)
	if err != nil {
		return errors.Annotatef(err, "failed to create UCF image")
	}

	common.Reportf("Uploading UCF image (%d bytes)", imgfs)

	if err := rc.UploadImageFile(imgfn); err != nil {
		return errors.Annotatef(err, "failed to upload image")
	}

	common.Reportf("Rebooting device...")

	return dc.BootFirmware()
}
