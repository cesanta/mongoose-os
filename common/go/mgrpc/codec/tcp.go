package codec

import "net"

type tcpCodec struct {
	conn net.Conn
}

func TCP(conn net.Conn) Codec {
	return StreamConn(&tcpCodec{
		conn: conn,
	}, nil)
}

func (c *tcpCodec) Read(b []byte) (n int, err error) {
	return c.conn.Read(b)
}

func (c *tcpCodec) Write(b []byte) (n int, err error) {
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
