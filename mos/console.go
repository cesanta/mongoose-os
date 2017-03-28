package main

import (
	"context"
	"fmt"
	"os"
	"time"

	"cesanta.com/mos/dev"
	"cesanta.com/mos/timestamp"

	"github.com/cesanta/errors"
	"github.com/cesanta/go-serial/serial"
	flag "github.com/spf13/pflag"
)

// console specific flags
var (
	baudRate uint
	noInput  bool
	hwFC     bool
	tsfSpec  string
)

var (
	tsFormat string
)

func init() {
	flag.UintVar(&baudRate, "baud-rate", 115200, "Serial port speed")
	flag.BoolVar(&noInput, "no-input", false,
		"Do not read from stdin, only print device's output to stdout")
	flag.BoolVar(&hwFC, "hw-flow-control", false, "Enable hardware flow control (CTS/RTS)")

	flag.StringVar(&tsfSpec, "timestamp", "",
		"Prepend each line with a timestamp in the specified format. A number of specifications are supported:"+
			"simple 'yes' or 'true' will use UNIX Epoch + .microseconds; the Go way of specifying date/time "+
			"format, as described in https://golang.org/pkg/time/, including the constants "+
			"(so --timestamp=UnixDate will work, as will --timestamp=Stamp); the strftime(3) format "+
			"(see http://strftime.org/)")

	flag.Lookup("timestamp").NoOptDefVal = "true" // support just passing --timestamp

	for _, f := range []string{"no-input", "timestamp"} {
		hiddenFlags = append(hiddenFlags, f)
	}

}

func console(ctx context.Context, devConn *dev.DevConn) error {
	in, out := os.Stdin, os.Stdout

	if tsfSpec != "" {
		tsFormat = timestamp.ParseTimeStampFormatSpec(tsfSpec)
	}

	port, err := getPort()
	if err != nil {
		return errors.Trace(err)
	}

	s, err := serial.Open(serial.OpenOptions{
		PortName:            port,
		BaudRate:            baudRate,
		HardwareFlowControl: hwFC,
		DataBits:            8,
		ParityMode:          serial.PARITY_NONE,
		StopBits:            1,
		MinimumReadSize:     1,
	})
	if err != nil {
		return errors.Annotatef(err, "failed to open %s", port)
	}
	// Explicitly deactivate DTR and RTS.
	// Some converters/drivers activate them which, in case of ESP, may put device in reset mode.
	s.SetDTR(false)
	s.SetRTS(false)
	cctx, cancel := context.WithCancel(ctx)
	go func() { // Serial -> Stdout
		lineStart := true
		for {
			buf := make([]byte, 100)
			n, err := s.Read(buf)
			if n > 0 {
				removeNonText(buf[:n])
				if tsfSpec != "" {
					ts := time.Now()
					for i, b := range buf[:n] {
						if lineStart && tsfSpec != "" {
							fmt.Printf("[%s] ", timestamp.FormatTimestamp(ts, tsfSpec, tsFormat))
						}
						out.Write(buf[i : i+1])
						lineStart = (b == '\n')
					}
				} else {
					out.Write(buf[:n])
				}
			}
			if err != nil {
				reportf("read err %s", err)
				cancel()
				return
			}
		}
	}()
	go func() { // Stdin -> Serial
		// If no input, just block forever
		if noInput {
			select {}
		}
		for {
			buf := make([]byte, 1)
			n, err := in.Read(buf)
			if n > 0 {
				s.Write(buf[:n])
			}
			if err != nil {
				cancel()
				return
			}
		}
	}()
	<-cctx.Done()
	return nil
}

func removeNonText(data []byte) {
	for i, c := range data {
		if (c < 0x20 && c != 0x0a && c != 0x0d && c != 0x1b /* Esc */) || c >= 0x80 {
			data[i] = 0x20
		}
	}
}
