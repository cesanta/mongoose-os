package codec

import (
	"context"
	"io"
	"sync"
	"time"

	"github.com/cesanta/errors"
	"github.com/cesanta/go-serial/serial"
	"github.com/golang/glog"
)

const (
	eofChar byte = 0x04

	// Due to lack of flow control, we send data in chunks and wait 5 ms after
	// each chunk.
	chunkSize  int           = 16
	chunkDelay time.Duration = 5 * time.Millisecond

	// Period for sending initial delimeter when opening a channel, until we
	// receive the delimeter in response
	handshakeInterval time.Duration = 200 * time.Millisecond

	// Maximum time to wait for a device to handshake with us
	handshakeTimeout time.Duration = 10 * time.Second

	interCharacterTimeout time.Duration = 200 * time.Millisecond
)

type serialCodec struct {
	portName        string
	conn            serial.Serial
	lastEOFTime     time.Time
	handsShaken     bool
	handsShakenLock sync.Mutex
	writeLock       sync.Mutex

	// Underlying serial port implementation allows concurrent Read/Write, but
	// calling Close concurrently results in a race. A read-write lock fits
	// perfectly for this case: for either Read or Write we lock it for reading
	// (RLock/RUnlock), but for Close we lock it for writing (Lock/Unlock).
	closeLock sync.RWMutex
}

func Serial(ctx context.Context, portName string, junkHandler func(junk []byte)) (Codec, error) {
	glog.Infof("Opening %s...", portName)
	s, err := serial.Open(serial.OpenOptions{
		PortName:              portName,
		BaudRate:              115200,
		DataBits:              8,
		ParityMode:            serial.PARITY_NONE,
		StopBits:              1,
		InterCharacterTimeout: uint(interCharacterTimeout / time.Millisecond),
		MinimumReadSize:       0,
	})
	glog.Infof("%s opened: %v, err: %v", portName, s, err)
	if err != nil {
		return nil, errors.Trace(err)
	}
	// Explicitly deactivate DTR and RTS.
	// Some converters/drivers activate them which, in case of ESP, may put device in reset mode.
	s.SetDTR(false)
	s.SetRTS(false)

	// Flush any data that might be not yet read
	s.Flush()

	return newStreamConn(&serialCodec{
		portName:    portName,
		conn:        s,
		handsShaken: false,
	}, true /* addChecksum */, junkHandler), nil
}

func (c *serialCodec) connRead(buf []byte) (read int, err error) {
	// Lock closeLock for reading
	c.closeLock.RLock()
	defer c.closeLock.RUnlock()
	return c.conn.Read(buf)
}

func (c *serialCodec) connWrite(buf []byte) (written int, err error) {
	// Lock closeLock for reading.
	// NOTE: don't be confused by the fact that we're going to Write to the port,
	// but we lock closeLock for reading. See comments for closeLock above for
	// details.
	c.closeLock.RLock()
	defer c.closeLock.RUnlock()
	return c.conn.Write(buf)
}

func (c *serialCodec) connClose() error {
	// Close can't be called concurrently with Read/Write, so, lock closeLock
	// for writing.
	c.closeLock.Lock()
	defer c.closeLock.Unlock()
	return c.conn.Close()
}

func (c *serialCodec) Read(buf []byte) (read int, err error) {
	res, err := c.connRead(buf)

	// We keep getting io.EOF after interCharacterTimeout (200 ms), and in order
	// to detect the actual EOF, we check the time of the previous pseudo-EOF.
	// If it's shorter than the half of the interCharacterTimeout, we assume
	// it's a real EOF.
	if errors.Cause(err) == io.EOF {
		now := time.Now()
		if !c.lastEOFTime.Add(interCharacterTimeout / 2).After(now) {
			// It's pseudo-EOF, clear the error
			err = nil
		}
		c.lastEOFTime = now
	}
	return res, errors.Trace(err)
}

func (c *serialCodec) Write(b []byte) (written int, err error) {
	tch := time.After(handshakeTimeout)
	c.writeLock.Lock()
	defer c.writeLock.Unlock()
	c.setHandsShaken(false)
	for !c.areHandsShaken() {
		if _, err := c.connWrite([]byte(streamFrameDelimiter)); err != nil {
			return 0, errors.Trace(err)
		}
		if _, err := c.connWrite([]byte{eofChar}); err != nil {
			return 0, errors.Trace(err)
		}
		if _, err := c.connWrite([]byte(streamFrameDelimiter)); err != nil {
			return 0, errors.Trace(err)
		}
		glog.V(1).Infof(" ...sent frame delimiter.")
		time.Sleep(handshakeInterval)

		select {
		case <-tch:
			return 0, errors.Errorf("Device handshake timeout")
		default:
		}
	}
	// Device is ready, send data.
	for i := 0; i < len(b); i += chunkSize {
		n, err := c.connWrite(b[i:min(i+chunkSize, len(b))])
		written += n
		if err != nil {
			c.Close()
			return written, errors.Trace(err)
		}
		glog.V(4).Infof("sent %d [%s]", n, string(b[i:i+n]))
		time.Sleep(chunkDelay)
	}
	return written, nil
}

func (c *serialCodec) Close() error {
	glog.Infof("closing serial %s", c.portName)
	return c.connClose()
}

func (c *serialCodec) RemoteAddr() string {
	return c.portName
}

func (c *serialCodec) PreprocessFrame(frameData []byte) (bool, error) {
	c.setHandsShaken(true)
	if len(frameData) == 1 && frameData[0] == eofChar {
		// The single-byte frame consisting of just EOF char: we need to send
		// a delimeter back
		if _, err := c.connWrite([]byte(streamFrameDelimiter)); err != nil {
			return true, errors.Trace(err)
		}
		return true, nil
	}
	return false, nil
}

func (c *serialCodec) areHandsShaken() bool {
	c.handsShakenLock.Lock()
	defer c.handsShakenLock.Unlock()
	return c.handsShaken
}

func (c *serialCodec) setHandsShaken(shaken bool) {
	c.handsShakenLock.Lock()
	defer c.handsShakenLock.Unlock()
	if !c.handsShaken && shaken {
		glog.Infof("handshake complete")
	}
	c.handsShaken = shaken
}

func min(a, b int) int {
	if a < b {
		return a
	} else {
		return b
	}
}
