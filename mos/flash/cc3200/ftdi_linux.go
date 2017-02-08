package cc3200

// Implements FTDI chip bit-bang pin access using libftdi
// Linux (Debian/Ubuntu): apt-get install libftdi-dev
// Note: Linux and Mac OS differ only in build flags.

/*
#include <ftdi.h>

#cgo LDFLAGS: -lftdi -lusb -pthread
*/
import "C"

import (
	"os/exec"
	"regexp"
	"runtime"
	"unsafe"

	"cesanta.com/mos/flash/common"
	"github.com/cesanta/errors"
	"github.com/golang/glog"
)

type ftdiLibWrapper struct {
	ctx  *C.struct_ftdi_context
	open bool
}

const (
	channelA         = 0x01
	modeAsyncBitBang = 0x01
)

func openFTDI(port string, vendor, product int) (FTDI, error) {
	ctx, err := C.ftdi_new()
	if ctx == nil {
		return nil, err
	}
	f := &ftdiLibWrapper{ctx: ctx}
	runtime.SetFinalizer(f, (*ftdiLibWrapper).close)
	if e := C.ftdi_set_interface(ctx, C.enum_ftdi_interface(channelA)); e < 0 {
		return nil, errors.Errorf("failed to set interface: %d", e)
	}

	var sn []byte // To keep the slice snPtr points to alive.
	var snPtr *C.char
	// Try to get serial number of this device but proceed without it in case of failure.
	if out, err := exec.Command("udevadm", "info", "--name", port).Output(); err == nil {
		glog.V(1).Infof("udevadm output:\n%s", out)
		m := regexp.MustCompile(` ID_SERIAL_SHORT=(\S+)`).FindSubmatch(out)
		if m != nil {
			sn = m[1]
			common.Reportf("Device S/N: %s", sn)
			sn = append(sn, 0) // NUL-terminate
			snPtr = (*C.char)(unsafe.Pointer(&sn[0]))
		}
	}
	if e := C.ftdi_usb_open_desc(
		f.ctx, C.int(vendor), C.int(product), (*C.char)(nil) /* description */, snPtr); e < 0 {
		return nil, errors.Errorf("failed to open: %d", e)
	}
	if e := C.ftdi_write_data_set_chunksize(f.ctx, C.uint(1)); e < 0 {
		return nil, errors.Errorf("failed to set write chunk size: %d", e)
	}
	return f, nil
}

func (f *ftdiLibWrapper) SetBitBangMode(mask byte) error {
	if e := C.ftdi_set_bitmode(f.ctx, C.uchar(mask), C.uchar(modeAsyncBitBang)); e < 0 {
		return errors.Errorf("failed to set bit mode: %d", e)
	}
	return nil
}

func (f *ftdiLibWrapper) WriteByte(data byte) error {
	if n := C.ftdi_write_data(f.ctx, (*C.uchar)(&data), 1); n != 1 {
		return errors.Errorf("failed to write byte")
	}
	return nil
}

func (f *ftdiLibWrapper) close() {
	if f.ctx != nil {
		if f.open {
			C.ftdi_usb_close(f.ctx)
		}
		C.ftdi_free(f.ctx)
		f.ctx = nil
	}
}
