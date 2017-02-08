package main

import (
	"context"
	"fmt"
	"strings"

	"cesanta.com/clubby"
	fwconfig "cesanta.com/fw/defs/config"
	"cesanta.com/mos/dev"
	"github.com/cesanta/errors"
	flag "github.com/spf13/pflag"
)

var (
	noSave   bool
	noReboot bool
)

// register advanced flash specific commands
func init() {
	flag.BoolVar(&noReboot, "no-reboot", false,
		"Save config but don't reboot the device.")
	flag.BoolVar(&noSave, "no-save", false,
		"Don't save config and don't reboot the device")

	hiddenFlags = append(hiddenFlags, "no-reboot", "no-save")
}

func configGet(ctx context.Context, devConn *dev.DevConn) error {
	path := ""

	args := flag.Args()[1:]

	if len(args) > 1 {
		return errors.Errorf("only one path to value is allowed")
	}

	// If path is given, use it; otherwise, an empty string will be assumed,
	// which means "all config"
	if len(args) == 1 {
		path = args[0]
	}

	// Get all config from the attached device
	devConf, err := devConn.GetConfig(ctx)
	if err != nil {
		return errors.Trace(err)
	}

	// Try to get requested value
	val, err := devConf.Get(path)
	if err != nil {
		return errors.Trace(err)
	}

	fmt.Println(val)

	return nil
}

func configSet(ctx context.Context, devConn *dev.DevConn) error {
	return internalConfigSet(ctx, devConn, flag.Args()[1:])
}

func internalConfigSet(
	ctx context.Context, devConn *dev.DevConn, args []string,
) error {
	if len(args) < 1 {
		return errors.Errorf("at least one path.to.value=value pair should be given")
	}

	// Get all config from the attached device
	reportf("Getting configuration...")
	devConf, err := devConn.GetConfig(ctx)
	if err != nil {
		return errors.Trace(err)
	}

	paramValues, err := parseParamValues(args)
	if err != nil {
		return errors.Trace(err)
	}

	// Try to set all provided values
	for path, val := range paramValues {
		err := devConf.Set(path, val)
		if err != nil {
			return errors.Trace(err)
		}
	}

	return configSetAndSave(ctx, devConn, devConf)
}

func configSetAndSave(ctx context.Context, devConn *dev.DevConn, devConf *dev.DevConf) error {
	// save changed conf
	reportf("Setting new configuration...")
	err := devConn.SetConfig(ctx, devConf)
	if err != nil {
		return errors.Trace(err)
	}

	if !noSave {
		if noReboot {
			reportf("Saving...")
		} else {
			reportf("Saving and rebooting...")
		}
		err = devConn.CConf.Save(ctx, &fwconfig.SaveArgs{
			Reboot: clubby.Bool(!noReboot),
		})
		if err != nil {
			return errors.Trace(err)
		}

		if !noReboot {
			waitForReboot()
		}
	}

	return nil
}

func parseParamValues(args []string) (map[string]string, error) {
	ret := map[string]string{}
	for _, a := range args {
		// Split arg into two substring by "=" (so, param name name cannot contain
		// "=", but value can)
		subs := strings.SplitN(a, "=", 2)
		if len(subs) < 2 {
			return nil, errors.Errorf("missing value for %q", a)
		}
		ret[subs[0]] = subs[1]
	}
	return ret, nil
}
