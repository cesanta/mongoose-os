package cc3200

// Implements FTDI chip bit-bang pin access using libftdi
// Mac OS X: brew install libftdi
// Note: Linux and Mac OS differ only in build flags.

/*
#include <ftdi.h>

#cgo CFLAGS: -I/usr/local/include/libftdi1
#cgo LDFLAGS: -L/usr/local/lib -lftdi1 -pthread
*/
import "C"

import (
	"github.com/cesanta/errors"
)

type ftdiLibWrapper struct {
	ctx  *C.struct_ftdi_context
	open bool
}

const (
	channelA         = 0x01
	modeAsyncBitBang = 0x01
)

func openFTDI(vendor, product int, serial string) (FTDI, error) {
	ctx, err := C.ftdi_new()
	if ctx == nil {
		return nil, err
	}
	f := &ftdiLibWrapper{ctx: ctx}
	if e := C.ftdi_set_interface(ctx, C.enum_ftdi_interface(channelA)); e < 0 {
		return nil, errors.Errorf("failed to set interface: %d", e)
	}

	// TODO(rojer): Find the right device corresponding to port.
	if e := C.ftdi_usb_open(f.ctx, C.int(vendor), C.int(product)); e < 0 {
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

func (f *ftdiLibWrapper) Close() {
	if f.ctx != nil {
		if f.open {
			C.ftdi_usb_close(f.ctx)
		}
		C.ftdi_free(f.ctx)
		f.ctx = nil
	}
}
