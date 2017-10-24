// +build !noflash

package esp32

import (
	"bytes"
	"encoding/hex"
	"fmt"
	"math/big"

	"cesanta.com/mos/flash/esp"
	"github.com/cesanta/errors"
	"github.com/golang/glog"
)

const (
	KeyLen = 32
)

var (
	blockReadBases       = []uint32{0x6001a000, 0x6001a038, 0x6001a058, 0x6001a078}
	blockWriteBases      = []uint32{0x6001a01c, 0x6001a098, 0x6001a0b8, 0x6001a0d8}
	blockLengths         = []int{7, 8, 8, 8}
	ReadDisableFuseName  = "efuse_rd_disable"
	WriteDisableFuseName = "efuse_wr_disable"
	MACAddressFuseName   = "WIFI_MAC_Address"
)

var (
	eFuseCtlRegConf        uint32 = 0x6001a0fc
	eFuseCtlRegConfOpRead  uint32 = 0x5aa5
	eFuseCtlRegConfOpWrite uint32 = 0x5a5a
	eFuseCtlRegCmd         uint32 = 0x6001a104
	eFuseCtlRegCmdRead     uint32 = 0x1
	eFuseCtlRegCmdWrite    uint32 = 0x2
)

type eFuseCtlOp int

const (
	eFuseCtlOpRead eFuseCtlOp = iota
	eFuseCtlOpWrite
)

type bitField struct {
	word, bh, bl int
}

type fuseDescriptor struct {
	name   string
	block  int
	fields []bitField
	wdBit  int
	rdBit  int
}

var (
	// Fuse definitions as described in ESP32 TRM chapter 11, tables 29 and 30.
	fuseDescriptors = []fuseDescriptor{
		{name: WriteDisableFuseName, block: 0, fields: []bitField{{0, 15, 0}}, wdBit: 1, rdBit: -1},
		{name: ReadDisableFuseName, block: 0, fields: []bitField{{0, 19, 16}}, wdBit: 0, rdBit: -1},
		{name: "flash_crypt_cnt", block: 0, fields: []bitField{{0, 27, 20}}, wdBit: 2, rdBit: -1},
		// 0, 31, 28 - ?
		{name: MACAddressFuseName, block: 0, fields: []bitField{{1, 31, 0}, {2, 23, 0}}, wdBit: 3, rdBit: -1},
		// 2, 31, 24 - ?
		// 3, 3, 0 - ?
		{name: "SPI_pad_config_hd", block: 0, fields: []bitField{{3, 8, 4}}, wdBit: 3, rdBit: -1},
		{name: "chip_package", block: 0, fields: []bitField{{3, 11, 9}}, wdBit: 3, rdBit: -1},
		{name: "chip_version", block: 0, fields: []bitField{{3, 15, 12}}, wdBit: 3, rdBit: -1},
		// 3, 31, 15 - ?
		// 4, 13, 0 - ?
		{name: "XPD_SDIO_REG", block: 0, fields: []bitField{{4, 14, 14}}, wdBit: 5, rdBit: -1},
		{name: "SDIO_TIEH", block: 0, fields: []bitField{{4, 15, 15}}, wdBit: 5, rdBit: -1},
		{name: "sdio_force", block: 0, fields: []bitField{{4, 16, 16}}, wdBit: 5, rdBit: -1},
		// 4, 31, 17 - ?
		{name: "SPI_pad_config_clk", block: 0, fields: []bitField{{5, 4, 0}}, wdBit: 6, rdBit: -1},
		{name: "SPI_pad_config_q", block: 0, fields: []bitField{{5, 9, 5}}, wdBit: 6, rdBit: -1},
		{name: "SPI_pad_config_d", block: 0, fields: []bitField{{5, 14, 10}}, wdBit: 6, rdBit: -1},
		{name: "SPI_pad_config_cs0", block: 0, fields: []bitField{{5, 19, 15}}, wdBit: 6, rdBit: -1},
		{name: "flash_crypt_config", block: 0, fields: []bitField{{5, 31, 28}}, wdBit: 10, rdBit: 3},
		{name: "coding_scheme", block: 0, fields: []bitField{{6, 1, 0}}, wdBit: 10, rdBit: 3},
		{name: "console_debug_disable", block: 0, fields: []bitField{{6, 2, 2}}, wdBit: 15, rdBit: -1},
		// 6, 3, 3 - ?
		{name: "abstract_done_0", block: 0, fields: []bitField{{6, 4, 4}}, wdBit: 12, rdBit: -1},
		{name: "abstract_done_1", block: 0, fields: []bitField{{6, 5, 5}}, wdBit: 13, rdBit: -1},
		{name: "JTAG_disable", block: 0, fields: []bitField{{6, 6, 6}}, wdBit: 14, rdBit: -1},
		{name: "download_dis_encrypt", block: 0, fields: []bitField{{6, 7, 7}}, wdBit: 15, rdBit: -1},
		{name: "download_dis_decrypt", block: 0, fields: []bitField{{6, 8, 8}}, wdBit: 15, rdBit: -1},
		{name: "download_dis_cache", block: 0, fields: []bitField{{6, 9, 9}}, wdBit: 15, rdBit: -1},
		{name: "key_status", block: 0, fields: []bitField{{6, 10, 10}}, wdBit: 10, rdBit: 3},
		// 6, 31, 11 - ?
		{name: "flash_encryption_key", block: 1, fields: []bitField{{0, 31, 0}, {1, 31, 0}, {2, 31, 0}, {3, 31, 0}, {4, 31, 0}, {5, 31, 0}, {6, 31, 0}, {7, 31, 0}}, wdBit: 7, rdBit: 0},
		{name: "secure_boot_key", block: 2, fields: []bitField{{0, 31, 0}, {1, 31, 0}, {2, 31, 0}, {3, 31, 0}, {4, 31, 0}, {5, 31, 0}, {6, 31, 0}, {7, 31, 0}}, wdBit: 8, rdBit: 1},
		{name: "user_key", block: 3, fields: []bitField{{0, 31, 0}, {1, 31, 0}, {2, 31, 0}, {3, 31, 0}, {4, 31, 0}, {5, 31, 0}, {6, 31, 0}, {7, 31, 0}}, wdBit: 9, rdBit: 2},
	}
)

type FuseBlock struct {
	rrw   esp.RegReaderWriter
	num   int
	rBase uint32
	wBase uint32
	data  []uint32
	diff  []uint32
}

func (b *FuseBlock) String() string {
	o := bytes.NewBuffer(nil)
	fmt.Fprintf(o, "eFuse block %d @ 0x%08x: ", b.num, b.rBase)
	for _, v := range b.data {
		fmt.Fprintf(o, "0x%08x ", v)
	}
	return o.String()
}

func (b *FuseBlock) Read() error {
	glog.Infof("Reading eFuse block %d @ 0x%08x...", b.num, b.rBase)
	for i, _ := range b.data {
		reg := b.rBase + uint32(4*i)
		v, err := b.rrw.ReadReg(reg)
		if err != nil {
			return errors.Annotatef(err, "failed to read word %d @ 0x%08x", i, reg)
		}
		b.data[i] = v
	}
	return nil
}

func (b *FuseBlock) HasDiffs() bool {
	for _, d := range b.diff {
		if d != 0 {
			return true
		}
	}
	return false
}

func (b *FuseBlock) WriteDiffs() error {
	glog.Infof("Writing eFuse block %d @ 0x%08x...", b.num, b.wBase)
	for i, d := range b.diff {
		reg := b.wBase + uint32(4*i)
		if err := b.rrw.WriteReg(reg, d); err != nil {
			return errors.Annotatef(err, "failed to write word %d @ 0x%08x", i, reg)
		}
	}
	return nil
}

func getFuseValue(name string, blocks []*FuseBlock) (*big.Int, error) {
	for _, fd := range fuseDescriptors {
		if fd.name == name {
			f := newFuse(fd, blocks)
			return f.Value(false)
		}
	}
	return nil, errors.Errorf("unknown fuse: %q", name)
}

func setFuseValue(name string, value *big.Int, blocks []*FuseBlock) error {
	for _, fd := range fuseDescriptors {
		if fd.name == name {
			f := newFuse(fd, blocks)
			return f.SetValue(value)
		}
	}
	return errors.Errorf("unknown fuse: %q", name)
}

type Fuse struct {
	d      fuseDescriptor
	blocks []*FuseBlock
}

func newFuse(fd fuseDescriptor, blocks []*FuseBlock) *Fuse {
	return &Fuse{d: fd, blocks: blocks}
}

func (f *Fuse) Name() string {
	return f.d.name
}

func (f *Fuse) BitLen() int {
	numBits := 0
	for _, bf := range f.d.fields {
		numBits += bf.bh - bf.bl + 1
	}
	return numBits
}

func (f *Fuse) IsReadable() bool {
	if f.d.rdBit < 0 {
		return true
	}
	v, err := getFuseValue(ReadDisableFuseName, f.blocks)
	if err != nil {
		// Must not happen
		glog.Exitf("could not get rd_disable flag value")
	}
	return v.Bit(f.d.rdBit) == 0
}

func (f *Fuse) IsWritable() bool {
	if f.d.wdBit < 0 {
		return true
	}
	v, err := getFuseValue(WriteDisableFuseName, f.blocks)
	if err != nil {
		// Must not happen
		glog.Exitf("could not get wr_disable flag value")
	}
	return v.Bit(f.d.wdBit) == 0
}

func (f *Fuse) SetReadDisable() error {
	if f.d.rdBit < 0 {
		return errors.Errorf("%s cannot be read-protected", f.Name())
	}
	v, err := getFuseValue(ReadDisableFuseName, f.blocks)
	if err != nil {
		// Must not happen
		glog.Exitf("could not get rd_disable flag value")
	}
	v.SetBit(v, f.d.rdBit, 1)
	return setFuseValue(ReadDisableFuseName, v, f.blocks)
}

func (f *Fuse) SetWriteDisable() error {
	if f.d.wdBit < 0 {
		return errors.Errorf("%s cannot be write-protected", f.Name())
	}
	v, err := getFuseValue(WriteDisableFuseName, f.blocks)
	if err != nil {
		// Must not happen
		glog.Exitf("could not get wr_disable flag value")
	}
	v.SetBit(v, f.d.wdBit, 1)
	return setFuseValue(WriteDisableFuseName, v, f.blocks)
}

func (f *Fuse) Value(withDiffs bool) (*big.Int, error) {
	if !f.IsReadable() {
		return nil, errors.Errorf("fuse %q is not readable", f.Name())
	}
	var v big.Int
	one := big.NewInt(1)
	for _, bf := range f.d.fields {
		w := big.NewInt(int64(f.blocks[f.d.block].data[bf.word]))
		if withDiffs {
			w.Or(w, big.NewInt(int64(f.blocks[f.d.block].diff[bf.word])))
		}
		for fbi := bf.bh; fbi >= bf.bl; fbi-- {
			v.Lsh(&v, 1)
			if w.Bit(fbi) != 0 {
				v.Or(&v, one)
			}
		}
	}
	return &v, nil
}

func (f *Fuse) HasDiffs() bool {
	v, err1 := f.Value(false /* withDiffs */)
	vd, err2 := f.Value(true /* withDiffs */)
	return err1 == nil && err2 == nil && vd.Cmp(v) != 0
}

func (f *Fuse) SetValue(v *big.Int) error {
	if !f.IsWritable() {
		return errors.Errorf("fuse %q is not writable", f.Name())
	}
	if v.BitLen() > f.BitLen() {
		return errors.Errorf("fuse %q is %d bits long, value is %d bits long", f.Name(), f.BitLen(), v.BitLen())
	}
	bi := f.BitLen()
	for _, bf := range f.d.fields {
		w := big.NewInt(int64(f.blocks[f.d.block].data[bf.word]))
		d := big.NewInt(int64(f.blocks[f.d.block].diff[bf.word]))
		for fbi := bf.bh; fbi >= bf.bl; fbi-- {
			bi--
			if w.Bit(fbi) == v.Bit(bi) {
				continue
			}
			if w.Bit(fbi) == 1 {
				return errors.Errorf("cannot reset fuse bit value from 1 to 0 (value bit %d => block %d, word %d, bit %d)",
					bi, f.d.block, bf.word, fbi)
			}
			d.SetBit(d, fbi, 1)
		}
		f.blocks[f.d.block].diff[bf.word] = uint32(d.Uint64())
	}
	return nil
}

func (f *Fuse) IsKey() bool {
	return f.d.block > 0
}

// Reverse and convert BE -> LE
func reverseKey(kb []byte) {
	if len(kb) != KeyLen {
		glog.Exitf("want %d bytes, got %d", KeyLen, len(kb))
	}
	for wi := 0; wi < 4; wi++ {
		for bi := 0; bi < 4; bi++ {
			i := 4*wi + bi
			j := len(kb) - 1 - 4*wi - (3 - bi)
			kb[i], kb[j] = kb[j], kb[i]
		}
	}
}

func (f *Fuse) SetKeyValue(kb []byte) error {
	if !f.IsKey() {
		return errors.Errorf("not a key slot")
	}
	if len(kb) != KeyLen {
		return errors.Errorf("want %d bytes, got %d", KeyLen, len(kb))
	}
	kbr := make([]byte, len(kb))
	copy(kbr, kb)
	reverseKey(kbr)
	v := big.NewInt(0)
	v.SetBytes(kbr)
	return f.SetValue(v)
}

// c4 05 dd 9c b6 24 0a -> 24:0a:c4:05:dd:9c
func (f *Fuse) MACAddressString() string {
	var macBytes [7]byte
	v, _ := f.Value(false)
	vBytes := v.Bytes()
	for i := len(macBytes) - len(vBytes); i < len(macBytes); {
		for j := 0; j < len(vBytes); {
			macBytes[i] = vBytes[j]
			i++
			j++
		}
	}
	return fmt.Sprintf("%02x:%02x:%02x:%02x:%02x:%02x",
		macBytes[5], macBytes[6], macBytes[0], macBytes[1], macBytes[2], macBytes[3])
}

func (f *Fuse) String() string {
	b := bytes.NewBuffer(nil)
	fmt.Fprintf(b, "%-21s:", f.Name())
	if f.IsReadable() {
		v, _ := f.Value(false /* withDiff */)
		vd, _ := f.Value(true /* withDiff */)
		vflen := f.BitLen() / 4
		if vflen == 0 {
			vflen = 1
		}
		if f.d.block > 0 {
			fmt.Fprintf(b, " %s", f.KeyString())
		} else {
			fmt.Fprintf(b, fmt.Sprintf(" 0x%%0%dx", vflen), v)
			if f.Name() == MACAddressFuseName {
				fmt.Fprintf(b, fmt.Sprintf(" (MAC: %s)", f.MACAddressString()))
			}
			if vd.Cmp(v) != 0 {
				fmt.Fprintf(b, fmt.Sprintf(" -> 0x%%0%dx", vflen), vd)
			}
		}
	} else {
		fmt.Fprint(b, " (RD)")
	}
	if !f.IsWritable() {
		fmt.Fprint(b, " (WD)")
	}
	return b.String()
}

func (f *Fuse) KeyString() string {
	if !f.IsKey() || !f.IsReadable() {
		return ""
	}
	vd, _ := f.Value(true /* withDiff */)
	// Key blocks store key values in reversed order.
	// Pad to 32 bytes, adding leading zeroes if needed.
	kb := make([]byte, KeyLen)
	copy(kb[KeyLen-len(vd.Bytes()):KeyLen], vd.Bytes())
	reverseKey(kb)
	return hex.EncodeToString(kb)
}

func readFuseBlock(rrw esp.RegReaderWriter, num int) (*FuseBlock, error) {
	b := &FuseBlock{
		rrw: rrw, num: num,
		rBase: blockReadBases[num],
		wBase: blockWriteBases[num],
		data:  make([]uint32, blockLengths[num]),
		diff:  make([]uint32, blockLengths[num]),
	}
	if err := b.Read(); err != nil {
		return nil, errors.Trace(err)
	}
	glog.V(2).Infof("%s", b)
	return b, nil
}

func eFuseCtlWaitIdle(rrw esp.RegReaderWriter) error {
	for i := 0; i < 10; i++ {
		v, err := rrw.ReadReg(eFuseCtlRegCmd)
		if err != nil {
			return errors.Trace(err)
		}
		glog.V(2).Infof("%08x", v)
		if v == 0 {
			return nil
		}
	}
	return errors.Errorf("eFuse controller is busy")
}

func eFuseCtlDoOp(rrw esp.RegReaderWriter, ctlOp eFuseCtlOp) error {
	if err := eFuseCtlWaitIdle(rrw); err != nil {
		return errors.Trace(err)
	}
	var op, cmd uint32
	switch ctlOp {
	case eFuseCtlOpRead:
		op, cmd = eFuseCtlRegConfOpRead, eFuseCtlRegCmdRead
	case eFuseCtlOpWrite:
		op, cmd = eFuseCtlRegConfOpWrite, eFuseCtlRegCmdWrite
	}
	if err := rrw.WriteReg(eFuseCtlRegConf, op); err != nil {
		return errors.Annotatef(err, "failed to set eFuse controller op to %x", op)
	}
	if err := rrw.WriteReg(eFuseCtlRegCmd, cmd); err != nil {
		return errors.Annotatef(err, "failed to set eFuse command to %x", cmd)
	}
	if err := eFuseCtlWaitIdle(rrw); err != nil {
		return errors.Annotatef(err, "%s failed", ctlOp)
	}
	glog.V(1).Infof("%s successful", ctlOp)
	return nil
}

func ReadFuses(rrw esp.RegReaderWriter) ([]*FuseBlock, []*Fuse, map[string]*Fuse, error) {
	var blocks []*FuseBlock

	if err := eFuseCtlDoOp(rrw, eFuseCtlOpRead); err != nil {
		return nil, nil, nil, errors.Annotatef(err, "failed to perform eFuse read operation")
	}

	for i := 0; i < 4; i++ {
		b, err := readFuseBlock(rrw, i)
		if err != nil {
			return nil, nil, nil, errors.Annotatef(err, "failed to read eFuse block %d", i)
		}
		blocks = append(blocks, b)
	}

	fuses := []*Fuse{}
	fusesByName := map[string]*Fuse{}
	for _, fd := range fuseDescriptors {
		f := newFuse(fd, blocks)
		fuses = append(fuses, f)
		fusesByName[f.Name()] = f
	}

	return blocks, fuses, fusesByName, nil
}

func ProgramFuses(rrw esp.RegReaderWriter) error {
	if err := eFuseCtlDoOp(rrw, eFuseCtlOpWrite); err != nil {
		return errors.Annotatef(err, "failed to perform eFuse write operation")
	}
	// re-read new values
	if err := eFuseCtlDoOp(rrw, eFuseCtlOpRead); err != nil {
		return errors.Annotatef(err, "failed to perform eFuse read operation")
	}
	return nil
}

func (op eFuseCtlOp) String() string {
	switch op {
	case eFuseCtlOpRead:
		return "eFuseCtlOpRead"
	case eFuseCtlOpWrite:
		return "eFuseCtlOpWrite"
	}
	return "???"
}
