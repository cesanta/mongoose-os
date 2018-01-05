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

// Package serial provides routines for interacting with serial ports.
// Currently it supports only OS X; see the readme file for details.

package serial

import (
	"io"
	"math"
	"time"
)

// Valid parity values.
type ParityMode int

const (
	PARITY_NONE ParityMode = 0
	PARITY_ODD  ParityMode = 1
	PARITY_EVEN ParityMode = 2
)

// OpenOptions is the struct containing all of the options necessary for
// opening a serial port.
type OpenOptions struct {
	// The name of the port, e.g. "/dev/tty.usbserial-A8008HlV".
	PortName string

	// The baud rate for the port.
	BaudRate uint

	// The number of data bits per frame. Legal values are 5, 6, 7, and 8.
	DataBits uint

	// The number of stop bits per frame. Legal values are 1 and 2.
	StopBits uint

	// The type of parity bits to use for the connection. Currently parity errors
	// are simply ignored; that is, bytes are delivered to the user no matter
	// whether they were received with a parity error or not.
	ParityMode ParityMode

	// An inter-character timeout value, in milliseconds, and a minimum number of
	// bytes to block for on each read. A call to Read() that otherwise may block
	// waiting for more data will return immediately if the specified amount of
	// time elapses between successive bytes received from the device or if the
	// minimum number of bytes has been exceeded.
	//
	// Note that the inter-character timeout value may be rounded to the nearest
	// 100 ms on some systems, and that behavior is undefined if calls to Read
	// supply a buffer whose length is less than the minimum read size.
	//
	// Behaviors for various settings for these values are described below. For
	// more information, see the discussion of VMIN and VTIME here:
	//
	//     http://www.unixwiz.net/techtips/termios-vmin-vtime.html
	//
	// InterCharacterTimeout = 0 and MinimumReadSize = 0 (the default):
	//     This arrangement is not legal; you must explicitly set at least one of
	//     these fields to a positive number. (If MinimumReadSize is zero then
	//     InterCharacterTimeout must be at least 100.)
	//
	// InterCharacterTimeout > 0 and MinimumReadSize = 0
	//     If data is already available on the read queue, it is transferred to
	//     the caller's buffer and the Read() call returns immediately.
	//     Otherwise, the call blocks until some data arrives or the
	//     InterCharacterTimeout milliseconds elapse from the start of the call.
	//     Note that in this configuration, InterCharacterTimeout must be at
	//     least 100 ms.
	//
	// InterCharacterTimeout > 0 and MinimumReadSize > 0
	//     Calls to Read() return when at least MinimumReadSize bytes are
	//     available or when InterCharacterTimeout milliseconds elapse between
	//     received bytes. The inter-character timer is not started until the
	//     first byte arrives.
	//
	// InterCharacterTimeout = 0 and MinimumReadSize > 0
	//     Calls to Read() return only when at least MinimumReadSize bytes are
	//     available. The inter-character timer is not used.
	//
	// On windows, the timeout magic just doesn't seem to work correctly.
	// The modes are:
	//   * ICT = MRS = 0: non-blocking mode, Read returns immediately if no bytes are ready.
	//   * ICT = 0, MRS > 0: blocking mode, wait indefinitely for data to arrive.
	//   * ICT > 0: Read waits up to ICT, returns as soon as at least one byte is available.

	InterCharacterTimeout uint
	MinimumReadSize       uint

	// Enable hardware flow control (CTS/RTS).
	HardwareFlowControl bool

	// Use to enable RS485 mode -- probably only valid on some Linux platforms
	Rs485Enable bool

	// Set to true for logic level high during send
	Rs485RtsHighDuringSend bool

	// Set to true for logic level high after send
	Rs485RtsHighAfterSend bool

	// set to receive data during sending
	Rs485RxDuringTx bool

	// RTS delay before send
	Rs485DelayRtsBeforeSend int

	// RTS delay after send
	Rs485DelayRtsAfterSend int
}

type Serial interface {
	io.ReadWriteCloser

	Flush() error
	SetBaudRate(baudRate uint) error
	SetReadTimeout(timeout time.Duration) error
	SetRTS(active bool) error
	SetDTR(active bool) error
	SetRTSDTR(rtsActive, dtrActive bool) error
	SetBreak(active bool) error
}

// Open creates an Serial based on the supplied options struct.
func Open(options OpenOptions) (Serial, error) {
	// Redirect to the OS-specific function.
	return openInternal(options)
}

// Rounds a float to the nearest integer.
func round(f float64) float64 {
	return math.Floor(f + 0.5)
}
