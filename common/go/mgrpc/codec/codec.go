package codec

import (
	"context"
	"crypto/x509"
	"io"

	"github.com/cesanta/errors"

	"cesanta.com/common/go/mgrpc/frame"
)

// Codec represents a transport for clubby frames.
type Codec interface {
	// Recv returns the next incoming frame.
	Recv(context.Context) (*frame.FrameV1V2, error)
	// Send sends the frame to the remote peer.
	Send(context.Context, *frame.FrameV1V2) error
	// Close closes the channel.
	Close()
	// CloseNotify() returns a channel that will be closed once the underlying channel has been closed.
	CloseNotify() <-chan struct{}
	// Can accept this many outgoing frames. 0 means no frames can be sent, values < 0 mean no limit.
	MaxNumFrames() int
	// Info() returns information about underlying connection.
	Info() ConnectionInfo
}

// ConnectionInfo provides information about the connection.
type ConnectionInfo struct {
	// TLS indicates if the connection uses TLS or not.
	TLS bool
	// RemoteAddr is the address of the remote peer.
	RemoteAddr string
	// PeerCertificates is the certificate chain presented by the peer.
	PeerCertificates []*x509.Certificate
}

// IsEOF returns true when err means "end of file".
func IsEOF(err error) bool {
	return errors.Cause(err) == io.EOF
}
