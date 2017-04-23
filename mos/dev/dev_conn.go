package dev

import (
	"context"
	"crypto/tls"
	"time"

	"cesanta.com/common/go/mgrpc"
	"cesanta.com/common/go/ourjson"
	fwconfig "cesanta.com/fw/defs/config"
	fwfilesystem "cesanta.com/fw/defs/fs"
	fwsys "cesanta.com/fw/defs/sys"
	"github.com/cesanta/errors"
	"github.com/golang/glog"
)

const (
	// we use empty destination so that device will receive the frame over uart
	// and handle it
	debugDevId = ""

	confOpTimeout  = 5 * time.Second
	confOpAttempts = 3
)

type DevConn struct {
	c           *Client
	ConnectAddr string
	RPC         mgrpc.MgRPC
	Dest        string
	JunkHandler func(junk []byte)
	Reconnect   bool

	CConf       fwconfig.Service
	CSys        fwsys.Service
	CFilesystem fwfilesystem.Service
}

// CreateDevConn creates a direct connection to the device at a given address,
// which could be e.g. "serial:///dev/ttyUSB0", "serial://COM7",
// "tcp://192.168.0.10", etc.
func (c *Client) CreateDevConn(
	ctx context.Context, connectAddr string, reconnect bool,
) (*DevConn, error) {

	dc := &DevConn{c: c, ConnectAddr: connectAddr, Dest: debugDevId}

	err := dc.Connect(ctx, reconnect)
	if err != nil {
		return nil, errors.Trace(err)
	}

	return dc, nil
}

func (c *Client) CreateDevConnWithJunkHandler(ctx context.Context, connectAddr string, junkHandler func(junk []byte), reconnect bool, tlsConfig *tls.Config) (*DevConn, error) {

	dc := &DevConn{c: c, ConnectAddr: connectAddr, Dest: debugDevId}

	err := dc.ConnectWithJunkHandler(ctx, junkHandler, reconnect, tlsConfig)
	if err != nil {
		return nil, errors.Trace(err)
	}

	return dc, nil
}

func (dc *DevConn) GetConfig(ctx context.Context) (*DevConf, error) {
	var devConfRaw ourjson.RawMessage
	var err error
	attempts := confOpAttempts
	for {
		ctx2, _ := context.WithTimeout(ctx, confOpTimeout)
		devConfRaw, err = dc.CConf.Get(ctx2, &fwconfig.GetArgs{})
		if err != nil {
			attempts -= 1
			if attempts > 0 {
				glog.Warningf("Error: %s", err)
				continue
			}
			return nil, errors.Trace(err)
		}
		break
	}

	var devConf DevConf

	err = devConfRaw.UnmarshalInto(&devConf.data)
	if err != nil {
		return nil, errors.Trace(err)
	}

	return &devConf, nil
}

func (dc *DevConn) SetConfig(ctx context.Context, devConf *DevConf) error {
	attempts := confOpAttempts
	for {
		ctx2, _ := context.WithTimeout(ctx, confOpTimeout)
		err := dc.CConf.Set(ctx2, &fwconfig.SetArgs{
			Config: ourjson.DelayMarshaling(devConf.data),
		})
		if err != nil {
			attempts -= 1
			if attempts > 0 {
				glog.Warningf("Error: %s", err)
				continue
			}
			return errors.Trace(err)
		}
		break
	}

	return nil
}

func (dc *DevConn) GetInfo(ctx context.Context) (*fwsys.GetInfoResult, error) {
	return dc.CSys.GetInfo(ctx)
}

func (dc *DevConn) Disconnect(ctx context.Context) error {
	glog.V(2).Infof("Disconnecting from %s", dc.ConnectAddr)
	err := dc.RPC.Disconnect(ctx)
	// On Windows, closing a port and immediately opening it back is not going to
	// work, so here we just use a random 500ms timeout which seems to solve the
	// problem.
	//
	// Just in case though, we sleep not only on Windows, but on all platforms.
	time.Sleep(500 * time.Millisecond)

	// We need to set RPC to nil, in order for the subsequent call to Connect()
	// to work
	dc.RPC = nil
	return err
}

func (dc *DevConn) IsConnected() bool {
	return dc.RPC != nil
}

func (dc *DevConn) Connect(ctx context.Context, reconnect bool) error {
	if dc.JunkHandler == nil {
		dc.JunkHandler = func(junk []byte) {}
	}
	return dc.ConnectWithJunkHandler(ctx, dc.JunkHandler, reconnect, nil)
}

func (dc *DevConn) ConnectWithJunkHandler(ctx context.Context, junkHandler func(junk []byte), reconnect bool, tlsConfig *tls.Config) error {
	var err error

	if dc.RPC != nil {
		return nil
	}

	dc.JunkHandler = junkHandler
	dc.Reconnect = reconnect

	opts := []mgrpc.ConnectOption{
		mgrpc.LocalID("mos"),
		mgrpc.JunkHandler(junkHandler),
		mgrpc.Reconnect(reconnect),
		mgrpc.TlsConfig(tlsConfig),
	}

	dc.RPC, err = mgrpc.New(ctx, dc.ConnectAddr, opts...)
	if err != nil {
		return errors.Trace(err)
	}

	dc.CConf = fwconfig.NewClient(dc.RPC, debugDevId)
	dc.CSys = fwsys.NewClient(dc.RPC, debugDevId)
	dc.CFilesystem = fwfilesystem.NewClient(dc.RPC, debugDevId)
	return nil
}
