/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

package cc3200

import (
	"fmt"
	"sort"

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

type fileInfo struct {
	cc32xx.SLFSFileInfo

	part *common.FirmwarePart
}

func isKnownPartType(pt string) bool {
	return pt == cc32xx.PartTypeServicePack ||
		pt == cc32xx.PartTypeSLFile ||
		pt == cc32xx.PartTypeBootLoader ||
		pt == cc32xx.PartTypeBootLoaderConfig ||
		pt == cc32xx.PartTypeApp ||
		pt == cc32xx.PartTypeFSContainer
}

func Flash(fw *common.FirmwareBundle, opts *FlashOpts) error {
	var files []*fileInfo

	parts := []*common.FirmwarePart{}
	for _, p := range fw.Parts {
		if isKnownPartType(p.Type) {
			parts = append(parts, p)
		}
	}
	sort.Sort(cc32xx.PartsByTypeAndName(parts))
	for _, p := range parts {
		fi := &fileInfo{
			SLFSFileInfo: cc32xx.SLFSFileInfo{
				Name:      p.Name,
				AllocSize: uint32(p.CC32XXFileAllocSize),
			},
			part: p,
		}
		if p.Src != "" {
			var err error
			fi.Data, err = fw.GetPartData(p.Name)
			if err != nil {
				return errors.Annotatef(err, "%s: failed to get data", p.Name)
			}
			if p.CC32XXFileSignature != "" {
				fs, err := fw.GetPartData(p.CC32XXFileSignature)
				if err != nil {
					return errors.Annotatef(err, "%s: failed to get signature data (%s)", p.Name, p.CC32XXFileSignature)
				}
				if len(fs) != cc32xx.FileSignatureLength {
					return errors.Errorf("%s: invalid signature length (%d)", p.Name, len(fi.Signature))
				}
				fi.Signature = cc32xx.FileSignature(fs)
			}
		}
		files = append(files, fi)
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

	dc, err := NewCC3200DeviceControl(opts.Port)
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

	vi, err := rc.GetVersionInfo()
	if err != nil {
		return errors.Annotatef(err, "failed to get loader version info")
	}
	if vi.BootLoaderVersion < 0x00040102 {
		// These are early pre-production devices that require loading code stubs, etc.
		return errors.Errorf("unsupported boot loader version (%s)", vi.BootLoaderVersionString())
	}

	if err := rc.SwitchToNWPLoader(); err != nil {
		return errors.Annotatef(err, "failed to connect to switch to NWP boot loader")
	}

	if opts.FormatSLFSSize > 0 {
		common.Reportf("Formatting SFLASH file system (%d)...", opts.FormatSLFSSize)
		err = rc.FormatSLFS(opts.FormatSLFSSize)
		if err != nil {
			return errors.Annotatef(err, "failed to format SLFS")
		}
	}

	if len(files) > 0 {
		common.Reportf("Writing...")
		for _, fi := range files {
			common.Reportf("  %s", fi)
			if err := rc.UploadFile(&fi.SLFSFileInfo); err != nil {
				return errors.Annotatef(err, "failed to write %s", fi.Name)
			}
		}
	}

	if dc != nil {
		common.Reportf("Booting firmware...")
		dc.BootFirmware()
	}

	return nil
}

func (fi *fileInfo) String() string {
	s := fmt.Sprintf("%s: size %d", fi.Name, len(fi.Data))
	if int(fi.AllocSize) > len(fi.Data) {
		s += fmt.Sprintf(", alloc %d", fi.AllocSize)
	}
	if len(fi.Signature) > 0 {
		s += ", signed"
	}
	return s
}
