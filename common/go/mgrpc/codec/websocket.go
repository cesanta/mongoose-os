package codec

import (
	"context"
	"encoding/json"
	"flag"
	"fmt"
	"strings"
	"sync"

	"cesanta.com/common/go/mgrpc/frame"
	"github.com/cesanta/errors"
	"github.com/cesanta/ubjson"
	"github.com/golang/glog"
	"golang.org/x/net/websocket"
)

var (
	// TODO(imax): remove this hack once we have proper negotiation implemented.
	ubjsonInput = flag.Bool("clubby.xxx_ubjson_input", false, "HACK: treat incoming frames as UBJSON")
)

const (
	WSProtocol          = "clubby.cesanta.com"
	WSEncodingExtension = "clubby.cesanta.com-encoding"
)

type WSEncoding struct {
	In  []string
	Out []string
}

func (e WSEncoding) String() string {
	if len(e.In) == 0 && len(e.Out) == 0 {
		return ""
	}
	r := []string{WSEncodingExtension}
	if len(e.In) > 0 {
		r = append(r, "in="+strings.Join(e.In, "|"))
	}
	if len(e.Out) > 0 {
		r = append(r, "out="+strings.Join(e.Out, "|"))
	}
	return strings.Join(r, "; ")
}

func ParseEncodingExtension(s string) (WSEncoding, error) {
	knownEnc := map[string]bool{
		"json":   true,
		"ubjson": true,
	}
	parts := strings.Split(s, ";")
	if strings.TrimSpace(parts[0]) != WSEncodingExtension {
		return WSEncoding{}, errors.Errorf("invalid extension name %q, expected %q", strings.TrimSpace(parts[0]), WSEncodingExtension)
	}
	r := WSEncoding{}
	for _, p := range parts[1:] {
		kv := strings.Split(strings.TrimSpace(p), "=")
		if len(kv) != 2 {
			return WSEncoding{}, errors.Errorf("invalid parameter: %q", p)
		}
		switch kv[0] {
		case "in":
			for _, enc := range strings.Split(kv[1], "|") {
				if !knownEnc[enc] {
					return WSEncoding{}, errors.Errorf("unknown encoding: %q", enc)
				}
				r.In = append(r.In, enc)
			}
		case "out":
			for _, enc := range strings.Split(kv[1], "|") {
				if !knownEnc[enc] {
					return WSEncoding{}, errors.Errorf("unknown encoding: %q", enc)
				}
				r.Out = append(r.Out, enc)
			}
		default:
			return WSEncoding{}, errors.Errorf("unknown parameter: %q", kv[0])
		}
	}
	return r, nil
}

func jsonMarshal(v interface{}) ([]byte, byte, error) {
	if _, ok := v.(*frame.Frame); !ok {
		return nil, websocket.TextFrame, errors.Errorf("only clubby frames are supported, got %T", v)
	}
	b, err := frame.MarshalJSON(v.(*frame.Frame))
	return b, websocket.TextFrame, err
}

func jsonUnmarshal(data []byte, payloadType byte, v interface{}) error {
	f12, ok := v.(*frame.Frame)
	if !ok {
		return errors.Errorf("only clubby frames are supported, got %T", v)
	}
	f12.SizeHint = len(data)
	if payloadType != websocket.TextFrame && payloadType != websocket.BinaryFrame {
		return errors.Errorf("unknown frame type: %d", payloadType)
	}
	return errors.Trace(json.Unmarshal(data, f12))
}

func ubjsonMarshal(v interface{}) ([]byte, byte, error) {
	if _, ok := v.(*frame.Frame); !ok {
		return nil, websocket.TextFrame, errors.Errorf("only clubby frames are supported, got %T", v)
	}
	b, err := ubjson.Marshal(v)
	return b, websocket.BinaryFrame, errors.Trace(err)
}

func ubjsonUnmarshal(data []byte, payloadType byte, v interface{}) error {
	f12, ok := v.(*frame.Frame)
	if !ok {
		return errors.Errorf("only clubby frames are supported, got %T", v)
	}
	f12.SizeHint = len(data)
	// Text frames are not allowed, since they MUST contain valid UTF-8, which can
	// be not the case with UBJSON.
	if payloadType != websocket.BinaryFrame {
		return errors.Errorf("unknown frame type: %d", payloadType)
	}
	return errors.Trace(ubjson.Unmarshal(data, f12))
}

func combinedUnmarshal(data []byte, payloadType byte, v interface{}) error {
	f12, ok := v.(*frame.Frame)
	if !ok {
		return errors.Errorf("only clubby frames are supported, got %T", v)
	}
	f12.SizeHint = len(data)
	switch payloadType {
	case websocket.TextFrame:
		return errors.Trace(json.Unmarshal(data, v))
	case websocket.BinaryFrame:
		// Horrible hack (TM):
		// UBJSON doesn't currently support raw messages
		// and even if it would, we need to know later which
		// codec was the source of the byte array.
		// Let's just convert the whole message into json
		// and let the json decoder take from here.
		var tmp interface{}
		if err := ubjson.Unmarshal(data, &tmp); err != nil {
			return errors.Trace(err)
		}
		j, err := json.Marshal(&tmp)
		if err != nil {
			return errors.Trace(err)
		}
		return errors.Trace(json.Unmarshal(j, f12))
	default:
		return errors.Errorf("unknown frame type: %d", payloadType)
	}
}

func WebSocket(conn *websocket.Conn) Codec {
	r := &wsCodec{
		closeNotify: make(chan struct{}),
		conn:        conn,
		codec:       websocket.Codec{Marshal: jsonMarshal, Unmarshal: jsonUnmarshal},
	}
	exts := conn.Config().OutboundExtensions
	if conn.IsClientConn() {
		exts = conn.Config().InboundExtensions
	}
	for _, ext := range exts {
		if !strings.HasPrefix(ext, WSEncodingExtension+";") {
			continue
		}
		enc, err := ParseEncodingExtension(ext)
		if err != nil {
			glog.Errorf("Failed to parse encoding parameters: %+v", err)
			break
		}
		if len(enc.In) != 1 || len(enc.Out) != 1 {
			glog.Errorf("Want exactly one encoding for each direction: %#v (%q)", enc, ext)
			break
		}
		in, out := enc.In[0], enc.Out[0]
		if conn.IsClientConn() {
			// This is an outboud connection, so encoding extension comes from the
			// server, which means that we need to reverse directions.
			in, out = out, in
		}
		if in == "ubjson" {
			r.codec.Unmarshal = ubjsonUnmarshal
		}
		if out == "ubjson" {
			r.codec.Marshal = ubjsonMarshal
		}
	}
	if *ubjsonInput {
		r.codec.Unmarshal = combinedUnmarshal
	}
	return r
}

type wsCodec struct {
	closeNotify chan struct{}
	conn        *websocket.Conn
	closeOnce   sync.Once
	codec       websocket.Codec
}

func (c *wsCodec) String() string {
	addr := "unknown address"
	if c.conn.Request() != nil && c.conn.Request().RemoteAddr != "" {
		addr = c.conn.Request().RemoteAddr
	}
	return fmt.Sprintf("[wsCodec from %s]", addr)
}

func (c *wsCodec) Recv(ctx context.Context) (*frame.Frame, error) {
	var f12 frame.Frame
	if err := c.codec.Receive(c.conn, &f12); err != nil {
		glog.V(2).Infof("%s Recv(): %s", c, err)
		c.Close()
		return nil, errors.Trace(err)
	}
	return &f12, nil
}

func (c *wsCodec) Send(ctx context.Context, f12 *frame.Frame) error {
	return errors.Trace(c.codec.Send(c.conn, f12))
}

func (c *wsCodec) Close() {
	c.closeOnce.Do(func() {
		glog.V(1).Infof("Closing %s", c)
		close(c.closeNotify)
		c.conn.Close()
	})
}

func (c *wsCodec) CloseNotify() <-chan struct{} {
	return c.closeNotify
}

func (c *wsCodec) MaxNumFrames() int {
	return -1
}

func (c *wsCodec) Info() ConnectionInfo {
	r := ConnectionInfo{
		TLS:        c.conn.Request().TLS != nil,
		RemoteAddr: c.conn.Request().RemoteAddr,
	}
	if r.TLS {
		r.PeerCertificates = c.conn.Request().TLS.PeerCertificates
	}
	return r
}
