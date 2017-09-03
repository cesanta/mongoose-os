// +build !linux,!windows,!darwin no_libudev

package cc3220

import (
	"cesanta.com/mos/flash/cc32xx"
	"github.com/cesanta/errors"
)

func NewCC3220DeviceControl(port string) (cc32xx.DeviceControl, error) {
	return nil, errors.NotImplementedf("")
}
