package rom_client

import (
	"bytes"
	"encoding/binary"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"io"
	"math"
	"time"

	"cesanta.com/mos/flash/common"
	"cesanta.com/mos/flash/esp"
	"cesanta.com/mos/flash/esp32"
	"cesanta.com/mos/flash/esp8266"
	"github.com/cesanta/errors"
	"github.com/cesanta/go-serial/serial"
	"github.com/golang/glog"
)

const (
	numConnectAttempts = 10

	syncValue         = 0x20120707
	memWriteBlockSize = 0x1000
)

type romPacketType uint8

const (
	pktReq  romPacketType = 0x00
	pktResp               = 0x01
)

type romCmd uint8

const (
	cmdMemWriteStart  romCmd = 0x05
	cmdMemWriteFinish        = 0x06
	cmdMemWriteBlock         = 0x07
	cmdSync                  = 0x08
	cmdWriteReg              = 0x09
	cmdReadReg               = 0x0a
)

type ROMClient struct {
	ct        esp.ChipType
	sc        serial.Serial
	sd        serial.Serial
	srw       *common.SLIPReaderWriter
	connected bool
	inverted  bool
}

type romResponse struct {
	cmd       romCmd
	value     uint32
	ok        bool
	lastError uint8
	body      []byte
}

func ConnectToROM(ct esp.ChipType, opts *esp.FlashOpts) (*ROMClient, error) {
	commonOpts := serial.OpenOptions{
		BaudRate:              opts.ROMBaudRate,
		DataBits:              8,
		ParityMode:            serial.PARITY_NONE,
		StopBits:              1,
		InterCharacterTimeout: 200.0,
	}
	scOpts := commonOpts
	scOpts.PortName = opts.ControlPort
	common.Reportf("Opening %s @ %d...", scOpts.PortName, opts.ROMBaudRate)
	sc, err := serial.Open(scOpts)
	if err != nil {
		return nil, errors.Annotate(err, "failed to open control port")
	}
	sd := sc
	if opts.DataPort != "" {
		sdOpts := commonOpts
		sdOpts.PortName = opts.DataPort
		common.Reportf("Opening %s...", sdOpts.PortName)
		sd, err = serial.Open(sdOpts)
		if err != nil {
			sc.Close()
			return nil, errors.Annotate(err, "failed to open data port")
		}
	}
	rc, err := NewROMClient(ct, sc, sd, opts.InvertedControlLines)
	if err != nil {
		sc.Close()
		sd.Close()
		return nil, errors.Annotatef(err, "failed to create ROM client")
	}
	return rc, nil
}

func NewROMClient(chipType esp.ChipType, sc, sd serial.Serial, inverted bool) (*ROMClient, error) {
	rc := &ROMClient{
		ct:       chipType,
		sc:       sc,
		sd:       sd,
		srw:      common.NewSLIPReaderWriter(sd),
		inverted: inverted,
	}
	if err := rc.connect(); err != nil {
		return nil, errors.Annotatef(err, "failed to connect to ROM")
	}
	return rc, nil
}

func (rc *ROMClient) DataPort() serial.Serial {
	return rc.sd
}

func (rc *ROMClient) connect() error {
	rc.connected = false
	rc.sd.SetReadTimeout(200 * time.Millisecond)
	for i := 1; i <= numConnectAttempts; i++ {
		is := ""
		if rc.inverted {
			is = " (inverted)"
		}
		common.Reportf("Connecting to %s ROM, attempt %d of %d%s...", rc.ct, i, numConnectAttempts, is)
		mFalse := rc.inverted
		mTrue := !rc.inverted
		rc.sc.SetRTSDTR(mTrue, mFalse)
		// If you are wondering why ESP32 delays are like this, read this and weep:
		// https://github.com/espressif/esptool/blob/96698a3da9acc6e357741663830f97524b688ade/esptool.py#L286
		if rc.ct == esp.ChipESP8266 {
			time.Sleep(50 * time.Millisecond)
		} else {
			time.Sleep(1200 * time.Millisecond)
		}
		rc.sc.SetRTSDTR(mFalse, mTrue)
		if rc.ct == esp.ChipESP8266 {
			time.Sleep(100 * time.Millisecond)
		} else {
			time.Sleep(400 * time.Millisecond)
		}
		rc.sc.SetRTSDTR(mFalse, mFalse)
		err := rc.sync()
		if err == nil {
			rc.connected = true
			cd, err := rc.GetChipDescr()
			if err != nil {
				return errors.Annotatef(err, "failed to read chip type")
			}
			common.Reportf("  Connected, chip: %s", cd)
			return nil
		} else {
			glog.V(1).Infof("Sync #%d failed: %s", i, err)
		}
	}
	return errors.Errorf("failed to connect to %s ROM", rc.ct)
}

func (rc *ROMClient) Disconnect() {
	rc.connected = false
	rc.sc.Close()
	if rc.sd != rc.sc {
		rc.sd.Close()
	}
	rc.sc = nil
	rc.sd = nil
}

func (rc *ROMClient) GetChipDescr() (string, error) {
	switch rc.ct {
	case esp.ChipESP8266:
		return esp8266.GetChipDescr(rc)
	case esp.ChipESP32:
		return esp32.GetChipDescr(rc)
	}
	return rc.ct.String(), nil
}

func (rc *ROMClient) sendCommand(cmd romCmd, arg []byte, csum uint8) error {
	glog.V(3).Infof("=> {cmd:%s csum:%d arg(%d):%q}", cmd, csum, len(arg), common.LimitStr(arg, 32))
	cmdBuf := bytes.NewBuffer([]byte{byte(pktReq), byte(cmd)})
	binary.Write(cmdBuf, binary.LittleEndian, uint16(len(arg)))
	binary.Write(cmdBuf, binary.LittleEndian, uint32(csum)) // Yes, uint8 -> uint32
	cmdBuf.Write(arg)
	_, err := rc.srw.Write(cmdBuf.Bytes())
	return err
}

func (rc *ROMClient) recvResponse() (*romResponse, error) {
	data := make([]byte, 10000)
	n, err := rc.srw.Read(data)
	if err != nil {
		return nil, errors.Annotate(err, "error reading response")
	}
	if n < 8 {
		return nil, errors.Errorf("response is too short (%d) %q", n, data[:n])
	}
	respBuf := bytes.NewBuffer(data[:n])
	var pktType uint8
	binary.Read(respBuf, binary.LittleEndian, &pktType)
	if pktType != pktResp {
		return nil, errors.Errorf("not a response (%d) %q", n, data[:n])
	}
	r := &romResponse{}
	binary.Read(respBuf, binary.LittleEndian, &r.cmd)
	var bodyLen uint16
	binary.Read(respBuf, binary.LittleEndian, &bodyLen)
	binary.Read(respBuf, binary.LittleEndian, &r.value)
	r.body = data[8:n]
	if len(r.body) != int(bodyLen) {
		return nil, errors.Errorf("unexpected body length: %d vs %d (%q)", len(r.body), bodyLen, data[:n])
	}
	var statusLen uint16
	switch rc.ct {
	case esp.ChipESP8266:
		statusLen = 2
	case esp.ChipESP32:
		statusLen = 4
	}
	if bodyLen == statusLen {
		r.ok = (r.body[0] == 0)
		r.lastError = r.body[1]
	}
	glog.V(3).Infof("<= {cmd:%s value:%d ok:%t lastError:%d body(%d):%q}", r.cmd, r.value, r.ok, r.lastError, len(r.body), common.LimitStr(r.body, 32))
	return r, nil
}

func (rc *ROMClient) simpleCmdResponse(cmd romCmd, arg []byte, csum uint8, timeout time.Duration) (*romResponse, error) {
	if !rc.connected {
		return nil, errors.New("not connected")
	}
	rc.sd.SetReadTimeout(timeout)
	err := rc.sendCommand(cmd, arg, csum)
	if err != nil {
		return nil, errors.Annotatef(err, "error sending cmd %d", cmd)
	}
	r, err := rc.recvResponse()
	if err != nil {
		return nil, errors.Annotatef(err, "error reading response to cmd %d", cmd)
	}
	if r.cmd != cmd {
		return nil, errors.Errorf("command mismatch: %d vs %d", r.cmd, cmd)
	}
	if !r.ok {
		return nil, errors.Errorf("command %d failure: error code %d", cmd, r.lastError)
	}
	return r, nil
}

func (rc *ROMClient) simpleCmd(cmd romCmd, arg []byte, csum uint8, timeout time.Duration) error {
	_, err := rc.simpleCmdResponse(cmd, arg, csum, timeout)
	return err
}

func (rc *ROMClient) trySync() error {
	argBuf := bytes.NewBuffer(nil)
	binary.Write(argBuf, binary.LittleEndian, uint32(syncValue))
	for i := 0; i < 32; i++ {
		argBuf.Write([]byte{0x55})
	}
	rc.sd.Flush()
	if err := rc.sendCommand(cmdSync, argBuf.Bytes(), 0); err != nil {
		return errors.Trace(err)
	}
	for i := 1; i <= 8; i++ {
		var r *romResponse
		for {
			var err error
			r, err = rc.recvResponse()
			if err != nil {
				// When reading first sync response we may need to skip over some junk
				// before we find the response
				if i == 1 && errors.Cause(err) != io.EOF {
					continue
				} else {
					return errors.Annotatef(err, "error reading sync response #%d", i)
				}
			} else {
				break
			}
		}
		if r.cmd != cmdSync {
			return errors.Errorf("invalid sync response #%d (cmd %d)", i, r.cmd)
		}
	}
	return nil
}

func (rc *ROMClient) sync() error {
	var err error
	// Usually there is no response to the first command, and the second is successful.
	for i := 0; i < 2; i++ {
		err = rc.trySync()
		if err == nil {
			return nil
		}
	}
	return errors.Trace(err)
}

func checksum(data []byte) (cs uint8) {
	cs = 0xef
	for _, b := range data {
		cs ^= uint8(b)
	}
	return cs
}

// MemWrite writes data to address addr and jumps to jumpAddr (if non-zero).
func (rc *ROMClient) MemWrite(data []byte, addr, jumpAddr uint32) error {
	if !rc.connected {
		return errors.New("not connected")
	}
	glog.V(2).Infof("memWrite(0x%08x, %d, 0x%08x)", addr, len(data), jumpAddr)
	numBlocks := int(math.Ceil(float64(len(data)) / memWriteBlockSize))
	argBuf := bytes.NewBuffer(nil)
	binary.Write(argBuf, binary.LittleEndian, uint32(len(data)))
	binary.Write(argBuf, binary.LittleEndian, uint32(numBlocks))
	binary.Write(argBuf, binary.LittleEndian, uint32(memWriteBlockSize))
	binary.Write(argBuf, binary.LittleEndian, uint32(addr))
	if err := rc.simpleCmd(cmdMemWriteStart, argBuf.Bytes(), 0, 100*time.Millisecond); err != nil {
		return errors.Annotatef(err, "failed to start mem write")
	}
	for i := 0; i < numBlocks; i++ {
		argBuf.Reset()
		offset := i * memWriteBlockSize
		end := offset + memWriteBlockSize
		if end > len(data) {
			end = len(data)
		}
		block := data[offset:end]
		binary.Write(argBuf, binary.LittleEndian, uint32(len(block)))
		binary.Write(argBuf, binary.LittleEndian, uint32(i))
		binary.Write(argBuf, binary.LittleEndian, uint32(0))
		binary.Write(argBuf, binary.LittleEndian, uint32(0))
		argBuf.Write(block)
		if err := rc.simpleCmd(cmdMemWriteBlock, argBuf.Bytes(), checksum(block), 500*time.Millisecond); err != nil {
			return errors.Annotatef(err, "failed to write block #%d", i)
		}
	}
	argBuf.Reset()
	if jumpAddr == 0 {
		binary.Write(argBuf, binary.LittleEndian, uint32(1))
		binary.Write(argBuf, binary.LittleEndian, uint32(0))
	} else {
		binary.Write(argBuf, binary.LittleEndian, uint32(0))
		binary.Write(argBuf, binary.LittleEndian, uint32(jumpAddr))
	}
	if err := rc.simpleCmd(cmdMemWriteFinish, argBuf.Bytes(), 0, 100*time.Millisecond); err != nil {
		return errors.Annotatef(err, "failed to finish mem write")
	}
	return nil
}

type stubInfo struct {
	NumParams   int    `json:"num_params"`
	ParamsStart uint32 `json:"params_start"`
	CodeStart   uint32 `json:"code_start"`
	CodeHex     string `json:"code"`
	Entry       uint32 `json:"entry"`
	DataStart   uint32 `json:"data_start"`
	DataHex     string `json:"data"`
}

// RunStub unwraps and runs a stub created by
// https://github.com/cesanta/mongoose-os/tree/master/common/platforms/esp8266/stubs
func (rc *ROMClient) RunStub(stubJSON []byte, params []uint32) error {
	if !rc.connected {
		return errors.New("not connected")
	}
	var si stubInfo
	err := json.Unmarshal(stubJSON, &si)
	if err != nil {
		return errors.Annotatef(err, "failed to unwrap stub")
	}
	code, err := hex.DecodeString(si.CodeHex)
	if err != nil {
		return errors.Annotatef(err, "failed to decode code section")
	}
	data, err := hex.DecodeString(si.DataHex)
	if err != nil {
		return errors.Annotatef(err, "failed to decode data section")
	}
	if false && len(params) != si.NumParams {
		return errors.Errorf("expected %d params, got %d", si.NumParams, len(params))
	}
	glog.V(2).Infof("RunStub: params: %d @ 0x%08x; code: %d @ 0x%08x, entry @ %08x; data: %d @ 0x%08x",
		si.NumParams, si.ParamsStart, len(code), si.CodeStart, si.Entry, len(data), si.DataStart)

	if si.NumParams > 0 {
		buf := bytes.NewBuffer(nil)
		for _, p := range params {
			binary.Write(buf, binary.LittleEndian, uint32(p))
		}
		if err := rc.MemWrite(buf.Bytes(), si.ParamsStart, 0); err != nil {
			return errors.Annotatef(err, "failed to write params")
		}
	}

	if len(data) > 0 {
		if err := rc.MemWrite(data, si.DataStart, 0); err != nil {
			return errors.Annotatef(err, "failed to write data")
		}
	}

	if len(code) > 0 {
		if err := rc.MemWrite(code, si.CodeStart, si.Entry); err != nil {
			return errors.Annotatef(err, "failed to write code")
		}
	}

	return nil
}

func (rc *ROMClient) BootFirmware() error {
	if !rc.connected {
		return errors.New("not connected")
	}
	mFalse := rc.inverted
	mTrue := !rc.inverted
	rc.sc.SetDTR(mFalse) // Pull up GPIO0
	rc.sc.SetRTS(mTrue)  // Reset
	time.Sleep(50 * time.Millisecond)
	rc.sc.SetRTS(mFalse) // Boot
	return nil
}

func (rc *ROMClient) ReadReg(reg uint32) (uint32, error) {
	argBuf := bytes.NewBuffer(nil)
	binary.Write(argBuf, binary.LittleEndian, reg)
	r, err := rc.simpleCmdResponse(cmdReadReg, argBuf.Bytes(), 0, 100*time.Millisecond)
	if err != nil {
		return 0, errors.Annotatef(err, "failed to read reg 0x%08x", reg)
	}
	glog.V(3).Infof("ReadReg(0x%08x) => 0x%08x", reg, r.value)
	return r.value, nil
}

func (rc *ROMClient) WriteReg(reg, value uint32) error {
	argBuf := bytes.NewBuffer(nil)
	binary.Write(argBuf, binary.LittleEndian, reg)
	binary.Write(argBuf, binary.LittleEndian, value)
	binary.Write(argBuf, binary.LittleEndian, uint32(0xffffffff)) // mask
	binary.Write(argBuf, binary.LittleEndian, uint32(0))          // delayMicros
	err := rc.simpleCmd(cmdWriteReg, argBuf.Bytes(), 0, 100*time.Millisecond)
	if err != nil {
		return errors.Annotatef(err, "failed to write reg 0x%08x", reg)
	}
	glog.V(3).Infof("WriteReg(0x%08x, 0x%08x)", reg, value)
	return nil
}

// TODO(rojer): Use stringer when it actually works.
func (cmd romCmd) String() string {
	switch cmd {
	case cmdMemWriteStart:
		return fmt.Sprintf("MemWriteStart(%d)", cmd)
	case cmdMemWriteFinish:
		return fmt.Sprintf("MemWriteFinish(%d)", cmd)
	case cmdMemWriteBlock:
		return fmt.Sprintf("MemWriteBlock(%d)", cmd)
	case cmdSync:
		return fmt.Sprintf("Sync(%d)", cmd)
	case cmdWriteReg:
		return fmt.Sprintf("WriteReg(%d)", cmd)
	case cmdReadReg:
		return fmt.Sprintf("ReadReg(%d)", cmd)
	default:
		return fmt.Sprintf("?(%d)", cmd)
	}
}
