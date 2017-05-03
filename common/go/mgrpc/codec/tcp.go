package codec

import (
	"context"
	"net"

	"github.com/cesanta/errors"
)

type tcpCodec struct {
	conn net.Conn
}

func TCP(conn net.Conn) Codec {
	return newStreamConn(&tcpCodec{
		conn: conn,
	}, false /* addChecksum */, nil)
}

func (c *tcpCodec) Read(b []byte) (n int, err error) {
	return c.conn.Read(b)
}

func (c *tcpCodec) WriteWithContext(ctx context.Context, b []byte) (n int, err error) {
	/* TODO(dfrank): use ctx */
	return c.conn.Write(b)
}

func (c *tcpCodec) Close() error {
	return c.conn.Close()
}

func (c *tcpCodec) RemoteAddr() string {
	return c.conn.RemoteAddr().String()
}

func (c *tcpCodec) PreprocessFrame(frameData []byte) (bool, error) {
	return false, nil
}

func (c *tcpCodec) SetOptions(opts *Options) error {
	return errors.NotImplementedf("SetOptions")
}
