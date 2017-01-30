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
	handshakeTimeout time.Duration = 7400 * time.Millisecond
)

type serialCodec struct {
	portName        string
	conn            serial.Serial
	handsShaken     bool
	handsShakenLock sync.Mutex
	writeLock       sync.Mutex
}

func Serial(ctx context.Context, portName string, junkHandler func(junk []byte)) (Codec, error) {
	glog.Infof("Opening %s...", portName)
	conn, err := serial.Open(serial.OpenOptions{
		PortName:              portName,
		BaudRate:              115200,
		DataBits:              8,
		ParityMode:            serial.PARITY_NONE,
		StopBits:              1,
		InterCharacterTimeout: 200,
		MinimumReadSize:       0,
	})
	glog.Infof("%s opened: %v, err: %v", portName, conn, err)
	if err != nil {
		return nil, errors.Trace(err)
	}

	// Flush any data that might be not yet read
	conn.Flush()

	return StreamConn(&serialCodec{
		portName:    portName,
		conn:        conn,
		handsShaken: false,
	}, junkHandler), nil
}

func (c *serialCodec) Read(buf []byte) (read int, err error) {
	res, err := c.conn.Read(buf)
	if errors.Cause(err) != io.EOF {
		return res, err
	}
	return res, nil
}

func (c *serialCodec) Write(b []byte) (written int, err error) {
	tch := time.After(handshakeTimeout)
	c.writeLock.Lock()
	defer c.writeLock.Unlock()
	c.setHandsShaken(false)
	for !c.areHandsShaken() {
		c.conn.Write([]byte(streamFrameDelimiter))
		c.conn.Write([]byte{eofChar})
		c.conn.Write([]byte(streamFrameDelimiter))
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
		n, err := c.conn.Write(b[i:min(i+chunkSize, len(b))])
		glog.V(4).Infof("written to serial: [%s]", string(b[i:i+n]))
		written += n
		if err != nil {
			c.Close()
			return written, errors.Trace(err)
		}
		time.Sleep(chunkDelay)
	}
	return written, nil
}

func (c *serialCodec) Close() error {
	glog.Infof("closing serial %s", c.portName)
	return c.conn.Close()
}

func (c *serialCodec) RemoteAddr() string {
	return c.portName
}

func (c *serialCodec) PreprocessFrame(frameData []byte) (bool, error) {
	c.setHandsShaken(true)
	if len(frameData) == 1 && frameData[0] == eofChar {
		// The single-byte frame consisting of just EOF char: we need to send
		// a delimeter back
		c.conn.Write([]byte(streamFrameDelimiter))
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
