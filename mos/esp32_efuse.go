// +build !noflash

package main

import (
	"golang.org/x/net/context"
	"io/ioutil"
	"math/big"
	"strings"

	"cesanta.com/mos/dev"
	"cesanta.com/mos/flash/esp"
	"cesanta.com/mos/flash/esp/rom_client"
	"cesanta.com/mos/flash/esp32"
	"github.com/cesanta/errors"
	flag "github.com/spf13/pflag"
)

var (
	esp32FakeFuses = false
)

func init() {
	flag.BoolVar(&esp32FakeFuses, "esp32-fake-fuses", false, "Use fake eFuse controller implementation, for testing")
}

func getRRW() (esp.RegReaderWriter, error) {
	if esp32FakeFuses {
		return esp32.NewFakeFuseController(), nil
	}

	port, err := getPort()
	if err != nil {
		return nil, errors.Trace(err)
	}
	espFlashOpts.ControlPort = port
	rc, err := rom_client.ConnectToROM(esp.ChipESP32, &espFlashOpts)
	if err != nil {
		return nil, errors.Trace(err)
	}

	return rc, nil
}

func esp32EFuseGet(ctx context.Context, devConn *dev.DevConn) error {
	rrw, err := getRRW()
	if err != nil {
		return errors.Trace(err)
	}
	defer rrw.Disconnect()

	_, fuses, fusesByName, err := esp32.ReadFuses(rrw)
	if err != nil {
		return errors.Annotatef(err, "failed to read eFuses")
	}

	if len(flag.Args()) >= 2 {
		for _, fuseName := range flag.Args()[1:] {
			f, found := fusesByName[fuseName]
			if !found {
				return errors.Errorf("invalid fuse %s", fuseName)
			}
			reportf("%s", f)
		}
	} else {
		for _, f := range fuses {
			reportf("%s", f)
		}
	}

	return nil
}

func esp32EFuseSet(ctx context.Context, devConn *dev.DevConn) error {

	if len(flag.Args()) < 2 {
		return errors.Errorf("one or more ops required. op is 'fuse=value', 'fuse=@file' or 'fuse.{WD|RD}=1'")
	}

	rrw, err := getRRW()
	if err != nil {
		return errors.Trace(err)
	}
	defer rrw.Disconnect()

	blocks, fuses, fusesByName, err := esp32.ReadFuses(rrw)
	if err != nil {
		return errors.Annotatef(err, "failed to read eFuses")
	}

	printFuses := map[string]bool{}
	for _, op := range flag.Args()[1:] {
		parts := strings.SplitN(op, "=", 2)
		if len(parts) < 2 {
			return errors.Errorf("invalid op %q, should be 'fuse=value', 'fuse=@file' or 'fuse.{WD|RD}=1'", op)
		}
		fuseName, flagName, valueStr := parts[0], "", parts[1]
		nameParts := strings.SplitN(fuseName, ".", 2)
		if len(nameParts) == 2 {
			fuseName, flagName = nameParts[0], nameParts[1]
		}
		f, found := fusesByName[fuseName]
		if !found {
			return errors.Errorf("invalid fuse %s", fuseName)
		}
		if flagName != "" {
			switch flagName {
			case "RD":
				if valueStr == "1" {
					err = f.SetReadDisable()
					printFuses[esp32.ReadDisableFuseName] = true
				} else {
					err = errors.Errorf("%s: ReadDisable flag can only be set to 1", fuseName)
				}
			case "WD":
				if valueStr == "1" {
					printFuses[esp32.WriteDisableFuseName] = true
					err = f.SetWriteDisable()
				} else {
					err = errors.Errorf("%s: WriteDisable flag can only be set to 1", fuseName)
				}
			default:
				err = errors.Errorf("%s: unknown flag %q", fuseName, flagName)
			}
			if err != nil {
				return err
			}
		} else {
			if strings.HasPrefix(valueStr, "@") {
				fname := valueStr[1:]
				var data []byte
				data, err = ioutil.ReadFile(fname)
				if err != nil {
					return errors.Annotatef(err, "%s: failed to read %q", fuseName, fname)
				}
				err = f.SetKeyValue(data)
			} else {
				value := big.NewInt(0)
				if err = value.UnmarshalText([]byte(valueStr)); err != nil {
					return errors.Annotatef(err, "invalid value for %s", fuseName)
				}
				err = f.SetValue(value)
			}
			if err != nil {
				return errors.Annotatef(err, "%s: failed to set value", fuseName)
			}
			printFuses[fuseName] = true
		}
	}

	for _, f := range fuses {
		if printFuses[f.Name()] {
			reportf("%s", f)
		}
	}

	haveDiffs := false
	for i, b := range blocks {
		haveDiffs = haveDiffs || b.HasDiffs()
		if err := b.WriteDiffs(); err != nil {
			return errors.Annotatef(err, "failed to write fuse block %d", i)
		}
	}

	if haveDiffs {
		if !*dryRun {
			reportf("Programming eFuses...")
			err = esp32.ProgramFuses(rrw)
			if err == nil {
				reportf("Success")
			}
		} else {
			reportf("Not applying changes, set --dry-run=false to burn the fuses.")
		}
	} else {
		reportf("No changes to apply")
	}

	return err
}
