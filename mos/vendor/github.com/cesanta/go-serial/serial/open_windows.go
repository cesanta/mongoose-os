// Copyright 2011 Aaron Jacobs. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package serial

import (
	"fmt"
	"io"
	"os"
	"sync"
	"syscall"
	"time"
	"unsafe"
)

type serialPort struct {
	f  *os.File
	fd syscall.Handle
	rl sync.Mutex
	wl sync.Mutex
	ro *syscall.Overlapped
	wo *syscall.Overlapped
}

type structDCB struct {
	DCBlength, BaudRate                            uint32
	flags                                          [4]byte
	wReserved, XonLim, XoffLim                     uint16
	ByteSize, Parity, StopBits                     byte
	XonChar, XoffChar, ErrorChar, EofChar, EvtChar byte
	wReserved1                                     uint16
}

type structTimeouts struct {
	ReadIntervalTimeout         uint32
	ReadTotalTimeoutMultiplier  uint32
	ReadTotalTimeoutConstant    uint32
	WriteTotalTimeoutMultiplier uint32
	WriteTotalTimeoutConstant   uint32
}

func openInternal(options OpenOptions) (Serial, error) {
	if len(options.PortName) > 0 && options.PortName[0] != '\\' {
		options.PortName = "\\\\.\\" + options.PortName
	}

	h, err := syscall.CreateFile(syscall.StringToUTF16Ptr(options.PortName),
		syscall.GENERIC_READ|syscall.GENERIC_WRITE,
		0,
		nil,
		syscall.OPEN_EXISTING,
		syscall.FILE_ATTRIBUTE_NORMAL|syscall.FILE_FLAG_OVERLAPPED,
		0)
	if err != nil {
		return nil, err
	}
	f := os.NewFile(uintptr(h), options.PortName)
	defer func() {
		if err != nil {
			f.Close()
		}
	}()

	if err = setCommState(h, options); err != nil {
		return nil, err
	}
	if err = setupComm(h, 64, 64); err != nil {
		return nil, err
	}
	if err = setCommTimeouts(h, time.Duration(options.InterCharacterTimeout)*time.Millisecond, options.MinimumReadSize); err != nil {
		return nil, err
	}
	if err = setCommMask(h); err != nil {
		return nil, err
	}

	ro, err := newOverlapped()
	if err != nil {
		return nil, err
	}
	wo, err := newOverlapped()
	if err != nil {
		return nil, err
	}
	port := new(serialPort)
	port.f = f
	port.fd = h
	port.ro = ro
	port.wo = wo

	return port, nil
}

func (p *serialPort) Close() error {
	return p.f.Close()
}

func (p *serialPort) Write(buf []byte) (int, error) {
	p.wl.Lock()
	defer p.wl.Unlock()

	if err := resetEvent(p.wo.HEvent); err != nil {
		return 0, err
	}
	var n uint32
	err := syscall.WriteFile(p.fd, buf, &n, p.wo)
	if err != nil && err != syscall.ERROR_IO_PENDING {
		return int(n), err
	}
	return getOverlappedResult(p.fd, p.wo)
}

func (p *serialPort) Read(buf []byte) (int, error) {
	if p == nil || p.f == nil {
		return 0, fmt.Errorf("Invalid port on read %v %v", p, p.f)
	}

	p.rl.Lock()
	defer p.rl.Unlock()

	if err := resetEvent(p.ro.HEvent); err != nil {
		return 0, err
	}
	var done uint32
	err := syscall.ReadFile(p.fd, buf, &done, p.ro)
	if err != nil && err != syscall.ERROR_IO_PENDING {
		return int(done), err
	}
	n, err := getOverlappedResult(p.fd, p.ro)
	if n == 0 && err == nil {
		return 0, io.EOF
	}
	return n, err
}

const PURGE_RXCLEAR = 0x8

func (s *serialPort) Flush() error {
	r, _, err := syscall.Syscall(nPurgeComm, 2, uintptr(s.fd), uintptr(PURGE_RXCLEAR), 0)
	if r == 0 {
		return err
	}
	return nil
}

func (s *serialPort) SetBaudRate(baudRate uint) error {
	var params structDCB
	params.DCBlength = uint32(unsafe.Sizeof(params))
	r, _, err := syscall.Syscall(nGetCommState, 2, uintptr(s.fd), uintptr(unsafe.Pointer(&params)), 0)
	if r == 0 {
		return err
	}
	params.BaudRate = uint32(baudRate)
	r, _, err = syscall.Syscall(nSetCommState, 2, uintptr(s.fd), uintptr(unsafe.Pointer(&params)), 0)
	if r == 0 {
		return err
	}
	return nil
}

func (s *serialPort) SetReadTimeout(timeout time.Duration) error {
	return setCommTimeouts(s.fd, timeout, 0)
}

const (
	// Function constants for EscapeCommFunction
	kCLRDTR uint32 = 6
	kCLRRTS        = 4
	kSETDTR        = 5
	kSETRTS        = 3
)

func (s *serialPort) escapeCommFunction(function uint32) error {
	r, _, err := syscall.Syscall(nEscapeCommFunction, 2, uintptr(s.fd), uintptr(function), 0)
	if r == 0 {
		return err
	}
	return nil
}

func (s *serialPort) SetRTS(active bool) error {
	if active {
		return s.escapeCommFunction(kSETRTS)
	} else {
		return s.escapeCommFunction(kCLRRTS)
	}
}

func (s *serialPort) SetDTR(active bool) error {
	if active {
		return s.escapeCommFunction(kSETDTR)
	} else {
		return s.escapeCommFunction(kCLRDTR)
	}
}

func (s *serialPort) SetRTSDTR(rtsActive, dtrActive bool) error {
	// Windows doesn't have a function to flip RTS and DTR atomically.
	s.SetDTR(dtrActive)
	s.SetRTS(rtsActive)
	return nil
}

func (s *serialPort) SetBreak(active bool) error {
	call := nClearCommBreak
	if active {
		call = nSetCommBreak
	}
	r, _, err := syscall.Syscall(call, 1, uintptr(s.fd), 0, 0)
	if r == 0 {
		return err
	}
	return nil
}

var (
	nClearCommBreak,
	nCreateEvent,
	nEscapeCommFunction,
	nGetCommState,
	nGetOverlappedResult,
	nPurgeComm,
	nResetEvent,
	nSetCommBreak,
	nSetCommMask,
	nSetCommState,
	nSetCommTimeouts,
	nSetupComm uintptr
)

func init() {
	k32, err := syscall.LoadLibrary("kernel32.dll")
	if err != nil {
		panic("LoadLibrary " + err.Error())
	}
	defer syscall.FreeLibrary(k32)

	nClearCommBreak = getProcAddr(k32, "ClearCommBreak")
	nCreateEvent = getProcAddr(k32, "CreateEventW")
	nEscapeCommFunction = getProcAddr(k32, "EscapeCommFunction")
	nGetCommState = getProcAddr(k32, "GetCommState")
	nGetOverlappedResult = getProcAddr(k32, "GetOverlappedResult")
	nPurgeComm = getProcAddr(k32, "PurgeComm")
	nResetEvent = getProcAddr(k32, "ResetEvent")
	nSetCommBreak = getProcAddr(k32, "SetCommBreak")
	nSetCommMask = getProcAddr(k32, "SetCommMask")
	nSetCommState = getProcAddr(k32, "SetCommState")
	nSetCommTimeouts = getProcAddr(k32, "SetCommTimeouts")
	nSetupComm = getProcAddr(k32, "SetupComm")
}

func getProcAddr(lib syscall.Handle, name string) uintptr {
	addr, err := syscall.GetProcAddress(lib, name)
	if err != nil {
		panic(name + " " + err.Error())
	}
	return addr
}

func setCommState(h syscall.Handle, options OpenOptions) error {
	var params structDCB
	params.DCBlength = uint32(unsafe.Sizeof(params))

	params.flags[0] = 0x01  // fBinary
	params.flags[0] |= 0x10 // Assert DSR

	if options.ParityMode != PARITY_NONE {
		params.flags[0] |= 0x03 // fParity
		params.Parity = byte(options.ParityMode)
	}

	if options.StopBits == 1 {
		params.StopBits = 0
	} else if options.StopBits == 2 {
		params.StopBits = 2
	}

	params.BaudRate = uint32(options.BaudRate)
	params.ByteSize = byte(options.DataBits)

	r, _, err := syscall.Syscall(nSetCommState, 2, uintptr(h), uintptr(unsafe.Pointer(&params)), 0)
	if r == 0 {
		return err
	}
	return nil
}

func setCommTimeouts(h syscall.Handle, ict time.Duration, mrs uint) error {
	const MAXDWORD = 1<<32 - 1
	var timeouts structTimeouts
	// rojer: These timeout settings don't seem to work as described.
	// The only reliable setting seems to be the constant, the inter-char magic with multiplers
	// just doesn't seem to work.
	// Luckily, we don't care about these finer details, so - whatever.
	if ict > 0 {
		intervalTimeoutMs := uint32(round(ict.Seconds() * 1000.0))
		timeouts.ReadIntervalTimeout = MAXDWORD
		timeouts.ReadTotalTimeoutMultiplier = MAXDWORD
		timeouts.ReadTotalTimeoutConstant = intervalTimeoutMs
	} else if mrs > 0 {
		// Blocking mode - wait essentially forever or until data arrives.
		timeouts.ReadIntervalTimeout = 1
		timeouts.ReadTotalTimeoutMultiplier = 0
		timeouts.ReadTotalTimeoutConstant = MAXDWORD - 1
	} else {
		// Non-blocking mode.
		timeouts.ReadIntervalTimeout = MAXDWORD
		timeouts.ReadTotalTimeoutMultiplier = 0
		timeouts.ReadTotalTimeoutConstant = 0
	}

	r, _, err := syscall.Syscall(nSetCommTimeouts, 2, uintptr(h), uintptr(unsafe.Pointer(&timeouts)), 0)
	if r == 0 {
		return err
	}
	return nil
}

func setupComm(h syscall.Handle, in, out int) error {
	r, _, err := syscall.Syscall(nSetupComm, 3, uintptr(h), uintptr(in), uintptr(out))
	if r == 0 {
		return err
	}
	return nil
}

func setCommMask(h syscall.Handle) error {
	const EV_RXCHAR = 0x0001
	r, _, err := syscall.Syscall(nSetCommMask, 2, uintptr(h), EV_RXCHAR, 0)
	if r == 0 {
		return err
	}
	return nil
}

func resetEvent(h syscall.Handle) error {
	r, _, err := syscall.Syscall(nResetEvent, 1, uintptr(h), 0, 0)
	if r == 0 {
		return err
	}
	return nil
}

func newOverlapped() (*syscall.Overlapped, error) {
	var overlapped syscall.Overlapped
	r, _, err := syscall.Syscall6(nCreateEvent, 4, 0, 1, 0, 0, 0, 0)
	if r == 0 {
		return nil, err
	}
	overlapped.HEvent = syscall.Handle(r)
	return &overlapped, nil
}

func getOverlappedResult(h syscall.Handle, overlapped *syscall.Overlapped) (int, error) {
	var n int
	r, _, err := syscall.Syscall6(nGetOverlappedResult, 4,
		uintptr(h),
		uintptr(unsafe.Pointer(overlapped)),
		uintptr(unsafe.Pointer(&n)), 1, 0, 0)
	if r == 0 {
		return n, err
	}

	return n, nil
}
