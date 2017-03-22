package esp32

import (
	"cesanta.com/mos/flash/esp/rom_client"
	"github.com/cesanta/errors"
	"github.com/golang/glog"
)

type fakeFuseController struct {
	regs        map[uint32]uint32
	busyCounter int
}

func NewFakeFuseController() rom_client.RegReaderWriter {
	return &fakeFuseController{
		regs: map[uint32]uint32{
			// eFuse register block 0
			// Read
			0x6001a000: 0x00110080,
			0x6001a004: 0xc40031bc,
			0x6001a008: 0x0098240a,
			0x6001a00c: 0x00000000,
			0x6001a010: 0x00000036,
			0x6001a014: 0xf0000000,
			0x6001a018: 0x000003c0,
			// Write
			0x6001a01c: 0x00000000,
			0x6001a020: 0x00000000,
			0x6001a024: 0x00000000,
			0x6001a028: 0x00000000,
			0x6001a02c: 0x00000000,
			0x6001a030: 0x00000000,
			0x6001a034: 0x00000000,

			// eFuse register block 1 - read
			0x6001a038: 0x00000000,
			0x6001a03c: 0x00000000,
			0x6001a040: 0x00000000,
			0x6001a044: 0x00000000,
			0x6001a048: 0x00000000,
			0x6001a04c: 0x00000000,
			0x6001a050: 0x00000000,
			0x6001a054: 0x00000000,
			// eFuse register block 2 - read
			0x6001a058: 0x00000000,
			0x6001a05c: 0x00000000,
			0x6001a060: 0x00000000,
			0x6001a064: 0x00000000,
			0x6001a068: 0x00000000,
			0x6001a06c: 0x00000000,
			0x6001a070: 0x00000000,
			0x6001a074: 0x00000000,
			// eFuse register block 3 - read
			0x6001a078: 0xdea4bc74,
			0x6001a07c: 0x29ed0719,
			0x6001a080: 0x78de2d7c,
			0x6001a084: 0x52836465,
			0x6001a088: 0xfe0e0810,
			0x6001a08c: 0x3a65350f,
			0x6001a090: 0x2732fd51,
			0x6001a094: 0xcd5b03d7,

			// eFuse register block 1 - write
			0x6001a098: 0x00000000,
			0x6001a09c: 0x00000000,
			0x6001a0a0: 0x00000000,
			0x6001a0a4: 0x00000000,
			0x6001a0a8: 0x00000000,
			0x6001a0ac: 0x00000000,
			0x6001a0b0: 0x00000000,
			0x6001a0b4: 0x00000000,
			// eFuse register block 2 - write
			0x6001a0b8: 0x00000000,
			0x6001a0bc: 0x00000000,
			0x6001a0c0: 0x00000000,
			0x6001a0c4: 0x00000000,
			0x6001a0c8: 0x00000000,
			0x6001a0cc: 0x00000000,
			0x6001a0d0: 0x00000000,
			0x6001a0d4: 0x00000000,
			// eFuse register block 3 - write
			0x6001a0d8: 0x00000000,
			0x6001a0dc: 0x00000000,
			0x6001a0e0: 0x00000000,
			0x6001a0e4: 0x00000000,
			0x6001a0e8: 0x00000000,
			0x6001a0ec: 0x00000000,
			0x6001a0f0: 0x00000000,
			0x6001a0f4: 0x00000000,

			eFuseCtlRegConf: 0, // eFuse controller conf (op) register
			eFuseCtlRegCmd:  0, // eFuse controller command register
		},
	}
}

func (frc *fakeFuseController) ReadReg(reg uint32) (uint32, error) {
	value, found := frc.regs[reg]
	if !found {
		return 0, errors.Errorf("unknown register 0x%08x", reg)
	}
	if reg == eFuseCtlRegCmd {
		frc.busyCounter--
		if frc.busyCounter <= 0 {
			frc.regs[reg] = 0
			value = 0
		}
	}
	glog.V(3).Infof("ReadReg(0x%08x) => 0x%08x", reg, value)
	return value, nil
}

func (frc *fakeFuseController) WriteReg(reg, value uint32) error {
	_, found := frc.regs[reg]
	if !found {
		return errors.Errorf("unknown register 0x%08x", reg)
	}
	switch reg {
	case eFuseCtlRegConf:
		if value != eFuseCtlRegConfOpRead && value != eFuseCtlRegConfOpWrite {
			return errors.Errorf("invalid op 0x%08x", value)
		}
	case eFuseCtlRegCmd:
		if value != eFuseCtlRegCmdRead && value != eFuseCtlRegCmdWrite {
			return errors.Errorf("invalid command 0x%08x", value)
		}
		frc.busyCounter = 3
	}
	frc.regs[reg] = value
	glog.V(3).Infof("WriteReg(0x%08x, 0x%08x)", reg, value)
	return nil
}

func (frc *fakeFuseController) Disconnect() {
}
