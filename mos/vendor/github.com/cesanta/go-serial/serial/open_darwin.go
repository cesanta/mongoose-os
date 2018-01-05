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

// This file contains OS-specific constants and types that work on OS X (tested
// on version 10.6.8).
//
// Helpful documentation for some of these options:
//
//     http://www.unixwiz.net/techtips/termios-vmin-vtime.html
//     http://www.taltech.com/support/entry/serial_intro
//     http://www.cs.utah.edu/dept/old/texinfo/glibc-manual-0.02/library_16.html
//     http://permalink.gmane.org/gmane.linux.kernel/103713
//

package serial

import (
	"errors"
	"os"
	"syscall"
	"time"
	"unsafe"
)

// termios types
type cc_t byte
type speed_t uint64
type tcflag_t uint64

// sys/termios.h
const (
	kCS5    = 0x00000000
	kCS6    = 0x00000100
	kCS7    = 0x00000200
	kCS8    = 0x00000300
	kCLOCAL = 0x00008000
	kCREAD  = 0x00000800
	kCSTOPB = 0x00000400
	kIGNPAR = 0x00000004
	kPARENB = 0x00001000
	kPARODD = 0x00002000

	kNCCS = 20

	kVMIN  = tcflag_t(16)
	kVTIME = tcflag_t(17)
)

const (

	// sys/ttycom.h
	kTIOCGETA = 1078490131
	kTIOCSETA = 2152231956

	// IOKit: serial/ioss.h
	kIOSSIOSPEED = 0x80045402

	kDefaultBaudRate = 115200

	kFREAD = 0x01
)

// sys/termios.h
type termios struct {
	c_iflag  tcflag_t
	c_oflag  tcflag_t
	c_cflag  tcflag_t
	c_lflag  tcflag_t
	c_cc     [kNCCS]cc_t
	c_ispeed speed_t
	c_ospeed speed_t
}

func ioctl(fd, request, arg uintptr) error {
	r1, _, errno := syscall.Syscall(syscall.SYS_IOCTL, fd, request, arg)

	// Did the syscall return an error?
	if errno != 0 {
		return os.NewSyscallError("SYS_IOCTL", errno)
	}

	// Just in case, check the return value as well.
	if r1 != 0 {
		return errors.New("Unknown error from SYS_IOCTL.")
	}

	// This is a workaround for CH340G driver, which seems to not wait for
	// USB_CONTROL calls to finish before returning. As a result, they may
	// interleave with subsequent data transfers, which can wreak all sorts
	// of havoc, changing baud rate and resetting FIFOs and whatnot.
	time.Sleep(5 * time.Millisecond)

	return nil
}

func getTermios(fd uintptr) (*termios, error) {
	var t termios
	err := ioctlp(fd, kTIOCGETA, unsafe.Pointer(&t))
	if err != nil {
		return nil, err
	}
	return &t, nil
}

// setTermios updates the termios struct associated with a serial port file
// descriptor. This sets appropriate options for how the OS interacts with the
// port.
func setTermios(fd uintptr, src *termios) error {
	return ioctlp(fd, kTIOCSETA, unsafe.Pointer(src))
}

func convertOptions(options OpenOptions) (*termios, error) {
	var result termios

	// Ignore modem status lines. We don't want to receive SIGHUP when the serial
	// port is disconnected, for example.
	result.c_cflag |= kCLOCAL

	// Enable receiving data.
	//
	// NOTE(jacobsa): I don't know exactly what this flag is for. The man page
	// seems to imply that it shouldn't really exist.
	result.c_cflag |= kCREAD

	vtime, vmin, err := timeoutSettings(time.Duration(options.InterCharacterTimeout)*time.Millisecond, options.MinimumReadSize)
	if err != nil {
		return nil, err
	}

	result.c_cc[kVTIME] = vtime
	result.c_cc[kVMIN] = vmin

	// Set an arbitrary baudrate. We'll set the real one later.
	result.c_ispeed = kDefaultBaudRate
	result.c_ospeed = kDefaultBaudRate

	// Data bits
	switch options.DataBits {
	case 5:
		result.c_cflag |= kCS5
	case 6:
		result.c_cflag |= kCS6
	case 7:
		result.c_cflag |= kCS7
	case 8:
		result.c_cflag |= kCS8
	default:
		return nil, errors.New("Invalid setting for DataBits.")
	}

	// Stop bits
	switch options.StopBits {
	case 1:
		// Nothing to do; CSTOPB is already cleared.
	case 2:
		result.c_cflag |= kCSTOPB
	default:
		return nil, errors.New("Invalid setting for StopBits.")
	}

	// Parity mode
	switch options.ParityMode {
	case PARITY_NONE:
		// Nothing to do; PARENB is already not set.
	case PARITY_ODD:
		// Enable parity generation and receiving at the hardware level using
		// PARENB, but continue to deliver all bytes to the user no matter what (by
		// not setting INPCK). Also turn on odd parity mode.
		result.c_cflag |= kPARENB
		result.c_cflag |= kPARODD
	case PARITY_EVEN:
		// Enable parity generation and receiving at the hardware level using
		// PARENB, but continue to deliver all bytes to the user no matter what (by
		// not setting INPCK). Leave out PARODD to use even mode.
		result.c_cflag |= kPARENB
	default:
		return nil, errors.New("Invalid setting for ParityMode.")
	}

	return &result, nil
}

// Set baud rate with the IOSSIOSPEED ioctl, to support non-standard speeds.
func setBaudRate(fd uintptr, baudRate uint) error {
	return ioctlp(fd, kIOSSIOSPEED, unsafe.Pointer(&baudRate))
}

type serialPort struct {
	file     *os.File
	baudRate uint
}

func openInternal(options OpenOptions) (Serial, error) {
	// Open the serial port in non-blocking mode, since otherwise the OS will
	// wait for the CARRIER line to be asserted.
	file, err :=
		os.OpenFile(
			options.PortName,
			syscall.O_RDWR|syscall.O_NOCTTY|syscall.O_NONBLOCK|syscall.O_EXCL|syscall.O_EXLOCK,
			0600)

	if err != nil {
		return nil, err
	}

	// We want to do blocking I/O, so clear the non-blocking flag set above.
	r1, _, errno :=
		syscall.Syscall(
			syscall.SYS_FCNTL,
			uintptr(file.Fd()),
			uintptr(syscall.F_SETFL),
			uintptr(0))

	if errno != 0 {
		return nil, os.NewSyscallError("SYS_FCNTL", errno)
	}

	if r1 != 0 {
		return nil, errors.New("Unknown error from SYS_FCNTL.")
	}

	// Set standard termios options.
	terminalOptions, err := convertOptions(options)
	if err != nil {
		return nil, err
	}

	err = setTermios(file.Fd(), terminalOptions)
	if err != nil {
		return nil, err
	}

	if err := setBaudRate(file.Fd(), options.BaudRate); err != nil {
		return nil, err
	}

	// We're done.
	return &serialPort{file: file, baudRate: options.BaudRate}, nil
}

func (s *serialPort) Read(buf []byte) (int, error) {
	return s.file.Read(buf)
}

func (s *serialPort) Write(buf []byte) (int, error) {
	return s.file.Write(buf)
}

func (s *serialPort) Close() error {
	return s.file.Close()
}

func (s *serialPort) Flush() error {
	f := kFREAD
	return ioctlp(s.file.Fd(), syscall.TIOCFLUSH, unsafe.Pointer(&f))
}

func (s *serialPort) SetBaudRate(baudRate uint) error {
	err := setBaudRate(s.file.Fd(), baudRate)
	if err == nil {
		s.baudRate = baudRate
	}
	return err
}

func (s *serialPort) SetReadTimeout(timeout time.Duration) error {
	vtime, vmin, err := timeoutSettings(timeout, 0)
	if err != nil {
		return err
	}
	t, err := getTermios(s.file.Fd())
	if err != nil {
		return err
	}
	t.c_cc[kVTIME] = vtime
	t.c_cc[kVMIN] = vmin
	setSpeed := false
	if t.c_ispeed > 230400 {
		// Non-standard baud rate is being used, setting will fail. We need to re-apply it separately.
		// This will result in momentary change of baud rate, but there seems to be no way to do it transactionally.
		// There may be data being transmitted at the moment and we want it to go out at the correct baud rate,
		// so we sleep for a while. This is incredibly dumb, but alas...
		t.c_ispeed = kDefaultBaudRate
		t.c_ospeed = kDefaultBaudRate
		setSpeed = true
		time.Sleep(10 * time.Millisecond)
	}
	err = setTermios(s.file.Fd(), t)
	if setSpeed {
		err = setBaudRate(s.file.Fd(), s.baudRate)
	}
	return err
}
