package common

import (
	"encoding/hex"
	"fmt"
	"os"

	"github.com/golang/glog"
)

func Reportf(f string, args ...interface{}) {
	fmt.Fprintf(os.Stderr, f+"\n", args...)
	glog.Infof(f, args...)
}

func LimitStr(b []byte, n int) string {
	if len(b) <= n {
		return hex.EncodeToString(b)
	} else {
		return hex.EncodeToString(b[:n]) + "..."
	}
}
