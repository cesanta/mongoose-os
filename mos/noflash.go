// +build noflash

package main

import (
	"context"

	"cesanta.com/mos/dev"
	"github.com/cesanta/errors"
)

func esp32EFuseGet(ctx context.Context, devConn *dev.DevConn) error {
	return errors.NotImplementedf("esp32-efuse-get: this build was built without flashing support")
}

func esp32EFuseSet(ctx context.Context, devConn *dev.DevConn) error {
	return errors.NotImplementedf("esp32-efuse-set: this build was built without flashing support")
}

func esp32EncryptImage(ctx context.Context, devConn *dev.DevConn) error {
	return errors.NotImplementedf("esp32-encrypt-image: this build was built without flashing support")
}

func esp32GenKey(ctx context.Context, devConn *dev.DevConn) error {
	return errors.NotImplementedf("esp32-gen-key: this build was built without flashing support")
}

func flash(ctx context.Context, devConn *dev.DevConn) error {
	return errors.NotImplementedf("flash: this build was built without flashing support")
}

func flashRead(ctx context.Context, devConn *dev.DevConn) error {
	return errors.NotImplementedf("flash-read: this build was built without flashing support")
}
