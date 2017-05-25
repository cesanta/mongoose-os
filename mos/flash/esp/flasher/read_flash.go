package flasher

import (
	"time"

	"cesanta.com/mos/flash/common"
	"cesanta.com/mos/flash/esp"
	"github.com/cesanta/errors"
)

func ReadFlash(ct esp.ChipType, addr uint32, length int, opts *esp.FlashOpts) ([]byte, error) {
	if addr < 0 {
		return nil, errors.Errorf("invalid addr: %d", addr)
	}
	if length < 0 {
		return nil, errors.Errorf("invalid addr: %d", length)
	}

	cfr, err := ConnectToFlasherClient(ct, opts)
	if err != nil {
		return nil, errors.Trace(err)
	}
	defer cfr.rc.Disconnect()

	flashSize := cfr.flashParams.Size()
	if addr == 0 && length == 0 {
		length = flashSize
	} else if int(addr)+length > flashSize {
		return nil, errors.Errorf("0x%x + %d exceeds flash size (%d)", addr, length, flashSize)
	}

	common.Reportf("Reading %d @ 0x%x...", length, addr)
	data := make([]byte, length)
	start := time.Now()
	if err := cfr.fc.Read(uint32(addr), data); err != nil {
		return nil, errors.Annotatef(err, "failed to read %d @ 0x%x", length, addr)
	}
	seconds := time.Since(start).Seconds()
	bytesPerSecond := float64(len(data)) / seconds
	common.Reportf("Read %d bytes in %.2f seconds (%.2f KBit/sec)", length, seconds, bytesPerSecond*8/1024)
	return data, nil
}
