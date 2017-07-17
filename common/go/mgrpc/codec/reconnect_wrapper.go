package codec

import (
	"fmt"
	"golang.org/x/net/context"
	"sync"
	"time"

	"cesanta.com/common/go/mgrpc/frame"

	"github.com/cesanta/errors"
	"github.com/golang/glog"
)

type ConnectFunc func(addr string) (Codec, error)

type reconnectWrapperCodec struct {
	addr    string
	connect ConnectFunc

	lock        sync.Mutex
	conn        Codec
	connEstd    chan struct{}
	nextAttempt time.Time

	closeNotifier chan struct{}
	closeOnce     sync.Once
}

func NewReconnectWrapperCodec(addr string, connect ConnectFunc) Codec {
	rwc := &reconnectWrapperCodec{
		addr:          addr,
		connect:       connect,
		nextAttempt:   time.Now(),
		connEstd:      make(chan struct{}), // Closed when a new connection is established.
		closeNotifier: make(chan struct{}),
	}
	go rwc.maintainConnection()
	return rwc
}

func (rwc *reconnectWrapperCodec) stringLocked() string {
	var connStatus string
	switch {
	case rwc.conn != nil:
		connStatus = "connected"
	case rwc.nextAttempt.After(time.Now()):
		connStatus = fmt.Sprintf("connect in %.2fs", rwc.nextAttempt.Sub(time.Now()).Seconds())
	default:
		connStatus = "connecting..."
	}
	return fmt.Sprintf("[reconnectWrapperCodec to %s; %s]", rwc.addr, connStatus)
}

func (rwc *reconnectWrapperCodec) String() string {
	rwc.lock.Lock()
	defer rwc.lock.Unlock()
	return rwc.stringLocked()
}

func (rwc *reconnectWrapperCodec) maintainConnection() {
	for {
		rwc.lock.Lock()
		conn := rwc.conn
		rwc.lock.Unlock()
		if conn != nil {
			select {
			case <-rwc.closeNotifier:
				glog.V(1).Infof("closed, stopping reconnect thread")
				return
			case <-conn.CloseNotify():
				glog.Errorf("%s Connection closed", rwc)
				rwc.lock.Lock()
				rwc.conn = nil
				rwc.connEstd = make(chan struct{})
				rwc.lock.Unlock()
			}
		}
		glog.V(2).Infof("Next attempt: %s, Now: %s, Diff: %s", rwc.nextAttempt, time.Now(), rwc.nextAttempt.Sub(time.Now()))
		select {
		case <-rwc.closeNotifier:
			glog.V(1).Infof("closed, stopping reconnect thread")
			return
		case <-time.After(rwc.nextAttempt.Sub(time.Now())):
		}

		glog.V(1).Infof("%s connecting", rwc)
		conn, err := rwc.connect(rwc.addr)
		rwc.lock.Lock()
		// TODO(rojer): implement backoff.
		rwc.nextAttempt = time.Now().Add(2 * time.Second)
		if err != nil {
			glog.Errorf("%s connection error: %+v", rwc.stringLocked(), err)
			rwc.lock.Unlock()
			continue
		}
		rwc.conn = conn
		glog.Infof("%s connected", rwc.stringLocked())
		close(rwc.connEstd)
		rwc.lock.Unlock()
	}
}

func (rwc *reconnectWrapperCodec) getConn(ctx context.Context) (Codec, error) {
	for {
		rwc.lock.Lock()
		conn, connEstd := rwc.conn, rwc.connEstd
		rwc.lock.Unlock()
		if conn != nil {
			return conn, nil
		}
		select {
		case <-ctx.Done():
			return nil, errors.Trace(ctx.Err())
		case <-connEstd:
		}
	}
}

func (rwc *reconnectWrapperCodec) closeConn() {
	rwc.lock.Lock()
	defer rwc.lock.Unlock()
	if rwc.conn != nil {
		rwc.conn.Close()
		rwc.conn = nil
	}
}

func (rwc *reconnectWrapperCodec) Recv(ctx context.Context) (*frame.Frame, error) {
	for {
		conn, err := rwc.getConn(ctx)
		if err != nil {
			return nil, errors.Trace(err)
		}
		frame, err := conn.Recv(ctx)
		if err != nil {
			glog.V(1).Infof("%s recv error: %s, eof? %v", rwc, err, IsEOF(err))
		}
		switch {
		case err == nil:
			return frame, nil
		case IsEOF(err):
			rwc.closeConn()
			return nil, errors.Trace(err)
		default:
			return nil, errors.Trace(err)
		}
	}
}

func (rwc *reconnectWrapperCodec) Send(ctx context.Context, frame *frame.Frame) error {
	for {
		conn, err := rwc.getConn(ctx)
		if err != nil {
			return errors.Trace(err)
		}
		err = conn.Send(ctx, frame)
		if err != nil {
			glog.V(1).Infof("%s send error: %s", rwc, err)
			rwc.closeConn()
			continue
		}
		return nil
	}
}

func (rwc *reconnectWrapperCodec) Close() {
	rwc.closeOnce.Do(func() {
		if rwc.conn != nil {
			rwc.conn.Close()
		}
		close(rwc.closeNotifier)
	})
}

func (rwc *reconnectWrapperCodec) CloseNotify() <-chan struct{} {
	return rwc.closeNotifier
}

func (rwc *reconnectWrapperCodec) MaxNumFrames() int {
	return -1
}

func (rwc *reconnectWrapperCodec) Info() ConnectionInfo {
	ctx, cancel := context.WithCancel(context.Background())
	cancel() // cancel now, we don't want to wait.
	c, err := rwc.getConn(ctx)
	if err != nil {
		return ConnectionInfo{RemoteAddr: rwc.addr}
	}
	return c.Info()
}

func (rwc *reconnectWrapperCodec) SetOptions(opts *Options) error {
	rwc.lock.Lock()
	defer rwc.lock.Unlock()
	if rwc.conn != nil {
		return rwc.conn.SetOptions(opts)
	}
	return errors.Errorf("not connected")
}
