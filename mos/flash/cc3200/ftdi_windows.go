package cc3200

// Implements FTDI chip bit-bang pin access using FTD2XX.DLL

import (
	"syscall"
	"unsafe"

	"github.com/cesanta/errors"
	"github.com/golang/glog"
)

type ftdxxAPI struct {
	Open,
	SetBitMode,
	Write,
	Close uintptr
}

var ftdapi *ftdxxAPI

type DWORD uint32
type ftHandle DWORD
type ftStatus DWORD

const (
	ftStatusOK                    ftStatus = 0
	ftStatusInvalidHandle                  = 1
	ftStatusDeviceNotFound                 = 2
	ftStatusDeviceNotOpened                = 3
	ftStatusIOError                        = 4
	ftStatusInsufficientResources          = 5
	ftStatusInvalidParameter               = 6
	ftStatusInvalidBaudRate                = 7
)

type ftd2xx struct {
	h ftHandle
}

func openFTDI(vendor, product int, serial string) (FTDI, error) {
	if ftdapi == nil {
		api, err := getAPI()
		if err != nil {
			return nil, errors.Annotatef(err, "failed to init")
		}
		ftdapi = api
	}
	ftdi := &ftd2xx{}
	// TODO(rojer): Find the right device corresponding to port.
	for i := 0; i < 10; i++ {
		if err := ftdi.Open(i); err != nil {
			glog.Infof("%s", err)
			continue
		}
		glog.Infof("opened device %d", i)
		break
	}
	return ftdi, nil
}

func getAPI() (*ftdxxAPI, error) {
	dll, err := syscall.LoadLibrary("ftd2xx.dll")
	if err != nil {
		return nil, errors.Errorf("FTD2XX library not found")
	}
	return &ftdxxAPI{
		Open:       getProcAddr(dll, "FT_Open"),
		SetBitMode: getProcAddr(dll, "FT_SetBitMode"),
		Write:      getProcAddr(dll, "FT_Write"),
		Close:      getProcAddr(dll, "FT_Close"),
	}, nil
}

func getProcAddr(lib syscall.Handle, name string) uintptr {
	addr, err := syscall.GetProcAddress(lib, name)
	if err != nil {
		glog.Errorf("%s: %s", name, err)
		return 0
	}
	return addr
}
func (f *ftd2xx) Open(index int) error {
	var h ftHandle
	// Sometimes FT_Open returns an error but succeeds, so don't rely on err and examine return values.
	r, _, _ := syscall.Syscall(ftdapi.Open, 2, uintptr(index), uintptr(unsafe.Pointer(&h)), 0)
	if r != uintptr(ftStatusOK) || h == 0 {
		return errors.Errorf("failed to open device %d: error %d", index, r)
	}
	f.h = h
	return nil
}

func (f *ftd2xx) SetBitMode(mask, mode uint8) error {
	r, _, err := syscall.Syscall(ftdapi.SetBitMode, 3, uintptr(f.h), uintptr(mask), uintptr(mode))
	if err != 0 || r != uintptr(ftStatusOK) {
		return errors.Errorf("error %d", r)
	}
	return nil
}

func (f *ftd2xx) Write(data []byte) (int, error) {
	toWrite := DWORD(len(data))
	var numWritten DWORD
	r, _, err := syscall.Syscall6(ftdapi.Write, 4, uintptr(f.h), uintptr(unsafe.Pointer(&data[0])), uintptr(toWrite), uintptr(unsafe.Pointer(&numWritten)), 0, 0)
	if err != 0 || r != uintptr(ftStatusOK) {
		return int(numWritten), errors.Errorf("error %d", r)
	}
	return int(numWritten), nil
}

func (f *ftd2xx) SetBitBangMode(mask byte) error {
	return f.SetBitMode(mask, 0x01)
}

func (f *ftd2xx) WriteByte(data byte) error {
	n, err := f.Write([]byte{data})
	if err != nil {
		return errors.Trace(err)
	}
	if n != 1 {
		return errors.Errorf("expected to write 1 byte, wrote %d", n)
	}
	return nil
}

func (f *ftd2xx) Close() {
	syscall.Syscall(ftdapi.Close, 1, uintptr(f.h), 0, 0)
}
