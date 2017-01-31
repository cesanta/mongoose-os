package codec

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io"
	"sync"

	"cesanta.com/common/go/mgrpc/frame"
	"github.com/cesanta/errors"
	"github.com/golang/glog"
)

const (
	streamFrameDelimiter string = `"""`
)

var wantRead = errors.New("wantRead")

// Stream connection implementation (see tcp.go and serial.go)
type streamConn interface {
	io.ReadWriteCloser
	// RemoteAddr returns the string representing the other party's address.
	RemoteAddr() string
	// PreprocessFrame preprocesses the given frame and returns true if the frame
	// has been treated specially, so the regular Stream connection codec will
	// just ignore it. If PreprocessFrame returned false, Stream connection codec
	// proceeds with the frame as usual.
	PreprocessFrame(frameData []byte) (bool, error)
}

type streamConnectionCodec struct {
	// Stream connection implementation (see tcp.go and serial.go).
	conn streamConn

	// EOF flags and their lock.
	eof          bool
	lastFrameEof bool
	eofLock      sync.Mutex

	// Rx buffer and its lock.
	rxBuf     []byte
	rxBufLock sync.Mutex

	// A channel that gets closed once the underlying connection has been closed,
	// and its lock.
	closeNotifier chan struct{}
	closeOnce     sync.Once

	junkHandler func(junk []byte)
}

func StreamConn(conn streamConn, junkHandler func(junk []byte)) Codec {
	return &streamConnectionCodec{
		conn:          conn,
		closeNotifier: make(chan struct{}),
		junkHandler:   junkHandler,
	}
}

func (scc *streamConnectionCodec) String() string {
	return fmt.Sprintf("[streamConnectionCodec to %s]", scc.conn.RemoteAddr())
}

// tailMatch returns the number of bytes of needle that match at the tail of haystack,
// i.e. how many bytes need to be kept in order to be able to match the needle with
// additional data.
func tailMatch(haystack, needle []byte) int {
	for l := len(needle); l > 0; l-- {
		if bytes.HasSuffix(haystack, needle[0:l]) {
			return l
		}
	}
	return 0
}

// frameFromRxBuf tries to get frame from rx buffer, returns a frame or nil if
// there are no valid frames received.
func (scc *streamConnectionCodec) frameFromRxBuf() (*frame.Frame, error) {
	var err error
	var junk, frameData, remainder []byte
	var junkLen, frameBegin, frameDataBegin, frameDataEnd, frameEnd int
	remainder = scc.rxBuf
	frameBegin = bytes.Index(remainder, []byte(streamFrameDelimiter))
	frameEnd = -1
	// Yield stuff before begin as junk, except suffix which can become part of a match.
	if frameBegin >= 0 {
		junkLen = frameBegin
		// It's a beginning of a frame if it is followed by an opening brace.
		frameDataBegin = frameBegin + len(streamFrameDelimiter)
		if frameDataBegin < len(remainder) {
			if remainder[frameDataBegin] != '{' && remainder[frameDataBegin] != eofChar {
				// It's some random junk or maybe we lost sync, skip the thing.
				junkLen += len(streamFrameDelimiter)
				frameBegin = -1
				frameDataBegin = -1
			}
		} else {
			// Wait for one more byte.
			frameBegin = -1
			frameDataBegin = -1
			err = wantRead
		}
	} else {
		junkLen = len(scc.rxBuf) - tailMatch(scc.rxBuf, []byte(streamFrameDelimiter))
		err = wantRead
	}
	if junkLen > 0 {
		junk = scc.rxBuf[:junkLen]
		remainder = scc.rxBuf[junkLen:]
		if scc.junkHandler != nil {
			scc.junkHandler(junk)
		} else {
			glog.V(4).Infof("junk: %s", junk)
		}
	}
	if frameBegin >= 0 && len(remainder) >= len(streamFrameDelimiter)*2 {
		frameDataEnd = bytes.Index(scc.rxBuf[frameDataBegin:], []byte(streamFrameDelimiter))
		if frameDataEnd >= 0 {
			frameDataEnd += frameDataBegin
			frameEnd = frameDataEnd + len(streamFrameDelimiter)
		} else {
			// We have not received frame delimeter, so let's see if we've got EOF then
			scc.eofLock.Lock()
			if scc.eof {
				// Yes we have EOF. If we were waiting so we'll consider all data in the Rx buffer as a frame
				frameEnd = len(remainder)
				scc.lastFrameEof = true
			}
			scc.eofLock.Unlock()
		}
	}
	if frameDataBegin > 0 && frameDataEnd > 0 {
		frameData = scc.rxBuf[frameDataBegin:frameDataEnd]
		remainder = scc.rxBuf[frameEnd:]
	}
	glog.V(4).Infof("len %d junkLen %d frameBegin %d frameEnd %d remLen %d '%s' '%s' '%s'", len(scc.rxBuf), junkLen, frameBegin, frameEnd, len(remainder), junk, frameData, remainder)
	if len(remainder) != len(scc.rxBuf) {
		// Update rxBuf to contain only remainder of the data. We create a new slice
		// instead of just doing "scc.rxBuf = remainder", because then underlying
		// array of rxBuf would constantly grow, and memory for parsed data will
		// never be reclaimed.
		scc.rxBuf = make([]byte, len(remainder))
		copy(scc.rxBuf, remainder)
	}

	if len(frameData) > 0 {
		// Check if the frame needs special treatment
		handled, err := scc.conn.PreprocessFrame(frameData)
		if err != nil {
			return nil, errors.Trace(err)
		}
		if handled {
			// The frame has been treated specially, so here we just ignore it.
			return nil, nil
		}

		// Try to parse frameData.
		frame := &frame.Frame{SizeHint: len(frameData)}
		err = json.Unmarshal(frameData, frame)

		if err != nil {
			// There was an error during parsing, so just log the error and drop the
			// erroneous data
			glog.Errorf("%s: failed to parse frame: %#v %+v", scc, string(frameData), err)
			return nil, nil
		}
		return frame, nil
	} else {
		if frameDataBegin > 0 {
			err = wantRead
		}
		return nil, err
	}
}

func (scc *streamConnectionCodec) Recv(ctx context.Context) (*frame.Frame, error) {
	scc.rxBufLock.Lock()
	defer scc.rxBufLock.Unlock()
	buf := make([]byte, 10000)
	var frame *frame.Frame
	for {
		var err error
		for len(scc.rxBuf) > 0 && err != wantRead {
			frame, err = scc.frameFromRxBuf()
			if err != nil && err != wantRead {
				return nil, errors.Trace(err)
			}
			if frame != nil {
				return frame, nil
			}
		}
		readLen, err := scc.conn.Read(buf)
		eof := IsEOF(err)
		scc.eofLock.Lock()
		scc.eof = eof
		lastFrameEof := scc.lastFrameEof
		scc.eofLock.Unlock()
		glog.V(3).Infof("%d bytes read, err %q, eof? %t buffer: [%s]", readLen, err, eof, string(scc.rxBuf))
		if err != nil && err != io.EOF {
			scc.Close()
			return nil, errors.Trace(err)
		}
		if len(scc.rxBuf) == 0 && eof {
			// Heuristic to maintain "netcat-ability": terminate half-closed
			// connection immediately unless the last successfully parsed frame ended
			// at EOF and not at a delimiter.
			if !lastFrameEof {
				scc.Close()
			} else {
				glog.V(2).Infof("%s delaying close until one more frame", scc)
			}
			return nil, errors.Trace(io.EOF)
		}
		// TODO(rojer): Limit frame buffer size.
		scc.rxBuf = append(scc.rxBuf, buf[:readLen]...)
	}
}

func (scc *streamConnectionCodec) Send(ctx context.Context, f *frame.Frame) error {
	frameData := []byte(streamFrameDelimiter)
	framePayload, err := frame.MarshalJSON(f)
	if err != nil {
		return errors.Trace(err)
	}
	frameData = append(frameData, framePayload...)
	frameData = append(frameData, []byte(streamFrameDelimiter)...)
	_, err = scc.conn.Write(frameData)
	if err != nil {
		scc.Close()
		return errors.Trace(err)
	}
	scc.eofLock.Lock()
	if scc.eof {
		// Sender has closed and this was the one frame we were waiting for and now it's time to close.
		scc.Close()
	}
	scc.eofLock.Unlock()
	return nil
}

func (scc *streamConnectionCodec) Close() {
	scc.closeOnce.Do(func() {
		scc.conn.Close()
		close(scc.closeNotifier)
	})
}

func (scc *streamConnectionCodec) CloseNotify() <-chan struct{} {
	return scc.closeNotifier
}

func (scc *streamConnectionCodec) MaxNumFrames() int {
	return -1
}

func (scc *streamConnectionCodec) Info() ConnectionInfo {
	return ConnectionInfo{RemoteAddr: scc.conn.RemoteAddr()}
}
