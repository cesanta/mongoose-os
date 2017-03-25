// +build noflash

package main

import (
	"context"

	"cesanta.com/mos/dev"
	"github.com/cesanta/errors"
)

func esp32EFuseGet(ctx context.Context, devConn *dev.DevConn) error {
	return errors.NotImplementedf("esp32-efuse-get")
}

func esp32EFuseSet(ctx context.Context, devConn *dev.DevConn) error {
	return errors.NotImplementedf("esp32-efuse-set")
}
