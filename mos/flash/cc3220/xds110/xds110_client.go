package xds110

// Client for the TI XDS110 debug probe.

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"strings"

	"github.com/cesanta/errors"
	"github.com/golang/glog"
	"github.com/google/gousb"
)

const (
	vendorTI      = 0x0451
	productXDS110 = 0xbef3
	intfNum       = 0x02
	endpointOut   = 0x02
	endpointIn    = 0x83
	maxPacketLen  = 0x1000
)

type XDS110Client struct {
	ctx     *gousb.Context
	dev     *gousb.Device
	cfg     *gousb.Config
	intf    *gousb.Interface
	inEndp  *gousb.InEndpoint
	outEndp *gousb.OutEndpoint
}

func NewXDS110Client(serial string) (*XDS110Client, error) {
	ok := false
	xc := &XDS110Client{ctx: gousb.NewContext()}
	defer func() {
		if !ok {
			xc.Close()
		}
	}()
	devs, _ := xc.ctx.OpenDevices(func(dd *gousb.DeviceDesc) bool {
		result := (dd.Vendor == vendorTI && dd.Product == productXDS110)
		glog.V(1).Infof("Dev %s %s %t %+v", dd.Vendor, dd.Product, result, dd)
		return result
	})
	// OpenDevices may fail overall but still return results.
	for _, dev := range devs {
		if xc.dev != nil {
			dev.Close()
			continue
		}
		sn, err := dev.SerialNumber()
		glog.V(1).Infof("Dev %+v sn '%s' %s", dev, sn, err)
		if serial == "" || sn == serial {
			xc.dev = dev
		} else {
			dev.Close()
		}
	}
	if xc.dev == nil {
		if serial == "" {
			return nil, errors.Errorf("No XDS110 probe found")
		} else {
			return nil, errors.Errorf("XDS110 probe with S/N %s not found", serial)
		}
	}

	cfgNum, err := xc.dev.ActiveConfigNum()
	if err != nil {
		return nil, errors.Annotatef(err, "failed to get active config number of device %s", xc.dev)
	}
	xc.cfg, err = xc.dev.Config(cfgNum)
	if err != nil {
		return nil, errors.Annotatef(err, "failed to claim config %d of device %s", cfgNum, xc.dev)
	}
	xc.intf, err = xc.cfg.Interface(intfNum, 0)
	if err != nil {
		return nil, errors.Annotatef(err, "couldn't open interface %+v", xc.cfg)
	}

	xc.outEndp, err = xc.intf.OutEndpoint(endpointOut)
	if err != nil {
		return nil, errors.Annotatef(err, "couldn't open output endpoint")
	}

	xc.inEndp, err = xc.intf.InEndpoint(endpointIn)
	if err != nil {
		return nil, errors.Annotatef(err, "couldn't open input endpoint")
	}

	ok = true
	return xc, nil
}

type xds110Command uint8

const (
	headerByte uint8 = 0x2a
)

const (
	cmdEcho           = 0x00
	cmdConnect        = 0x01
	cmdDisconnect     = 0x02
	cmdGetVersionInfo = 0x03
	cmdSetTCLKDelay   = 0x04
	cmdSetTRST        = 0x05
	cmdGetTRST        = 0x06
	cmdSetSRST        = 0x0e
	cmdReadMemory     = 0x13
	cmdWriteMemory    = 0x14
)

func (xc *XDS110Client) GetSerialNumber() string {
	sn, _ := xc.dev.SerialNumber()
	// Sometimes serial contains trailing NULs. Not sure whose fault it is (libusb?), strip them.
	return strings.TrimRight(sn, "\x00")
}

func (xc *XDS110Client) doCommand(cmd xds110Command, argBuf *bytes.Buffer, respPayloadLen int) (*bytes.Buffer, error) {
	if xc.outEndp == nil {
		return nil, errors.Errorf("not connected")
	}
	if argBuf == nil {
		argBuf = bytes.NewBuffer(nil)
	}
	glog.V(2).Infof("=> %s %q", cmd, argBuf.Bytes())
	cmdBuf := bytes.NewBuffer([]byte{headerByte})
	binary.Write(cmdBuf, binary.LittleEndian, uint16(len(argBuf.Bytes()))+1)
	binary.Write(cmdBuf, binary.LittleEndian, &cmd)
	cmdBuf.Write(argBuf.Bytes())
	glog.V(4).Infof("=> (%d) %q", len(cmdBuf.Bytes()), cmdBuf.Bytes())
	_, err := xc.outEndp.Write(cmdBuf.Bytes())
	if err != nil {
		return nil, errors.Annotatef(err, "failed to send command")
	}
	resp := make([]byte, maxPacketLen)
	n, err := xc.inEndp.Read(resp)
	if err != nil {
		return nil, errors.Annotatef(err, "failed to read response")
	}
	glog.V(4).Infof("<= (%d) %q", n, resp[:n])
	if n < 7 {
		return nil, errors.Errorf("response is too short (%d) %q", n, resp[:n])
	}
	if resp[0] != headerByte {
		return nil, errors.Errorf("invalid response header")
	}
	respBuf := bytes.NewBuffer(resp[1:])
	var packetLen uint16
	binary.Read(respBuf, binary.LittleEndian, &packetLen)
	var status uint32
	binary.Read(respBuf, binary.LittleEndian, &status)
	glog.V(2).Infof("<= %d %q", status, resp[7:n])
	if status != 0 {
		return nil, errors.Errorf("error response: %d", status)
	}
	if n-3 != int(packetLen) {
		return nil, errors.Errorf("incomplete packet: expected %d, got %d", packetLen, n-3)
	}
	if int(packetLen) != respPayloadLen+4 {
		return nil, errors.Errorf("incomplete response: expected %d, got %d", respPayloadLen, packetLen-4)
	}
	return bytes.NewBuffer(resp[7:n]), nil
}

func (xc *XDS110Client) Echo(data []byte) error {
	if len(data) > 255 {
		return errors.Errorf("Echo: max 255 bytes, got %d", len(data))
	}
	ab := bytes.NewBuffer(nil)
	binary.Write(ab, binary.LittleEndian, uint32(len(data)&0xff))
	ab.Write(data)
	echoData, err := xc.doCommand(cmdEcho, ab, len(data))
	if err == nil && !bytes.Equal(echoData.Bytes(), data) {
		return errors.Errorf("invalid echo response: send %q, got %q", data, echoData.Bytes())
	}
	return errors.Annotatef(err, "Echo")
}

func (xc *XDS110Client) Connect() error {
	_, err := xc.doCommand(cmdConnect, nil, 0)
	return errors.Annotatef(err, "Connect")
}

func (xc *XDS110Client) Disconnect() error {
	_, err := xc.doCommand(cmdDisconnect, nil, 7)
	return errors.Annotatef(err, "Disconnect")
}

type XDS110VersionInfo struct {
	Version        uint32
	V1, V2, V3, V4 uint8
	HWVersion      uint16
}

func (xc *XDS110Client) GetVersionInfo() (*XDS110VersionInfo, error) {
	rb, err := xc.doCommand(cmdGetVersionInfo, nil, 6)
	if err != nil {
		return nil, errors.Annotatef(err, "GetVersionInfo")
	}
	vi := &XDS110VersionInfo{}
	binary.Read(rb, binary.LittleEndian, &vi.Version)
	vi.V1 = uint8(vi.Version >> 24)
	vi.V2 = uint8(vi.Version >> 16)
	vi.V3 = uint8(vi.Version >> 8)
	vi.V4 = uint8(vi.Version & 0xff)
	binary.Read(rb, binary.LittleEndian, &vi.HWVersion)
	return vi, nil
}

func (xc *XDS110Client) SetTCLKDelay(delay uint8) error {
	ab := bytes.NewBuffer(nil)
	binary.Write(ab, binary.LittleEndian, uint32(delay))
	_, err := xc.doCommand(cmdSetTCLKDelay, ab, 0)
	return errors.Annotatef(err, "SetTCLKDelay")
}

func boolToByte(b bool) byte {
	if b {
		return 1
	} else {
		return 0
	}
}

func (xc *XDS110Client) SetTRST(rstOn bool) error {
	ab := bytes.NewBuffer([]byte{boolToByte(rstOn)})
	_, err := xc.doCommand(cmdSetTRST, ab, 0)
	return errors.Annotatef(err, "SetTRST")
}

func (xc *XDS110Client) SetSRST(rstOn bool) error {
	ab := bytes.NewBuffer([]byte{boolToByte(!rstOn)})
	_, err := xc.doCommand(cmdSetSRST, ab, 0)
	return errors.Annotatef(err, "SetSRST")
}

func (xc *XDS110Client) Close() error {
	if xc.intf != nil {
		xc.inEndp = nil
		xc.outEndp = nil
		xc.intf.Close()
		xc.intf = nil
	}
	if xc.cfg != nil {
		xc.cfg.Close()
		xc.cfg = nil
	}
	if xc.dev != nil {
		xc.dev.Close()
		xc.dev = nil
	}
	if xc.ctx != nil {
		xc.ctx.Close()
		xc.ctx = nil
	}
	return nil
}

// TODO(rojer): Use stringer when it actually works.
func (cmd xds110Command) String() string {
	switch cmd {
	case cmdEcho:
		return fmt.Sprintf("Echo(0x%x)", uint8(cmd))
	case cmdConnect:
		return fmt.Sprintf("Connect(0x%x)", uint8(cmd))
	case cmdDisconnect:
		return fmt.Sprintf("Disconnect(0x%x)", uint8(cmd))
	case cmdGetVersionInfo:
		return fmt.Sprintf("GetVersionInfo(0x%x)", uint8(cmd))
	case cmdSetTCLKDelay:
		return fmt.Sprintf("SetTCLKDelay(0x%x)", uint8(cmd))
	case cmdSetTRST:
		return fmt.Sprintf("SetTRST(0x%x)", uint8(cmd))
	case cmdGetTRST:
		return fmt.Sprintf("GetTRST(0x%x)", uint8(cmd))
	case cmdSetSRST:
		return fmt.Sprintf("SetSRST(0x%x)", uint8(cmd))
	case cmdReadMemory:
		return fmt.Sprintf("ReadMemory(0x%x)", uint8(cmd))
	case cmdWriteMemory:
		return fmt.Sprintf("WriteMemory(0x%x)", uint8(cmd))
	default:
		return fmt.Sprintf("?(0x%x)", uint8(cmd))
	}
}
