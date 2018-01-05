// Copyright 2011 Cesanta Software Ltd.
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

// +build !windows
// Common parts for MacOS and Linux

package serial

import (
	"errors"
	"syscall"
	"time"
	"unsafe"
)

func (s *serialPort) setModemControl(bits uintptr, active bool) error {
	var call uintptr
	switch active {
	case true:
		call = syscall.TIOCMBIS
	case false:
		call = syscall.TIOCMBIC
	}
	return ioctlp(s.file.Fd(), call, unsafe.Pointer(&bits))
}

func (s *serialPort) SetRTS(active bool) error {
	return s.setModemControl(syscall.TIOCM_RTS, active)
}

func (s *serialPort) SetDTR(active bool) error {
	return s.setModemControl(syscall.TIOCM_DTR, active)
}

func (s *serialPort) SetRTSDTR(rtsActive, dtrActive bool) error {
	var bits uintptr
	if rtsActive {
		bits |= syscall.TIOCM_RTS
	}
	if dtrActive {
		bits |= syscall.TIOCM_DTR
	}
	return ioctlp(s.file.Fd(), syscall.TIOCMSET, unsafe.Pointer(&bits))
}

func (s *serialPort) SetBreak(active bool) error {
	var call uintptr
	switch active {
	case true:
		call = syscall.TIOCSBRK
	case false:
		call = syscall.TIOCCBRK
	}
	return ioctl(s.file.Fd(), call, 0)
}

func timeoutSettings(ict time.Duration, mrs uint) (cc_t, cc_t, error) {
	// Sanity check inter-character timeout and minimum read size options.

	vtime := uint(round(ict.Seconds() * 10))
	vmin := mrs

	if vmin == 0 && vtime < 1 {
		return 0, 0, errors.New("invalid values for InterCharacterTimeout and MinimumReadSize")
	}

	if vtime > 255 {
		return 0, 0, errors.New("invalid value for InterCharacterTimeout")
	}
	return cc_t(vtime), cc_t(vmin), nil
}

func ioctlp(fd, request uintptr, argp unsafe.Pointer) error {
	return ioctl(fd, request, uintptr(argp))
}
