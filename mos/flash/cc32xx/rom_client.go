package cc32xx

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"io"
	"io/ioutil"
	"time"

	"cesanta.com/mos/flash/common"
	"github.com/cesanta/errors"
	"github.com/cesanta/go-serial/serial"
	"github.com/golang/glog"
)

type loaderCmd uint8

// Some of these are documented in the SimpleLink CC3100/CC3200 Embedded Programming Application Note
// and CC3120 and CC3220 SimpleLink Wi-Fi Embedded Programming User Guide (SWPA230)
// http://www.ti.com/tool/embedded-programming
const (
	cmdStartUpload  loaderCmd = 0x21
	cmdFinishUpload           = 0x22
	cmdGetStatus              = 0x23
	cmdUploadChunk            = 0x24

	cmdGetStorageList = 0x27
	cmdFormatSLFS     = 0x28

	cmdRawStorageWrite     = 0x2d
	cmdEraseFile           = 0x2e
	cmdGetVersionInfo      = 0x2f
	cmdRawStorageErase     = 0x30
	cmdGetStorageInfo      = 0x31
	cmdExecuteFromRAM      = 0x32
	cmdSwitchUARTtoAppsMCU = 0x33
	cmdUploadImage         = 0x34

	cmdGetDeviceInfo = 0x37

	cmdGetMACAddress = 0x3a
)

const (
	fsBlockSize         = 0x1000
	rawWriteSize        = 0x1000 - 0x10
	imageWriteSize      = 0x1000
	uartSwitchDelay     = 26666667 // 1 second
	FileSignatureLength = 0x100
	fileUploadBlockSize = 0x1000
)

type ChipType int

const (
	ChipCC3200 ChipType = iota
	ChipCC3220
	ChipCC3220S
	ChipCC3220SF
)

type fileOpenMode uint32

const (
	openModeCreateifNotExist fileOpenMode = 0x3000
	openModeSecure                        = 0x20000
)

type FileSignature []byte

type SLFSFileInfo struct {
	Name      string
	Data      []byte
	AllocSize uint32
	Signature FileSignature
}

type romStatus byte

const (
	statusOk          romStatus = 0x40
	statusUnknownCmd            = 0x41
	statusInvalidCmd            = 0x42
	statusInvalidAddr           = 0x43
	statusFlashFail             = 0x44
)

type StorageBitmap uint8

const (
	StorageBitInvalid StorageBitmap = 0x00
	StorageBitFlash                 = 0x02
	StorageBitSFlash                = 0x04
	StorageBitRAM                   = 0x80
)

type StorageID uint8

const (
	StorageRAM    StorageID = 0x00
	StorageFlash            = 0x01
	StorageSFlash           = 0x02
)

type ROMClient struct {
	s  serial.Serial
	ct ChipType
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
		return nil, errors.Annotatef(err, "failed to get loader version info")
	}
	sb, err := rc.GetStorageList()
	if err != nil {
		return nil, errors.Annotatef(err, "failed to get storage list")
	}
	common.Reportf("  %s boot loader v%s, storage 0x%02x",
		vi.ChipTypeString(), vi.BootLoaderVersionString(), sb)
	return rc, nil
}

func (rc *ROMClient) SwitchToNWPLoader() error {
	common.Reportf("Switching to NWP...")
	if err := rc.SwitchUARTtoAppsMCU(); err != nil {
		return errors.Annotatef(err, "failed to switch to apps mode")
	}
	vi, err := rc.GetVersionInfo()
	if err != nil {
		return err
	}
	sb, err := rc.GetStorageList()
	if err != nil {
		return errors.Annotatef(err, "failed to get storage list")
	}
	if sb&StorageBitSFlash == 0 || sb&StorageBitRAM == 0 {
		return errors.Errorf("invalid storage type: 0x%02x", sb)
	}
	common.Reportf("  NWP boot loader v%s, storage 0x%02x", vi.BootLoaderVersionString(), sb)
	return nil
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
	glog.V(4).Infof("=> (%d) %s", len(buf.Bytes()), common.LimitStr(buf.Bytes(), 64))
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
	glog.V(4).Infof("<= (%d) %s", toRead, common.LimitStr(buf, 64))
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
	rc.s.SetReadTimeout(1000 * time.Millisecond)
	for i := 1; i <= 5; i++ {
		glog.V(3).Infof("Connect attempt #%d...", i)
		if err := rc.s.SetBreak(true); err != nil {
			return errors.Annotatef(err, "failed to set break")
		}
		rc.s.Flush()
		if dc != nil {
			dc.EnterBootLoader()
			time.Sleep(200 * time.Millisecond)
		}
		err := rc.recvACK()
		rc.s.SetBreak(false)
		if err == nil {
			return nil
		} else {
			glog.Infof("%s", err)
		}
	}
	return errors.Errorf("failed to communicate to boot loader")
}

type VersionInfo struct {
	BootLoaderVersion uint32
	NWPVersion        uint32
	MACVersion        uint32
	PHYVersion        uint32
	ChipTypeV         uint32
}

func (vi *VersionInfo) BootLoaderVersionString() string {
	return fmt.Sprintf("%d.%d.%d.%d", vi.BootLoaderVersion>>24, (vi.BootLoaderVersion>>16)&0xff, (vi.BootLoaderVersion>>8)&0xff, vi.BootLoaderVersion&0xff)
}

func (vi *VersionInfo) ChipType() (ChipType, error) {
	switch vi.ChipTypeV >> 24 {
	case 0x10:
		if vi.BootLoaderVersion > 0x03000000 {
			return ChipCC3220, nil
		} else {
			return ChipCC3200, nil
		}
	case 0x18:
		return ChipCC3220S, nil
	case 0x19:
		return ChipCC3220SF, nil
	}
	return 0, errors.Errorf("unknown chip")
}

func (vi *VersionInfo) ChipTypeString() string {
	ct, err := vi.ChipType()
	if err != nil {
		return err.Error()
	}
	switch ct {
	case ChipCC3200:
		return "CC3200"
	case ChipCC3220:
		return "CC3220"
	case ChipCC3220S:
		return "CC3220S"
	case ChipCC3220SF:
		return "CC3220SF"
	}
	return "???"
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
		return errors.Errorf("%s: status 0x%02x", cmd, status)
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
	binary.Read(rb, binary.BigEndian, &vi.BootLoaderVersion)
	binary.Read(rb, binary.BigEndian, &vi.NWPVersion)
	binary.Read(rb, binary.BigEndian, &vi.MACVersion)
	binary.Read(rb, binary.BigEndian, &vi.PHYVersion)
	binary.Read(rb, binary.BigEndian, &vi.ChipTypeV)
	glog.V(3).Infof("VersionInfo: %+v", *vi)
	return vi, nil
}

func (rc *ROMClient) GetStorageList() (StorageBitmap, error) {
	err := rc.sendCommand(cmdGetStorageList, nil)
	if err != nil {
		return StorageBitInvalid, errors.Annotatef(err, "GetStorageList")
	}
	sb, err := rc.readN(1)
	if err != nil {
		return StorageBitInvalid, errors.Annotatef(err, "failed to read GetStorageList response")
	}
	return StorageBitmap(sb[0]), nil
}

type StorageInfo struct {
	BlockSize uint16
	NumBlocks uint16
	Reserved  uint32
}

func (rc *ROMClient) GetStorageInfo(sid StorageID) (*StorageInfo, error) {
	buf := bytes.NewBuffer(nil)
	binary.Write(buf, binary.BigEndian, uint32(sid))
	if err := rc.sendCommand(cmdGetStorageInfo, buf.Bytes()); err != nil {
		return nil, errors.Annotatef(err, "GetStorageInfo")
	}
	rb, err := rc.recvPacket()
	if err != nil {
		return nil, errors.Annotatef(err, "GetStorageInfo")
	}
	si := &StorageInfo{}
	binary.Read(rb, binary.BigEndian, &si.BlockSize)
	binary.Read(rb, binary.BigEndian, &si.NumBlocks)
	binary.Read(rb, binary.BigEndian, &si.Reserved)
	glog.V(3).Infof("StorageInfo %s: %+v", sid, *si)
	return si, nil
}

func (rc *ROMClient) ExecuteFromRAM() error {
	if err := rc.sendCommand(cmdExecuteFromRAM, nil); err != nil {
		return errors.Trace(err)
	}
	// New loader should send ACK.
	return rc.recvACK()
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

type DeviceInfo struct {
	FSBlockSize         uint16
	FSNumBlocks         uint16
	Unknown04           uint16
	Unknown06           uint16
	Unknown08           uint16
	FSUnknownFreeSize0A uint16
	FSFreeBlocks        uint16
	// ???
}

// 100004000178000b0016025402670000f00100ff33030b00000000000000000f00120000
// |||| - block size
//     |||| - num blocks
//                         |||| - free blocks
// 100002000178000b0016005400670000f00100ff33030b00000000000000000f00120000

func (rc *ROMClient) GetDeviceInfo() (*DeviceInfo, error) {
	err := rc.sendCommand(cmdGetDeviceInfo, nil)
	if err != nil {
		return nil, errors.Annotatef(err, "GetDeviceInfo")
	}
	rb, err := rc.recvPacket()
	if err != nil {
		return nil, errors.Annotatef(err, "GetDeviceInfo")
	}
	di := &DeviceInfo{}
	binary.Read(rb, binary.BigEndian, &di.FSBlockSize)
	binary.Read(rb, binary.BigEndian, &di.FSNumBlocks)
	binary.Read(rb, binary.BigEndian, &di.Unknown04)
	binary.Read(rb, binary.BigEndian, &di.Unknown06)
	binary.Read(rb, binary.BigEndian, &di.Unknown08)
	binary.Read(rb, binary.BigEndian, &di.FSUnknownFreeSize0A)
	binary.Read(rb, binary.BigEndian, &di.FSFreeBlocks)
	glog.V(3).Infof("DeviceInfo: %+v", *di)
	return di, nil
}

type MACAddress [6]byte

func (mac MACAddress) String() string {
	return fmt.Sprintf(
		"%02x:%02x:%02x:%02x:%02x:%02x",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5])
}

func (rc *ROMClient) GetMACAddress() (MACAddress, error) {
	var mac MACAddress
	buf := bytes.NewBuffer(nil)
	binary.Write(buf, binary.BigEndian, uint32(0)) // ? mystery argument
	err := rc.sendCommand(cmdGetMACAddress, buf.Bytes())
	if err != nil {
		return mac, errors.Annotatef(err, "GetMACAddress")
	}
	rb, err := rc.recvPacket()
	if err != nil {
		return mac, errors.Annotatef(err, "GetMACAddress")
	}
	copy(mac[:], rb.Bytes())
	return mac, nil
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

func (rc *ROMClient) RawStorageWrite(sid StorageID, offset int, data []byte) error {
	for numWritten := 0; numWritten < len(data); {
		remaining := data[numWritten:]
		writeSize := rawWriteSize
		if writeSize > len(remaining) {
			writeSize = len(remaining)
		}
		toWrite := remaining[:writeSize]
		buf := bytes.NewBuffer(nil)
		binary.Write(buf, binary.BigEndian, uint32(sid))
		binary.Write(buf, binary.BigEndian, uint32(offset))
		binary.Write(buf, binary.BigEndian, uint32(len(toWrite)))
		buf.Write(toWrite)
		glog.V(3).Infof("Raw write to %s: %d @ %d", sid, len(toWrite), offset)
		err := rc.commandWithStatus(cmdRawStorageWrite, buf.Bytes())
		if err != nil {
			return errors.Annotatef(err, "%s write failed: %d @ %d", sid, len(toWrite), offset)
		}
		numWritten += writeSize
		offset += writeSize
	}
	return nil
}

func (rc *ROMClient) RawStorageEraseBlocks(sid StorageID, startBlock int, numBlocks int) error {
	buf := bytes.NewBuffer(nil)
	binary.Write(buf, binary.BigEndian, uint32(sid))
	binary.Write(buf, binary.BigEndian, uint32(startBlock))
	binary.Write(buf, binary.BigEndian, uint32(numBlocks))
	glog.V(3).Infof("Raw erase %s: %d @ %d", sid, numBlocks, startBlock)
	err := rc.commandWithStatus(cmdRawStorageErase, buf.Bytes())
	if err != nil {
		return errors.Annotatef(err, "%s erase failed (%d @ %d)", sid, numBlocks, startBlock)
	}
	return nil
}

func (rc *ROMClient) RawStorageEraseBytes(sid StorageID, offset int, numBytes int) error {
	si, err := rc.GetStorageInfo(sid)
	if err != nil {
		return errors.Annotatef(err, "failed to get storage info")
	}
	bs := int(si.BlockSize)
	startBlock := offset / bs
	endBlock := (offset + numBytes + bs - 1) / bs
	numBlocks := endBlock - startBlock
	glog.V(3).Infof("Erase %s blocks %d-%d (%d)", sid, startBlock, endBlock, numBlocks)
	return rc.RawStorageEraseBlocks(sid, startBlock, numBlocks)
}

func (rc *ROMClient) RawEraseAndWrite(sid StorageID, offset int, data []byte) error {
	if err := rc.RawStorageEraseBytes(sid, offset, len(data)); err != nil {
		return errors.Trace(err)
	}
	if err := rc.RawStorageWrite(sid, offset, data); err != nil {
		return errors.Trace(err)
	}
	return nil
}

func (rc *ROMClient) EraseFile(fname string) error {
	buf := bytes.NewBuffer(nil)
	binary.Write(buf, binary.BigEndian, uint32(0))
	buf.Write([]byte(fname))
	binary.Write(buf, binary.BigEndian, uint8(0))
	rc.s.SetReadTimeout(1 * time.Second)
	return rc.commandWithStatus(cmdEraseFile, buf.Bytes())
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

func (rc *ROMClient) UploadImageFile(fname string) error {
	glog.Infof("UCF image file name: %s", fname)
	data, err := ioutil.ReadFile(fname)
	if err != nil {
		return errors.Annotatef(err, "failed to read image file")
	}
	rc.s.SetReadTimeout(25 * time.Second)
	for numWritten := 0; numWritten < len(data); {
		remaining := data[numWritten:]
		writeSize := imageWriteSize
		if writeSize > len(remaining) {
			writeSize = len(remaining)
		}
		toWrite := remaining[:writeSize]
		buf := bytes.NewBuffer(nil)
		binary.Write(buf, binary.BigEndian, uint16(0)) // KeySize
		binary.Write(buf, binary.BigEndian, uint16(writeSize))
		binary.Write(buf, binary.BigEndian, uint32(0)) // Flags
		buf.Write(toWrite)
		glog.V(3).Infof("Image write: %d @ %d", len(toWrite), numWritten)
		if writeSize == len(remaining) {
			// This is the last write, it will trigger image extraction and will take a while.
			common.Reportf("Upload finished, image is being extracted...")
		}
		err := rc.sendCommand(cmdUploadImage, buf.Bytes())
		if err != nil {
			if writeSize == len(remaining) {
				// Extracting image can take a long time, but we cannot set read timeout more than 25 seconds.
				// Try reading ACK again.
				err = rc.recvACK()
			}
			if err != nil {
				return errors.Annotatef(err, "image write failed: %d @ %d", len(toWrite), numWritten)
			}
		}
		// Read extended status.
		extStatus, err := rc.readN(4)
		if err != nil {
			return errors.Annotatef(err, "failed to read ACK")
		}
		var e0 int16
		var e1 uint16
		buf = bytes.NewBuffer(extStatus)
		binary.Read(buf, binary.BigEndian, &e0)
		binary.Read(buf, binary.BigEndian, &e1)
		if e0 < 0 {
			return errors.Errorf("image programming error @ %d: e0 = %d, e1 = %d", numWritten, e0, e1)
		}
		numWritten += writeSize
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
	case cmdRawStorageWrite:
		return fmt.Sprintf("RawStorageWrite(0x%x)", uint8(cmd))
	case cmdEraseFile:
		return fmt.Sprintf("EraseFile(0x%x)", uint8(cmd))
	case cmdGetVersionInfo:
		return fmt.Sprintf("GetVersionInfo(0x%x)", uint8(cmd))
	case cmdRawStorageErase:
		return fmt.Sprintf("RawStorageErase(0x%x)", uint8(cmd))
	case cmdGetStorageInfo:
		return fmt.Sprintf("GetStorageInfo(0x%x)", uint8(cmd))
	case cmdExecuteFromRAM:
		return fmt.Sprintf("ExecuteFromRAM(0x%x)", uint8(cmd))
	case cmdSwitchUARTtoAppsMCU:
		return fmt.Sprintf("SwitchUARTtoAppsMCU(0x%x)", uint8(cmd))
	case cmdUploadImage:
		return fmt.Sprintf("UploadImage(0x%x)", uint8(cmd))
	case cmdGetDeviceInfo:
		return fmt.Sprintf("GetDeviceInfo(0x%x)", uint8(cmd))
	case cmdGetMACAddress:
		return fmt.Sprintf("GetMACAddress(0x%x)", uint8(cmd))
	default:
		return fmt.Sprintf("?(0x%x)", uint8(cmd))
	}
}

func (sid StorageID) String() string {
	switch sid {
	case StorageRAM:
		return "RAM"
	case StorageFlash:
		return "Flash"
	case StorageSFlash:
		return "SFLASH"
	default:
		return fmt.Sprintf("?(0x%x)", uint8(sid))
	}
}
