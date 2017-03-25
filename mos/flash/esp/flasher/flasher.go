package esp

import (
	"crypto/md5"
	"encoding/hex"
	"io/ioutil"
	"sort"
	"strconv"
	"strings"
	"time"

	"cesanta.com/mos/flash/common"
	"cesanta.com/mos/flash/esp"
	"cesanta.com/mos/flash/esp/rom_client"
	"cesanta.com/mos/flash/esp32"
	"github.com/cesanta/errors"
	"github.com/golang/glog"
)

const (
	flashSectorSize   = 0x1000
	flashBlockSize    = 0x10000
	sysParamsPartType = "sys_params"
	sysParamsAreaSize = 4 * flashSectorSize
	fsDirPartTye      = "fs_dir"
	espImageMagicByte = 0xe9
)

type image struct {
	addr uint32
	data []byte
	part *common.FirmwarePart
}

type imagesByAddr []*image

func (pp imagesByAddr) Len() int      { return len(pp) }
func (pp imagesByAddr) Swap(i, j int) { pp[i], pp[j] = pp[j], pp[i] }
func (pp imagesByAddr) Less(i, j int) bool {
	return pp[i].addr < pp[j].addr
}

func Flash(ct esp.ChipType, fw *common.FirmwareBundle, opts *esp.FlashOpts) error {
	var err error

	if opts.BaudRate < 0 || opts.BaudRate > 4000000 {
		return errors.Errorf("invalid flashing baud rate (%d)", opts.BaudRate)
	}
	if len(opts.FlashParams) == 0 {
		return errors.Errorf("flash params not provided")
	}
	flashParams, flashSize, err := parseFlashParams(ct, opts.FlashParams)
	if err != nil {
		return errors.Annotatef(err, "invalid flash params (%q)", opts.FlashParams)
	}

	var encryptionKey []byte
	if ct == esp.ChipESP32 && opts.ESP32EncryptionKeyFile != "" {
		encryptionKey, err = ioutil.ReadFile(opts.ESP32EncryptionKeyFile)
		if err != nil {
			return errors.Annotatef(err, "failed to read encryption key")
		}
		if len(encryptionKey) != 32 {
			return errors.Errorf("encryption key must be 32 bytes, got %d", len(encryptionKey))
		}
	}

	rc, err := rom_client.ConnectToROM(ct, opts)
	if err != nil {
		return errors.Trace(err)
	}
	defer rc.Disconnect()

	fc, err := NewFlasherClient(ct, rc, opts.BaudRate)
	if err != nil {
		return errors.Annotatef(err, "failed to run flasher")
	}
	if flashSize <= 0 {
		flashSize, err = detectFlashSize(fc)
		if err != nil {
			return errors.Annotatef(err, "flash size is not specified and could not be detected")
		}
		if flashSize > 4194304 {
			glog.Warningf("Clamping flash size to 32m (actual: %d)", flashSize)
			flashSize = 4194304
		}
		var flashSizes []int
		switch ct {
		case esp.ChipESP8266:
			flashSizes = flashSizesESP8266
		case esp.ChipESP32:
			flashSizes = flashSizesESP32
		}
		for i, s := range flashSizes {
			if s == flashSize {
				flashParams |= (i << 4)
				break
			}
		}
	}
	common.Reportf("Flash size: %d, params: 0x%04x", flashSize, flashParams)

	if ct == esp.ChipESP8266 {
		// Based on our knowledge of flash size, adjust type=sys_params image.
		adjustSysParamsLocation(fw, flashSize)
	}

	// Sort images by address
	var images []*image
	for _, p := range fw.Parts {
		if p.Type == fsDirPartTye {
			continue
		}
		data, err := fw.GetPartData(p.Name)
		if err != nil {
			return errors.Annotatef(err, "%s: failed to get data", p.Name)
		}
		im := &image{addr: p.ESPFlashAddress, data: data, part: p}
		if im.addr == 0 || im.addr == 0x1000 && len(data) >= 4 && data[0] == 0xe9 {
			im.data[2] = byte((flashParams >> 8) & 0xff)
			im.data[3] = byte(flashParams & 0xff)
		}
		if p.ESP32Encrypt && encryptionKey != nil {
			encData, err := esp32.ESP32EncryptImageData(
				im.data, encryptionKey, im.addr, opts.ESP32FlashCryptConf)
			if err != nil {
				return errors.Annotatef(err, "%s: failed to encrypt", p.Name)
			}
			im.data = encData
		}
		images = append(images, im)
	}
	sort.Sort(imagesByAddr(images))

	err = sanityCheckImages(ct, images, flashSize, flashSectorSize)
	if err != nil {
		return errors.Trace(err)
	}

	imagesToWrite := images
	if opts.EraseChip {
		common.Reportf("Erasing chip...")
		if err = fc.EraseChip(); err != nil {
			return errors.Annotatef(err, "failed to erase chip")
		}
	} else if opts.MinimizeWrites {
		common.Reportf("Deduping...")
		imagesToWrite, err = dedupImages(fc, images)
		if err != nil {
			return errors.Annotatef(err, "failed to dedup images")
		}
	}

	if len(imagesToWrite) > 0 {
		common.Reportf("Writing...")
		start := time.Now()
		bytesWritten := 0
		for _, im := range imagesToWrite {
			data := im.data
			if len(data)%flashSectorSize != 0 {
				newData := make([]byte, len(data))
				copy(newData, data)
				paddingLen := flashSectorSize - len(data)%flashSectorSize
				for i := 0; i < paddingLen; i++ {
					newData = append(newData, 0xff)
				}
				data = newData
			}
			common.Reportf("  %6d @ 0x%x", len(data), im.addr)
			err = fc.Write(im.addr, data, true /* erase */)
			if err != nil {
				return errors.Annotatef(err, "%s: failed to write", im.part.Name)
			}
			bytesWritten += len(data)
		}
		seconds := time.Since(start).Seconds()
		bytesPerSecond := float64(bytesWritten) / seconds
		common.Reportf("Wrote %d bytes in %.2f seconds (%.2f KBit/sec)", bytesWritten, seconds, bytesPerSecond*8/1024)
	}

	common.Reportf("Verifying...")
	for _, im := range images {
		common.Reportf("  %6d @ 0x%x", len(im.data), im.addr)
		digest, err := fc.Digest(im.addr, uint32(len(im.data)), 0 /* blockSize */)
		if err != nil {
			return errors.Annotatef(err, "%s: failed to compute digest %d @ 0x%x", im.part.Name, len(im.data), im.addr)
		}
		if len(digest) != 1 || len(digest[0]) != 16 {
			return errors.Errorf("unexpected digest packetresult %+v", digest)
		}
		digestHex := strings.ToLower(hex.EncodeToString(digest[0]))
		expectedDigest := md5.Sum(im.data)
		expectedDigestHex := strings.ToLower(hex.EncodeToString(expectedDigest[:]))
		if digestHex != expectedDigestHex {
			return errors.Errorf("%d @ 0x%x: digest mismatch: expected %s, got %s", len(im.data), im.addr, expectedDigestHex, digestHex)
		}
	}
	if opts.BootFirmware {
		common.Reportf("Booting firmware...")
		if err = fc.BootFirmware(); err != nil {
			return errors.Annotatef(err, "failed to reboot into firmware")
		}
	}
	return nil
}

func detectFlashSize(fc *FlasherClient) (int, error) {
	chipID, err := fc.GetFlashChipID()
	if err != nil {
		return 0, errors.Annotatef(err, "failed to get flash chip id")
	}
	// Parse the JEDEC ID.
	mfg := (chipID & 0xff0000) >> 16
	sizeExp := (chipID & 0xff)
	glog.V(2).Infof("Flash chip ID: 0x%08x, mfg: 0x%02x, sizeExp: %d", chipID, mfg, sizeExp)
	if mfg == 0 || sizeExp < 19 || sizeExp > 32 {
		return 0, errors.Errorf("invalid chip id: 0x%08x", chipID)
	}
	// Capacity is the power of two.
	return (1 << sizeExp), nil
}

func adjustSysParamsLocation(fw *common.FirmwareBundle, flashSize int) {
	sysParamsAddr := uint32(flashSize - sysParamsAreaSize)
	for _, p := range fw.Parts {
		if p.Type != sysParamsPartType {
			continue
		}
		if p.ESPFlashAddress != sysParamsAddr {
			glog.Infof("Sys params image moved from 0x%x to 0x%x", p.ESPFlashAddress, sysParamsAddr)
			p.ESPFlashAddress = sysParamsAddr
		}
	}
}

func sanityCheckImages(ct esp.ChipType, images []*image, flashSize, flashSectorSize int) error {
	// Note: we require that images are sorted by address.
	sort.Sort(imagesByAddr(images))
	for i, im := range images {
		imageBegin := int(im.addr)
		imageEnd := imageBegin + len(im.data)
		if imageBegin >= flashSize || imageEnd > flashSize {
			return errors.Errorf(
				"Image %d @ 0x%x will not fit in flash (size %d)", len(im.data), imageBegin, flashSize)
		}
		if imageBegin%flashSectorSize != 0 {
			return errors.Errorf("Image starting address (0x%x) is not on flash sector boundary (sector size %d)",
				imageBegin,
				flashSectorSize)
		}
		if imageBegin == 0 && len(im.data) > 0 {
			if im.data[0] != espImageMagicByte {
				return errors.Errorf("Invalid magic byte in the first image")
			}
		}
		if ct == esp.ChipESP8266 {
			sysParamsBegin := flashSize - sysParamsAreaSize
			if imageBegin == sysParamsBegin && im.part.Type == sysParamsPartType {
				// Ok, a sys_params image.
			} else if imageEnd > sysParamsBegin {
				return errors.Errorf("Image 0x%x overlaps with system params area (%d @ 0x%x)",
					imageBegin, sysParamsAreaSize, sysParamsBegin)
			}
		}
		if i > 0 {
			prevImageBegin := int(images[i-1].addr)
			prevImageEnd := prevImageBegin + len(images[i-1].data)
			// We traverse the list in order, so a simple check will suffice.
			if prevImageEnd > imageBegin {
				return errors.Errorf("Images 0x%x and 0x%x overlap", prevImageBegin, imageBegin)
			}
		}
	}
	return nil
}

func dedupImages(fc *FlasherClient, images []*image) ([]*image, error) {
	var dedupedImages []*image
	for _, im := range images {
		glog.V(2).Infof("%d @ 0x%x", len(im.data), im.addr)
		imAddr := int(im.addr)
		digests, err := fc.Digest(im.addr, uint32(len(im.data)), flashSectorSize)
		if err != nil {
			return nil, errors.Annotatef(err, "%s: failed to compute digest %d @ 0x%x", im.part.Name, len(im.data), im.addr)
		}
		i, offset := 0, 0
		var newImages []*image
		newAddr, newLen, newTotalLen := imAddr, 0, 0
		for offset < len(im.data) {
			blockLen := flashSectorSize
			if offset+blockLen > len(im.data) {
				blockLen = len(im.data) - offset
			}
			digestHex := strings.ToLower(hex.EncodeToString(digests[i]))
			expectedDigest := md5.Sum(im.data[offset : offset+blockLen])
			expectedDigestHex := strings.ToLower(hex.EncodeToString(expectedDigest[:]))
			glog.V(2).Infof("0x%06x %4d %s %s %t", imAddr+offset, blockLen, expectedDigestHex, digestHex, expectedDigestHex == digestHex)
			if expectedDigestHex == digestHex {
				// Found a matching sector. If we've been building an image,  commit it.
				if newLen > 0 {
					nim := &image{addr: uint32(newAddr), data: im.data[newAddr-imAddr : newAddr-imAddr+newLen], part: im.part}
					glog.V(2).Infof("%d @ 0x%x", len(nim.data), nim.addr)
					newImages = append(newImages, nim)
					newTotalLen += newLen
					newAddr, newLen = 0, 0
				}
			} else {
				// Found a sector that needs to be written. Start a new image or continue the existing one.
				if newLen == 0 {
					newAddr = imAddr + offset
				}
				newLen += blockLen
			}
			offset += blockLen
			i++
		}
		if newLen > 0 {
			nim := &image{addr: uint32(newAddr), data: im.data[newAddr-imAddr : newAddr-imAddr+newLen], part: im.part}
			newImages = append(newImages, nim)
			glog.V(2).Infof("%d @ %x", len(nim.data), nim.addr)
			newTotalLen += newLen
			newAddr, newLen = 0, 0
		}
		glog.V(2).Infof("%d @ 0x%x -> %d", len(im.data), im.addr, newTotalLen)
		// There's a price for fragmenting a large image: erasing many individual
		// sectors is slower than erasing a whole block. So unless the difference
		// is substantial, don't bother.
		if newTotalLen < len(im.data) && (newTotalLen < flashBlockSize || len(im.data)-newTotalLen >= flashBlockSize) {
			dedupedImages = append(dedupedImages, newImages...)
			common.Reportf("  %6d @ 0x%x -> %d", len(im.data), im.addr, newTotalLen)
		} else {
			dedupedImages = append(dedupedImages, im)
		}
	}
	return dedupedImages, nil
}

var (
	flashModes = map[string]int{
		// +1, to distinguish from null-value
		"qio":  1,
		"qout": 2,
		"dio":  3,
		"dout": 4,
	}
	flashFreqs = map[string]int{
		// +1, to distinguish from null-value
		"40m": 1,
		"26m": 2,
		"20m": 3,
		"80m": 0x10,
	}
	flashSizeToIdESP8266 = map[string]int{
		// +1, to distinguish from null-value
		"4m":     1,
		"2m":     2,
		"8m":     3,
		"16m":    4,
		"32m":    5,
		"16m-c1": 6,
		"32m-c1": 7,
		"32m-c2": 8,
	}
	flashSizesESP8266  = []int{524288, 262144, 1048576, 2097152, 4194304, 2097152, 4194304, 4194304}
	flashSizeToIdESP32 = map[string]int{
		// +1, to distinguish from null-value
		"8m":   1,
		"16m":  2,
		"32m":  3,
		"64m":  4,
		"128m": 7,
	}
	flashSizesESP32 = []int{1048576, 2097152, 4194304, 8388608, 16777216}
)

func parseFlashParams(ct esp.ChipType, ps string) (int, int, error) {
	var flashSizeToId map[string]int
	var flashSizes []int
	switch ct {
	case esp.ChipESP8266:
		flashSizeToId = flashSizeToIdESP8266
		flashSizes = flashSizesESP8266
	case esp.ChipESP32:
		flashSizeToId = flashSizeToIdESP32
		flashSizes = flashSizesESP32
	}
	parts := strings.Split(ps, ",")
	switch len(parts) {
	case 1: // a number
		p64, err := strconv.ParseInt(ps, 0, 16)
		if err != nil {
			return -1, -1, errors.Trace(err)
		}
		flashSizeId := (p64 >> 4) & 0xf
		if flashSizeId > 7 {
			return -1, -1, errors.Errorf("invalid flash size (%d)", flashSizeId)
		}
		return int(p64), flashSizes[flashSizeId], nil
	case 3: // a mode,size,freq triplet
		flashMode := flashModes[parts[0]]
		if flashMode == 0 {
			return -1, -1, errors.Errorf("invalid flash mode (%q)", parts[0])
		}
		flashMode--
		flashSizeId := 0
		flashSize := 0
		if len(parts[1]) > 0 {
			flashSizeId = flashSizeToId[parts[1]]
			if flashSizeId == 0 {
				return -1, -1, errors.Errorf("invalid flash size (%q)", parts[1])
			}
			flashSizeId--
			flashSize = flashSizes[flashSizeId]
		}
		flashFreq := flashFreqs[parts[2]]
		if flashFreq == 0 {
			return -1, -1, errors.Errorf("invalid flash freq (%q)", parts[2])
		}
		flashFreq--
		return ((flashMode << 8) | (flashSizeId << 4) | flashFreq), flashSize, nil
	default:
		return -1, -1, errors.Errorf("invalid flash params format")
	}
}
