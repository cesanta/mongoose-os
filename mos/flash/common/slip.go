package common

import (
	"io"

	"github.com/cesanta/errors"
	"github.com/golang/glog"
)

const (
	// https://tools.ietf.org/html/rfc1055
	slipFrameDelimiter       = 0xC0
	slipEscape               = 0xDB
	slipEscapeFrameDelimiter = 0xDC
	slipEscapeEscape         = 0xDD
)

type SLIPReaderWriter struct {
	rw io.ReadWriter
}

func NewSLIPReaderWriter(rw io.ReadWriter) *SLIPReaderWriter {
	return &SLIPReaderWriter{rw: rw}
}

func (srw *SLIPReaderWriter) Read(buf []byte) (int, error) {
	n := 0
	start := true
	esc := false
	for {
		b := []byte{0}
		bn, err := srw.rw.Read(b)
		if err != nil || bn != 1 {
			return n, errors.Annotatef(err, "error reading")
		}
		if start {
			if b[0] != slipFrameDelimiter {
				return 0, errors.Errorf("invalid SLIP starting byte: 0x%02x", b[0])
			}
			start = false
			continue
		}
		if !esc {
			switch b[0] {
			case slipFrameDelimiter:
				glog.V(4).Infof("<= (%d) %s", n, LimitStr(buf[:n], 32))
				return n, nil
			case slipEscape:
				esc = true
			default:
				if n >= len(buf) {
					return n, errors.Errorf("frame buffer overflow (%d)", len(buf))
				}
				buf[n] = b[0]
				n += 1
			}
		} else {
			if n >= len(buf) {
				return n, errors.Errorf("frame buffer overflow (%d)", len(buf))
			}
			switch b[0] {
			case slipEscapeFrameDelimiter:
				buf[n] = slipFrameDelimiter
			case slipEscapeEscape:
				buf[n] = slipEscape
			default:
				return n, errors.Errorf("invalid SLIP escape sequence: %d", b[0])
			}
			n += 1
			esc = false
		}
	}
}

func (srw *SLIPReaderWriter) Write(data []byte) (int, error) {
	frame := []byte{slipFrameDelimiter}
	for _, b := range data {
		switch b {
		case slipFrameDelimiter:
			frame = append(frame, slipEscape)
			frame = append(frame, slipEscapeFrameDelimiter)
		case slipEscape:
			frame = append(frame, slipEscape)
			frame = append(frame, slipEscapeEscape)
		default:
			frame = append(frame, b)
		}
	}
	frame = append(frame, slipFrameDelimiter)
	glog.V(4).Infof("=> (%d) %s", len(data), LimitStr(data, 32))
	return srw.rw.Write(frame)
}
