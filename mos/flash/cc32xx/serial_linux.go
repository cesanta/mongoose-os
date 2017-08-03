package cc32xx

import (
	"os/exec"
	"regexp"

	"github.com/cesanta/errors"
	"github.com/golang/glog"
)

func GetUSBSerialNumberForPort(port string) (string, error) {
	out, err := exec.Command("udevadm", "info", "--name", port).Output()
	if err != nil {
		return "", errors.Trace(err)
	}
	glog.V(1).Infof("udevadm output:\n%s", out)
	m := regexp.MustCompile(` ID_SERIAL_SHORT=(\S+)`).FindSubmatch(out)
	if m == nil {
		return "", errors.Errorf("No serial number in udevadm output")
	}
	return string(m[1]), nil
}
