package cc3200

import (
	"fmt"
	"sort"

	"cesanta.com/mos/flash/common"
	"github.com/cesanta/errors"
	"github.com/cesanta/go-serial/serial"
)

type FlashOpts struct {
	Port           string
	FormatSLFSSize int
}

const (
	baudRate       = 921600
	servicePackImg = "/sys/servicepack.ucf"
	bootImg        = "/sys/mcuimg.bin"
)

type fileInfo struct {
	SLFSFileInfo

	part *common.FirmwarePart
}

func Flash(fw *common.FirmwareBundle, opts *FlashOpts) error {
	var files []*fileInfo
	for _, p := range fw.Parts {
		if p.Type != "slfile" && p.Type != "boot" && p.Type != "boot_cfg" && p.Type != "app" && p.Type != "fs" {
			continue
		}
		fi := &fileInfo{
			SLFSFileInfo: SLFSFileInfo{
				Name:      p.Name,
				AllocSize: p.CC3200FileAllocSize,
			},
			part: p,
		}
		if p.Src != "" {
			var err error
			fi.Data, err = fw.GetPartData(p.Name)
			if err != nil {
				return errors.Annotatef(err, "%s: failed to get data", p.Name)
			}
			if p.CC3200FileSignature != "" {
				fs, err := fw.GetPartData(p.CC3200FileSignature)
				if err != nil {
					return errors.Annotatef(err, "%s: failed to get signature data (%s)", p.Name, p.CC3200FileSignature)
				}
				if len(fs) != FileSignatureLength {
					return errors.Errorf("%s: invalid signature length (%d)", p.Name, len(fi.Signature))
				}
				fi.Signature = fileSignature(fs)
			}
		}
		files = append(files, fi)
	}
	sort.Sort(filesByTypeAndName(files))

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
	}

	rc, err := NewROMClient(s, dc)
	if err != nil {
		return errors.Annotatef(err, "failed to run flasher")
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

type filesByTypeAndName []*fileInfo

func (ff filesByTypeAndName) Len() int      { return len(ff) }
func (ff filesByTypeAndName) Swap(i, j int) { ff[i], ff[j] = ff[j], ff[i] }
func (ff filesByTypeAndName) Less(i, j int) bool {
	fi, fj := ff[i], ff[j]
	// Service pack goes first (there's only one).
	if fi.Name == servicePackImg || fj.Name == servicePackImg {
		return fi.Name == servicePackImg && fj.Name != servicePackImg
	}
	// Then boot image (there's only one).
	if fi.Name == bootImg || fj.Name == bootImg {
		return fi.Name == bootImg && fj.Name != bootImg
	}
	// Then boot configs.
	if fi.part.Type == "boot_cfg" || fj.part.Type == "boot_cfg" {
		if fi.part.Type == "boot_cfg" && fj.part.Type != "boot_cfg" {
			return true
		}
		if fi.part.Type == "boot_cfg" && fj.part.Type == "boot_cfg" {
			return fi.Name < fj.Name
		}
		return false
	}
	// Then app.
	if fi.part.Type == "app" || fj.part.Type == "app" {
		if fi.part.Type == "app" && fj.part.Type != "app" {
			return true
		}
		if fi.part.Type == "app" && fj.part.Type == "app" {
			return fi.Name < fj.Name
		}
		return false
	}
	// Then fs containers.
	if fi.part.Type == "fs" || fj.part.Type == "fs" {
		if fi.part.Type == "fs" && fj.part.Type != "fs" {
			return true
		}
		if fi.part.Type == "fs" && fj.part.Type == "fs" {
			return fi.Name < fj.Name
		}
		return false
	}
	// Then the rest, sorted by name.
	return fi.Name < fj.Name
}
