package dev

import (
	"crypto/tls"
	"time"

	"context"

	"cesanta.com/common/go/mgrpc"
	"cesanta.com/common/go/mgrpc/codec"
	"cesanta.com/common/go/ourjson"
	fwconfig "cesanta.com/fw/defs/config"
	fwfilesystem "cesanta.com/fw/defs/fs"
	fwsys "cesanta.com/fw/defs/sys"
	"cesanta.com/mos/rpccreds"
	"github.com/cesanta/errors"
	"github.com/golang/glog"
)

const (
	// we use empty destination so that device will receive the frame over uart
	// and handle it
	debugDevId = ""

	confOpTimeout  = 10 * time.Second
	confOpAttempts = 3
)

type DevConn struct {
	c           *Client
	ConnectAddr string
	RPC         mgrpc.MgRPC
	Dest        string
	Reconnect   bool
	codecOpts   codec.Options

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

func (c *Client) CreateDevConnWithOpts(ctx context.Context, connectAddr string, reconnect bool, tlsConfig *tls.Config, codecOpts *codec.Options) (*DevConn, error) {

	dc := &DevConn{c: c, ConnectAddr: connectAddr, Dest: debugDevId}

	err := dc.ConnectWithOpts(ctx, reconnect, tlsConfig, codecOpts)
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
		ctx2, cancel := context.WithTimeout(ctx, confOpTimeout)
		defer cancel()
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

	err = devConfRaw.UnmarshalIntoUseNumber(&devConf.data)
	if err != nil {
		return nil, errors.Trace(err)
	}

	return &devConf, nil
}

func (dc *DevConn) SetConfig(ctx context.Context, devConf *DevConf) error {
	attempts := confOpAttempts
	for {
		ctx2, cancel := context.WithTimeout(ctx, confOpTimeout)
		defer cancel()
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
	r, err := dc.CSys.GetInfo(ctx)
	return r, err
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
	return dc.RPC != nil && dc.RPC.IsConnected()
}

func (dc *DevConn) Connect(ctx context.Context, reconnect bool) error {
	return dc.ConnectWithOpts(ctx, reconnect, nil, nil)
}

func (dc *DevConn) ConnectWithOpts(ctx context.Context, reconnect bool, tlsConfig *tls.Config, codecOpts *codec.Options) error {
	var err error

	if dc.RPC != nil {
		return nil
	}

	if codecOpts != nil {
		dc.codecOpts = *codecOpts
	}

	opts := []mgrpc.ConnectOption{
		mgrpc.LocalID("mos"),
		mgrpc.Reconnect(reconnect),
		mgrpc.TlsConfig(tlsConfig),
		mgrpc.CodecOptions(dc.codecOpts),
	}

	dc.RPC, err = mgrpc.New(ctx, dc.ConnectAddr, opts...)
	if err != nil {
		return errors.Trace(err)
	}

	dc.CConf = fwconfig.NewClient(dc.RPC, debugDevId, rpccreds.GetRPCCreds)
	dc.CSys = fwsys.NewClient(dc.RPC, debugDevId, rpccreds.GetRPCCreds)
	dc.CFilesystem = fwfilesystem.NewClient(dc.RPC, debugDevId, rpccreds.GetRPCCreds)
	return nil
}
