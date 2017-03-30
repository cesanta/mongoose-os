package atca

import (
	"bytes"
	"encoding/binary"

	"github.com/cesanta/errors"
)

const (
	ConfigSize     = 128
	KeySize        = 32
	PrivateKeySize = 32
	PublicKeySize  = 64
	SignatureSize  = 64
)

type LockZone int

const (
	LockZoneConfig LockZone = 0
	LockZoneData            = 1
)

type Config struct {
	SerialNum   []byte
	Revision    uint32
	Reserved13  uint8
	I2CEnable   bool
	Reserved15  uint8
	I2CAddress  uint8
	Reserved17  uint8
	OTPMode     uint8
	ChipMode    ChipMode
	SlotInfo    []SlotInfo
	Counter0    uint64
	Counter1    uint64
	LastKeyUse0 uint64
	LastKeyUse1 uint64
	UserExtra   uint8
	Selector    uint8
	LockValue   LockMode
	LockConfig  LockMode
	SlotLocked  uint16
	Reserved90  uint8
	Reserved91  uint8
	X509Format  []X509Format
}

type ChipMode struct {
	SelectorWriteOnce bool
	TTLEnable         bool
	WatchDogDuration  WatchdogDuration
}

type WatchdogDuration string

const (
	Watchdog1  WatchdogDuration = "1s"
	Watchdog10                  = "10s"
)

// This struct is not stored in the chip and simply contains
// both SlotConfig and KeyConfig together, for convenience and readability.
type SlotInfo struct {
	Num        uint8
	SlotConfig SlotConfig
	KeyConfig  KeyConfig
}

type SlotConfig struct {
	PrivateKeySlotConfig *PrivateKeySlotConfig `json:",omitempty" yaml:",omitempty"` // For slots 0-7
	ReadKey              *uint8                `json:",omitempty" yaml:",omitempty"` // For slots 8-15
	NoMAC                bool
	LimitedUse           bool
	EncryptRead          bool
	IsSecret             bool
	WriteKey             uint8
	WriteConfig          uint8
}

type PrivateKeySlotConfig struct {
	ExtSignEnable  bool
	IntSignEnable  bool
	ECDHEnable     bool
	ECDHToNextSlot bool
}

type LockMode string

const (
	LockModeLocked   LockMode = "Locked"
	LockModeUnlocked          = "Unlocked"
)

type X509Format struct {
	PublicPosition uint8
	TemplateLength uint8
}

type KeyConfig struct {
	Private          bool    // 0
	PubInfo          bool    // 1
	KeyType          KeyType // 2, 3, 4
	Lockable         bool    // 5
	ReqRandom        bool    // 6
	ReqAuth          bool    // 7
	AuthKey          uint8   // 8,9,10,11
	IntrusionDisable bool    // 12
	// 13 - Reserved
	X509ID uint8 // 14,15
}

type KeyType string

const (
	KeyTypeECC    KeyType = "ECC"
	KeyTypeNonECC         = "NonECC"
)

func parseSlotConfig(num int, scv uint16, kc *KeyConfig) SlotConfig {
	sc := SlotConfig{}
	if kc.Private {
		pkc := &PrivateKeySlotConfig{}
		pkc.ExtSignEnable = (scv&1 != 0)
		pkc.IntSignEnable = (scv&2 != 0)
		pkc.ECDHEnable = (scv&4 != 0)
		pkc.ECDHToNextSlot = (scv&8 != 0)
		sc.PrivateKeySlotConfig = pkc
	} else {
		rk := uint8(scv & 0xF)
		sc.ReadKey = &rk
	}
	sc.NoMAC = (scv&0x10 != 0)
	sc.LimitedUse = (scv&0x20 != 0)
	sc.EncryptRead = (scv&0x40 != 0)
	sc.IsSecret = (scv&0x80 != 0)
	sc.WriteKey = uint8((scv >> 8) & 0xF)
	sc.WriteConfig = uint8((scv >> 12) & 0xF)
	return sc
}

func parseLockMode(b uint8) (LockMode, error) {
	if b == 0x55 {
		return LockModeUnlocked, nil
	} else if b == 0x00 {
		return LockModeLocked, nil
	} else {
		return "", errors.Errorf("unknown data lock mode 0x%02x", b)
	}
}

func parseKeyConfig(num int, kcv uint16) (*KeyConfig, error) {
	var err error
	kc := &KeyConfig{}
	kc.Private = (kcv&1 != 0)
	kc.PubInfo = (kcv&2 != 0)
	kc.KeyType, err = parseKeyType(uint8((kcv >> 2) & 0x7))
	if err != nil {
		return nil, err
	}
	if num > 7 && (kc.KeyType == KeyTypeECC || kc.Private) {
		return nil, errors.Errorf("only slots 0-7 can be used for ECC keys (slot: %d)", num)
	}
	kc.Lockable = (kcv&0x20 != 0)
	kc.ReqRandom = (kcv&0x40 != 0)
	kc.ReqAuth = (kcv&0x80 != 0)
	kc.AuthKey = uint8((kcv >> 8) & 0xF)
	kc.IntrusionDisable = (kcv&0x1000 != 0)
	kc.X509ID = uint8((kcv >> 14) & 0x3)
	return kc, nil
}

func parseKeyType(b uint8) (KeyType, error) {
	if b == 4 {
		return KeyTypeECC, nil
	} else if b == 7 {
		return KeyTypeNonECC, nil
	} else {
		return "", errors.Errorf("unknown key type %d", b)
	}
}

func ParseBinaryConfig(cd []byte) (*Config, error) {
	cb := bytes.NewBuffer(cd)
	var b uint8
	var err error
	if len(cd) != ConfigSize {
		return nil, errors.Errorf("expected %d bytes, got %d", ConfigSize, len(cd))
	}
	c := &Config{}
	c.SerialNum = make([]byte, 9)
	cb.Read(c.SerialNum[0:4])
	binary.Read(cb, binary.BigEndian, &c.Revision)
	cb.Read(c.SerialNum[4:9])
	binary.Read(cb, binary.BigEndian, &c.Reserved13)
	binary.Read(cb, binary.BigEndian, &b)
	c.I2CEnable = (b&1 != 0)
	binary.Read(cb, binary.BigEndian, &c.Reserved15)
	binary.Read(cb, binary.BigEndian, &c.I2CAddress)
	binary.Read(cb, binary.BigEndian, &c.Reserved17)
	binary.Read(cb, binary.BigEndian, &c.OTPMode)
	binary.Read(cb, binary.BigEndian, &b)
	c.ChipMode.SelectorWriteOnce = (b&1 != 0)
	c.ChipMode.TTLEnable = (b&2 != 0)
	if b&4 != 0 {
		c.ChipMode.WatchDogDuration = Watchdog10
	} else {
		c.ChipMode.WatchDogDuration = Watchdog1
	}
	var scvs [16]uint16
	for i := 0; i < 16; i++ {
		// We need to know slot's KeyConfig.Private setting to know how to parse SlotConfig.ReadKey.
		binary.Read(cb, binary.LittleEndian, &scvs[i])
	}
	binary.Read(cb, binary.BigEndian, &c.Counter0)
	binary.Read(cb, binary.BigEndian, &c.Counter1)
	binary.Read(cb, binary.BigEndian, &c.LastKeyUse0)
	binary.Read(cb, binary.BigEndian, &c.LastKeyUse1)
	binary.Read(cb, binary.BigEndian, &c.UserExtra)
	binary.Read(cb, binary.BigEndian, &c.Selector)
	binary.Read(cb, binary.BigEndian, &b)
	c.LockValue, err = parseLockMode(b)
	if err != nil {
		return nil, errors.Annotatef(err, "LockValue")
	}
	binary.Read(cb, binary.BigEndian, &b)
	c.LockConfig, err = parseLockMode(b)
	if err != nil {
		return nil, errors.Annotatef(err, "LockConfig")
	}
	binary.Read(cb, binary.LittleEndian, &c.SlotLocked)
	binary.Read(cb, binary.BigEndian, &c.Reserved90)
	binary.Read(cb, binary.BigEndian, &c.Reserved91)
	for i := 0; i < 4; i++ {
		var fc X509Format
		binary.Read(cb, binary.BigEndian, &b)
		fc.PublicPosition = (b & 0xF)
		fc.TemplateLength = ((b >> 4) & 0xF)
		c.X509Format = append(c.X509Format, fc)
	}
	for i := 0; i < 16; i++ {
		var kcv uint16
		binary.Read(cb, binary.LittleEndian, &kcv)
		kc, err := parseKeyConfig(i, kcv)
		if err != nil {
			return nil, errors.Annotatef(err, "KeyConfig %d", i)
		}
		c.SlotInfo = append(c.SlotInfo, SlotInfo{
			Num:        uint8(i),
			SlotConfig: parseSlotConfig(i, scvs[i], kc),
			KeyConfig:  *kc,
		})
	}
	return c, nil
}

func writeSlotConfig(cb *bytes.Buffer, si SlotInfo) error {
	var scv uint16
	sc := &si.SlotConfig
	if si.KeyConfig.Private {
		pkc := sc.PrivateKeySlotConfig
		if pkc == nil {
			return errors.Errorf("no PrivateKeyConfig")
		}
		if pkc.ExtSignEnable {
			scv |= 1
		}
		if pkc.IntSignEnable {
			scv |= 2
		}
		if pkc.ECDHEnable {
			scv |= 4
		}
		if pkc.ECDHToNextSlot {
			scv |= 8
		}
	} else if si.Num < 16 {
		if sc.ReadKey == nil {
			return errors.Errorf("no ReadKey")
		}
		scv = uint16(*sc.ReadKey)
	} else {
		return errors.Errorf("invalid slot number")
	}
	if sc.NoMAC {
		scv |= 0x10
	}
	if sc.LimitedUse {
		scv |= 0x20
	}
	if sc.EncryptRead {
		scv |= 0x40
	}
	if sc.IsSecret {
		scv |= 0x80
	}
	scv |= (uint16(sc.WriteKey&0xF) << 8)
	scv |= (uint16(sc.WriteConfig&0xF) << 12)
	binary.Write(cb, binary.LittleEndian, scv)
	return nil
}

func writeKeyConfig(cb *bytes.Buffer, si SlotInfo) error {
	var kcv uint16
	kc := &si.KeyConfig
	if kc.Private {
		kcv |= 1
	}
	if kc.PubInfo {
		kcv |= 2
	}
	switch kc.KeyType {
	case KeyTypeECC:
		kcv |= (uint16(4) << 2)
	case KeyTypeNonECC:
		kcv |= (uint16(7) << 2)
	default:
		return errors.Errorf("unknown key type '%s'", kc.KeyType)
	}
	if kc.Lockable {
		kcv |= 0x20
	}
	if kc.ReqRandom {
		kcv |= 0x40
	}
	if kc.ReqAuth {
		kcv |= 0x80
	}
	kcv |= (uint16(kc.AuthKey&0xF) << 8)
	if kc.IntrusionDisable {
		kcv |= 0x1000
	}
	kcv |= (uint16(kc.X509ID&0x3) << 14)
	binary.Write(cb, binary.LittleEndian, kcv)
	return nil
}

func writeLockMode(cb *bytes.Buffer, lm LockMode) error {
	var b uint8
	switch lm {
	case LockModeUnlocked:
		b = 0x55
	case LockModeLocked:
		b = 0
	default:
		return errors.Errorf("unknown lock mode %s", lm)
	}
	return binary.Write(cb, binary.BigEndian, b)
}

func WriteBinaryConfig(c *Config) ([]byte, error) {
	var b uint8
	var err error
	cd := make([]byte, 0, ConfigSize)
	cb := bytes.NewBuffer(cd)
	sn := c.SerialNum
	if sn == nil {
		sn = make([]byte, 9)
	}
	cb.Write(sn[0:4])
	binary.Write(cb, binary.BigEndian, c.Revision)
	cb.Write(sn[4:9])
	binary.Write(cb, binary.BigEndian, c.Reserved13)
	b = 0
	if c.I2CEnable {
		b |= 1
	}
	binary.Write(cb, binary.BigEndian, b)
	binary.Write(cb, binary.BigEndian, c.Reserved15)
	binary.Write(cb, binary.BigEndian, c.I2CAddress)
	binary.Write(cb, binary.BigEndian, c.Reserved17)
	binary.Write(cb, binary.BigEndian, c.OTPMode)
	b = 0
	if c.ChipMode.SelectorWriteOnce {
		b |= 1
	}
	if c.ChipMode.TTLEnable {
		b |= 2
	}
	switch c.ChipMode.WatchDogDuration {
	case Watchdog1:
		break
	case Watchdog10:
		b |= 4
	default:
		return nil, errors.Errorf("unknown watchdog duration %s", c.ChipMode.WatchDogDuration)
	}
	binary.Write(cb, binary.BigEndian, b)
	for i := 0; i < 16; i++ {
		err = writeSlotConfig(cb, c.SlotInfo[i])
		if err != nil {
			return nil, errors.Annotatef(err, "SlotConfig %d", i)
		}
	}
	binary.Write(cb, binary.BigEndian, c.Counter0)
	binary.Write(cb, binary.BigEndian, c.Counter1)
	binary.Write(cb, binary.BigEndian, c.LastKeyUse0)
	binary.Write(cb, binary.BigEndian, c.LastKeyUse1)
	binary.Write(cb, binary.BigEndian, c.UserExtra)
	binary.Write(cb, binary.BigEndian, c.Selector)
	err = writeLockMode(cb, c.LockValue)
	if err != nil {
		return nil, errors.Annotatef(err, "LockValue")
	}
	err = writeLockMode(cb, c.LockConfig)
	if err != nil {
		return nil, errors.Annotatef(err, "LockConfig")
	}
	binary.Write(cb, binary.LittleEndian, c.SlotLocked)
	b = 0
	binary.Write(cb, binary.BigEndian, c.Reserved90)
	binary.Write(cb, binary.BigEndian, c.Reserved91)
	for i := 0; i < 4; i++ {
		fc := &c.X509Format[i]
		b = 0
		b |= (fc.PublicPosition & 0xF)
		b |= ((fc.TemplateLength & 0xF) << 4)
		binary.Write(cb, binary.BigEndian, b)
	}
	for i := 0; i < 16; i++ {
		err = writeKeyConfig(cb, c.SlotInfo[i])
		if err != nil {
			return nil, errors.Annotatef(err, "KeyConfig %d", i)
		}
	}
	return cb.Bytes(), nil
}
