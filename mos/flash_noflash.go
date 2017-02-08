// +build noflash

package main

import (
	"context"

	"cesanta.com/mos/dev"
	"github.com/cesanta/errors"
)

func flash(ctx context.Context, devConn *dev.DevConn) error {
	return errors.NotImplementedf("this build was built without flashing support")
}

func esp32EncryptImage(ctx context.Context, devConn *dev.DevConn) error {
	return errors.NotImplementedf("this build was built without flashing support")
}
