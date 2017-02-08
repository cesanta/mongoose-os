// +build !linux,!windows,!darwin

package cc3200

import (
	"github.com/cesanta/errors"
)

func NewCC3200DeviceControl(port string) (DeviceControl, error) {
	return nil, errors.NotImplementedf("")
}
