package cc3200

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"io"
	"time"

	"cesanta.com/mos/flash/common"
	"github.com/cesanta/errors"
	"github.com/cesanta/go-serial/serial"
	"github.com/golang/glog"
)

type loaderCmd uint8

// Some of these are documented in the SimpleLink CC3100/CC3200 Embedded Programming Application Note
// http://www.ti.com/tool/embedded-programming
const (
	cmdStartUpload  loaderCmd = 0x21
	cmdFinishUpload           = 0x22
	cmdGetStatus              = 0x23
	cmdUploadChunk            = 0x24

	cmdGetStorageList = 0x27
	cmdFormatSLFS     = 0x28

	cmdEraseFile      = 0x2e
	cmdGetVersionInfo = 0x2f

	cmdSwitchUARTtoAppsMCU = 0x33
)

const (
	ChipTypeCC3200      = 0x10
	fsBlockSize         = 0x1000
	uartSwitchDelay     = 26666667 // 1 second
	FileSignatureLength = 0x100
	fileUploadBlockSize = 0x1000
)

type fileOpenMode uint32

const (
	openModeCreateifNotExist fileOpenMode = 0x3000
	openModeSecure                        = 0x20000
)

type fileSignature []byte

type romStatus byte

const (
	statusOk          romStatus = 0x40
	statusUnknownCmd            = 0x41
	statusInvalidCmd            = 0x42
	statusInvalidAddr           = 0x43
	statusFlashFail             = 0x44
)

type ROMClient struct {
	s serial.Serial
}

func NewROMClient(s serial.Serial, dc DeviceControl) (*ROMClient, error) {
	rc := &ROMClient{s: s}
	common.Reportf("Connecting to boot loader..")
	err := rc.connect(dc)
	if err != nil {
		return nil, errors.Errorf(
			"Unable to communicate with the boot loader. " +
				"Please make sure SOP2 is high and reset the device. " +
				"If you are using a LAUNCHXL board, the SOP2 jumper should " +
				"be closed")
	}
	vi, err := rc.GetVersionInfo()
	if err != nil {
		return nil, err
	}
	// CC3100 built into CC3200 reports chiptype 0x10 but boot loader bersion 2.0.4.0.
	// So, if the version we get is 2.0.4.0, it means we're already talking to the NWP.
	if vi.ChipType&ChipTypeCC3200 != 0 && vi.BootLoaderVersion != 0x02000400 {
		common.Reportf("  Main boot loader v%s", vi.BootLoaderVersionString())
		common.Reportf("Switching to NWP...")
		switch {
		case vi.BootLoaderVersion < 0x02010300:
			return nil, errors.Errorf("unsupported boot loader version (%s)", vi.BootLoaderVersionString())
		case vi.BootLoaderVersion == 0x02010300:
			return nil, errors.Errorf("unsupported boot loader version (%s)", vi.BootLoaderVersionString())
		case vi.BootLoaderVersion >= 0x02010400:
			if err = rc.SwitchUARTtoAppsMCU(); err != nil {
				return nil, errors.Annotatef(err, "failed to switch to apps mode")
			}
			vi, err = rc.GetVersionInfo()
			if err != nil {
				return nil, err
			}
		}
	}
	sb, err := rc.GetStorageList()
	if err != nil {
		return nil, errors.Annotatef(err, "failed to get storage list")
	}
	if sb&StorageSFlash == 0 || sb&StorageRAM == 0 {
		return nil, errors.Errorf("invalid storage type: 0x%02x", sb)
	}
	common.Reportf("  NWP boot loader v%s, storage 0x%02x", vi.BootLoaderVersionString(), sb)
	return rc, nil
}

func checksum(data []byte) (result uint8) {
	for _, b := range data {
		result += b
	}
	return
}

func (rc *ROMClient) sendPacket(payload []byte) error {
	buf := bytes.NewBuffer(nil)
	binary.Write(buf, binary.BigEndian, uint16(len(payload)+2))
	binary.Write(buf, binary.BigEndian, checksum(payload))
	buf.Write(payload)
	glog.V(4).Infof("=> (%d) %s", len(buf.Bytes()), common.LimitStr(buf.Bytes(), 32))
	n, err := rc.s.Write(buf.Bytes())
	if err != nil || n != len(buf.Bytes()) {
		return errors.Annotatef(err, "failed to write command packet")
	}
	return rc.recvACK()
}

func (rc *ROMClient) readN(toRead int) ([]byte, error) {
	buf := make([]byte, toRead)
	nr := 0
	for nr < toRead {
		n, err := rc.s.Read(buf[nr:toRead])
		if err != nil || n == 0 {
			if err == nil {
				err = io.EOF
			}
			return nil, errors.Annotatef(err, "read(%d) failed, got %d", toRead, nr)
		}
		nr += n
	}
	glog.V(4).Infof("<= (%d) %s", toRead, common.LimitStr(buf, 32))
	return buf, nil
}

func (rc *ROMClient) recvPacket() (*bytes.Buffer, error) {
	header, err := rc.readN(3)
	if err != nil {
		return nil, errors.Annotatef(err, "failed to read packet header")
	}
	buf := bytes.NewBuffer(header)
	var plen uint16
	binary.Read(buf, binary.BigEndian, &plen)
	var csum uint8
	binary.Read(buf, binary.BigEndian, &csum)
	plen -= 2
	packet, err := rc.readN(int(plen))
	if err != nil {
		return nil, errors.Annotatef(err, "failed to read packet")
	}
	gotCsum := checksum(packet)
	if gotCsum != csum {
		return nil, errors.Errorf("checksum mismatch: want 0x%02x, got 0x%02x", csum, gotCsum)
	}
	rc.sendACK()
	return bytes.NewBuffer(packet), nil
}

func (rc *ROMClient) sendACK() error {
	ack := []byte{0x00, 0xcc}
	glog.V(4).Infof("=> (%d) %s", len(ack), common.LimitStr(ack, 32))
	_, err := rc.s.Write(ack)
	return err
}

func (rc *ROMClient) recvACK() error {
	ack, err := rc.readN(2)
	if err != nil {
		return errors.Annotatef(err, "failed to read ACK")
	}
	if ack[0] != 0 || ack[1] != 0xcc {
		return errors.Errorf("invalid ACK (%s)", common.LimitStr(ack, 2))
	}
	return nil
}

func (rc *ROMClient) sendCommand(cmd loaderCmd, args []byte) error {
	glog.V(2).Infof("=> {cmd:%s args(%d):%s}", cmd, len(args), common.LimitStr(args, 32))
	payload := []byte{byte(cmd)}
	payload = append(payload, args...)
	return rc.sendPacket(payload)
}

func (rc *ROMClient) connect(dc DeviceControl) error {
	rc.s.SetReadTimeout(200 * time.Millisecond)
	for i := 1; i <= 5; i++ {
		glog.V(3).Infof("Connect attempt #%d...", i)
		if dc != nil {
			dc.EnterBootLoader()
		}
		rc.s.Flush()
		if err := rc.s.SetBreak(true); err != nil {
			return errors.Annotatef(err, "failed to set break")
		}
		time.Sleep(200 * time.Millisecond)
		err := rc.recvACK()
		rc.s.SetBreak(false)
		if err == nil {
			return nil
		}
	}
	return errors.Errorf("failed to communicate to boot loader")
}

type VersionInfo struct {
	BootLoaderVersion uint32
	NWPVersion        uint32
	MACVersion        uint32
	PHYVersion        uint32
	ChipType          uint32
}

func (vi *VersionInfo) BootLoaderVersionString() string {
	return fmt.Sprintf("%d.%d.%d.%d", vi.BootLoaderVersion>>24, (vi.BootLoaderVersion>>16)&0xff, (vi.BootLoaderVersion>>8)&0xff, vi.BootLoaderVersion&0xff)
}

func (rc *ROMClient) GetStatus() (romStatus, error) {
	err := rc.sendCommand(cmdGetStatus, nil)
	if err != nil {
		return 0, errors.Annotatef(err, "GetStatus")
	}
	rb, err := rc.recvPacket()
	if err != nil {
		return 0, errors.Annotatef(err, "GetStatus")
	}
	return romStatus(rb.Bytes()[0]), nil
}

func (rc *ROMClient) commandWithStatus(cmd loaderCmd, args []byte) error {
	err := rc.sendCommand(cmd, args)
	if errors.Cause(err) == io.EOF {
		return errors.Annotatef(err, "%s: failed to send", cmd)
	}
	status, err := rc.GetStatus()
	if err != nil {
		return errors.Annotatef(err, "%s: failed to get status", cmd)
	}
	if status != statusOk {
		return errors.Errorf("%s: status %02x", cmd, status)
	}
	return nil
}

func (rc *ROMClient) GetVersionInfo() (*VersionInfo, error) {
	if err := rc.sendCommand(cmdGetVersionInfo, nil); err != nil {
		return nil, errors.Annotatef(err, "GetVersionInfo")
	}
	rb, err := rc.recvPacket()
	if err != nil {
		return nil, errors.Annotatef(err, "GetVersionInfo")
	}
	vi := &VersionInfo{}
	binary.Read(rb, binary.LittleEndian, &vi.BootLoaderVersion)
	binary.Read(rb, binary.LittleEndian, &vi.NWPVersion)
	binary.Read(rb, binary.LittleEndian, &vi.MACVersion)
	binary.Read(rb, binary.LittleEndian, &vi.PHYVersion)
	binary.Read(rb, binary.LittleEndian, &vi.ChipType)
	glog.V(3).Infof("VersionInfo: %+v", *vi)
	return vi, nil
}

type StorageBitmap uint8

const (
	StorageInvalid StorageBitmap = 0x00
	StorageFlash                 = 0x02
	StorageSFlash                = 0x04
	StorageRAM                   = 0x80
)

func (rc *ROMClient) GetStorageList() (StorageBitmap, error) {
	err := rc.sendCommand(cmdGetStorageList, nil)
	if err != nil {
		return StorageInvalid, errors.Annotatef(err, "GetStorageList")
	}
	sb, err := rc.readN(1)
	if err != nil {
		return StorageInvalid, errors.Annotatef(err, "failed to read GetStorageList response")
	}
	return StorageBitmap(sb[0]), nil
}

func (rc *ROMClient) SwitchUARTtoAppsMCU() error {
	delay := uartSwitchDelay
	buf := bytes.NewBuffer(nil)
	binary.Write(buf, binary.BigEndian, uint32(delay))
	if err := rc.sendCommand(cmdSwitchUARTtoAppsMCU, buf.Bytes()); err != nil {
		return errors.Annotatef(err, "failed to switch UART to apps CPU")
	}
	time.Sleep(1200 * time.Millisecond)
	return rc.connect(nil)
}

func (rc *ROMClient) FormatSLFS(size int) error {
	buf := bytes.NewBuffer(nil)
	binary.Write(buf, binary.BigEndian, uint32(2))
	binary.Write(buf, binary.BigEndian, uint32(size/fsBlockSize))
	binary.Write(buf, binary.BigEndian, uint32(0))
	binary.Write(buf, binary.BigEndian, uint32(0))
	binary.Write(buf, binary.BigEndian, uint32(2))
	rc.s.SetReadTimeout(10 * time.Second)
	return rc.commandWithStatus(cmdFormatSLFS, buf.Bytes())
}

func (rc *ROMClient) EraseFile(fname string) error {
	buf := bytes.NewBuffer(nil)
	binary.Write(buf, binary.BigEndian, uint32(0))
	buf.Write([]byte(fname))
	binary.Write(buf, binary.BigEndian, uint8(0))
	rc.s.SetReadTimeout(1 * time.Second)
	return rc.commandWithStatus(cmdEraseFile, buf.Bytes())
}

type SLFSFileInfo struct {
	Name      string
	Data      []byte
	AllocSize uint32
	Signature fileSignature
}

func (rc *ROMClient) UploadFile(fi *SLFSFileInfo) error {
	openFlags := uint32(openModeCreateifNotExist)
	if len(fi.Signature) > 0 {
		if len(fi.Signature) == FileSignatureLength {
			openFlags |= openModeSecure
		} else {
			return errors.Errorf("%s: invalid signature length (%d)", fi.Name, len(fi.Signature))
		}
	}
	allocSize := int(fi.AllocSize)
	if allocSize == 0 {
		allocSize = len(fi.Data)
	}
	i, numBlocks, blockSize := 0, 0, 0
	for i, blockSize = range []int{0x100, 0x400, 0x1000, 0x4000, 0x10000} {
		numBlocks = allocSize / blockSize
		if allocSize%blockSize > 0 {
			numBlocks++
		}
		if numBlocks <= 255 {
			break
		}
	}
	if numBlocks > 255 {
		return errors.Errorf("file is too big")
	}
	openFlags |= uint32(i) << 8
	openFlags |= uint32(numBlocks)
	glog.V(2).Infof("%s: %d blocks * %d; flags: 0x%x", fi.Name, numBlocks, blockSize, openFlags)
	buf := bytes.NewBuffer(nil)
	binary.Write(buf, binary.BigEndian, uint32(openFlags))
	binary.Write(buf, binary.BigEndian, uint32(0))
	buf.Write([]byte(fi.Name))
	binary.Write(buf, binary.BigEndian, uint16(0))
	rc.s.SetReadTimeout(10 * time.Second)
	err := rc.sendCommand(cmdStartUpload, buf.Bytes())
	if err != nil {
		return errors.Annotatef(err, "WriteFile")
	}
	t, err := rc.readN(4)
	if err != nil {
		return errors.Annotatef(err, "failed to read token")
	}
	var token uint32
	binary.Read(bytes.NewBuffer(t), binary.BigEndian, &token)
	glog.V(2).Infof("token: 0x%08x", token)

	rc.s.SetReadTimeout(1 * time.Second)
	for offset := 0; offset < len(fi.Data); {
		end := offset + fileUploadBlockSize
		if end > len(fi.Data) {
			end = len(fi.Data)
		}
		buf.Reset()
		binary.Write(buf, binary.BigEndian, uint32(offset))
		buf.Write(fi.Data[offset:end])
		if err := rc.commandWithStatus(cmdUploadChunk, buf.Bytes()); err != nil {
			return errors.Annotatef(err, "%s: write failed @ %d", fi.Name, offset)
		}
		offset = end
	}

	buf.Reset()
	if len(fi.Signature) > 0 {
		for i := 0; i < 63; i++ {
			buf.Write([]byte{0})
		}
		buf.Write(fi.Signature)
	}
	if err := rc.commandWithStatus(cmdFinishUpload, buf.Bytes()); err != nil {
		return errors.Annotatef(err, "%s: close failed", fi.Name)
	}

	return nil
}

// TODO(rojer): Use stringer when it actually works.
func (cmd loaderCmd) String() string {
	switch cmd {
	case cmdStartUpload:
		return fmt.Sprintf("StartUpload(0x%x)", uint8(cmd))
	case cmdFinishUpload:
		return fmt.Sprintf("FinishUpload(0x%x)", uint8(cmd))
	case cmdGetStatus:
		return fmt.Sprintf("GetStatus(0x%x)", uint8(cmd))
	case cmdUploadChunk:
		return fmt.Sprintf("UploadChunk(0x%x)", uint8(cmd))
	case cmdGetStorageList:
		return fmt.Sprintf("GetStorageList(0x%x)", uint8(cmd))
	case cmdFormatSLFS:
		return fmt.Sprintf("FormatSLFS(0x%x)", uint8(cmd))
	case cmdEraseFile:
		return fmt.Sprintf("EraseFile(0x%x)", uint8(cmd))
	case cmdGetVersionInfo:
		return fmt.Sprintf("GetVersionInfo(0x%x)", uint8(cmd))
	case cmdSwitchUARTtoAppsMCU:
		return fmt.Sprintf("SwitchUARTtoAppsMCU(0x%x)", uint8(cmd))
	default:
		return fmt.Sprintf("?(0x%x)", uint8(cmd))
	}
}
