package codec

import (
	"golang.org/x/net/context"
	"io"
	"runtime"
	"sync"
	"time"

	"github.com/cesanta/errors"
	"github.com/cesanta/go-serial/serial"
	"github.com/golang/glog"
)

const (
	eofChar byte = 0x04

	// XON/XOFF chars are used for software flow control.
	// They cannot occur in valid JSON, so this is completely transparent to the protocol.
	// We do it at the RPC layer since most development modules we work with do not have pins
	// available for HW flow control and software flow control support is not reliable enough
	// across different operating systems and drivers.
	xonChar  = 0x11
	xoffChar = 0x13

	// Period for sending initial delimeter when opening a channel, until we
	// receive the delimeter in response
	handshakeInterval time.Duration = 200 * time.Millisecond

	interCharacterTimeout time.Duration = 200 * time.Millisecond
)

type SerialCodecOptions struct {
	SendChunkSize        int
	SendChunkDelay       time.Duration
	JunkHandler          func(junk []byte)
	InvertedControlLines bool
}

type serialCodec struct {
	portName        string
	conn            serial.Serial
	opts            *SerialCodecOptions
	lastEOFTime     time.Time
	handsShaken     bool
	handsShakenLock sync.Mutex
	writeLock       sync.Mutex

	// Underlying serial port implementation allows concurrent Read/Write, but
	// calling Close while Read/Write is in progress results in a race. A
	// read-write lock fits perfectly for this case: for either Read or Write we
	// lock it for reading (RLock/RUnlock), but for Close we lock it for writing
	// (Lock/Unlock).
	closeLock sync.RWMutex
	isClosed  bool

	// The channel is closed when sending is allowed and not closed when it isn't.
	// xonLock should only be acquired to obtain the channel, it should not be held while waiting.
	xonChan chan interface{}
	xonLock sync.Mutex
}

func Serial(ctx context.Context, portName string, opts *SerialCodecOptions) (Codec, error) {
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

	// On Linux all the serial drivers are known to behave sensibly.
	// On other systems, some converters/drivers activate them which,
	// in case of ESP, may put device in reset mode.
	// If control pin polarity is inverted, then it also needs explicit setting.
	if runtime.GOOS != "linux" || opts.InvertedControlLines {
		bFalse := opts.InvertedControlLines
		s.SetRTSDTR(bFalse, bFalse)
	}

	// Flush any data that might be not yet read
	s.Flush()

	sc := &serialCodec{
		portName:    portName,
		opts:        opts,
		conn:        s,
		handsShaken: false,
		xonChan:     make(chan interface{}),
	}
	close(sc.xonChan) // Sending is initially allowed
	return newStreamConn(sc, true /* addChecksum */, opts.JunkHandler), nil
}

func (c *serialCodec) connRead(buf []byte) (read int, err error) {
	// Keep holding closeLock while Reading (see comment for closeLock)
	c.closeLock.RLock()
	defer c.closeLock.RUnlock()
	if !c.isClosed {
		read, err = c.conn.Read(buf)
		if err == nil {
			i := 0
			// Process and excise XON/XOFF symbols.
			for _, b := range buf[0:read] {
				switch b {
				case xoffChar:
					glog.V(3).Infof("XOFF")
					c.blockWrite()
				case xonChar:
					glog.V(3).Infof("XON")
					c.unblockWrite()
				default:
					buf[i] = b
					i++
				}
			}
			read = i
		}
		return read, err
	} else {
		return 0, io.EOF
	}
}

func (c *serialCodec) blockWrite() {
	c.xonLock.Lock()
	select {
	case <-c.xonChan:
		c.xonChan = make(chan interface{})
	default:
		// nothing - channel is open, sending is already blocked
	}
	c.xonLock.Unlock()
}

func (c *serialCodec) unblockWrite() {
	c.xonLock.Lock()
	select {
	case <-c.xonChan:
		// nothing - channel is closed, sending is already allowed
	default:
		close(c.xonChan)
	}
	c.xonLock.Unlock()
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
	c.isClosed = true
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

func (c *serialCodec) WriteWithContext(ctx context.Context, b []byte) (written int, err error) {
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
		case <-ctx.Done():
			return 0, ctx.Err()
		default:
		}
	}
	// Device is ready, send data.
	// We start with writes unblocked, since we just had a successful sync.
	c.unblockWrite()
	chunkSize := c.opts.SendChunkSize
	if chunkSize > 0 {
		for i := 0; i < len(b); i += chunkSize {
			// Check SW flos control first
			c.xonLock.Lock()
			xonChan := c.xonChan
			c.xonLock.Unlock()
			select {
			case <-xonChan:
				// channel is closed, sending is allowed
			case <-ctx.Done():
				return written, ctx.Err()
			}
			n, err := c.connWrite(b[i:min(i+chunkSize, len(b))])
			written += n
			if err != nil {
				c.Close()
				return written, errors.Trace(err)
			}
			glog.V(4).Infof("sent %d [%s]", n, string(b[i:i+n]))
			time.Sleep(c.opts.SendChunkDelay)
		}
	} else {
		for written < len(b) {
			n, err := c.connWrite(b[written:])
			glog.V(4).Infof("sent %d [%s]", n, string(b[written:written+n]))
			written += n
			if err != nil {
				c.Close()
				return written, errors.Trace(err)
			}
		}
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

func (c *serialCodec) SetOptions(opts *Options) error {
	c.opts = &opts.Serial
	return nil
}

func min(a, b int) int {
	if a < b {
		return a
	} else {
		return b
	}
}
