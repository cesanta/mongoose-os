package main

import (
	"fmt"
	"golang.org/x/net/context"
	"os"

	"cesanta.com/mos/dev"
	"github.com/cesanta/errors"
	flag "github.com/spf13/pflag"
)

func wifi(ctx context.Context, devConn *dev.DevConn) error {
	args := flag.Args()
	if len(args) != 3 {
		return errors.Errorf("Usage: %s wifi WIFI_NETWORK_NAME WIFI_PASSWORD", os.Args[0])
	}
	params := []string{
		"wifi.ap.enable=false",
		"wifi.sta.enable=true",
		fmt.Sprintf("wifi.sta.ssid=%s", args[1]),
		fmt.Sprintf("wifi.sta.pass=%s", args[2]),
	}
	return internalConfigSet(ctx, devConn, params)
}
