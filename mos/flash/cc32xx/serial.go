// +build !linux

package cc32xx

import "github.com/cesanta/errors"

func GetUSBSerialNumberForPort(port string) (string, error) {
	// Not supported
	return "", errors.NotImplementedf("")
}
