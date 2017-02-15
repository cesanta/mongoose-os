package stm32

import (
	"io/ioutil"
	"os"
	"path/filepath"
	"time"

	"cesanta.com/mos/flash/common"
	"github.com/cesanta/errors"
)

type FlashOpts struct {
	ShareName string
	Timeout   time.Duration
}

func Flash(fw *common.FirmwareBundle, opts *FlashOpts) error {
	data, err := fw.GetPartData("boot")
	if err != nil {
		return errors.Annotatef(err, "invalid manifest")
	}

	name := filepath.Join(opts.ShareName, fw.Parts["boot"].Src)

	common.Reportf("Copying %s to %s...", fw.Parts["boot"].Src, opts.ShareName)
	err = ioutil.WriteFile(name, data, 0)
	if err != nil {
		return errors.Trace(err)
	}

	common.Reportf("Waiting for operation to complete...")

	start := time.Now()

	for {
		_, err = os.Stat(name)
		if err != nil {
			if os.IsNotExist(err) {
				// File is disappeared: operation ok
				return nil
			} else {
				return errors.Annotatef(err, "flash failed")
			}
		}

		if time.Since(start) > opts.Timeout {
			return errors.Errorf("timeout")
		}

		time.Sleep(1000 * time.Millisecond)
	}

	return nil
}
