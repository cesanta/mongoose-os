package main

import (
	"encoding/json"
	"fmt"
	"golang.org/x/net/context"

	"cesanta.com/common/go/mgrpc/frame"
	"cesanta.com/common/go/ourjson"
	"cesanta.com/mos/dev"

	"github.com/cesanta/errors"
	flag "github.com/spf13/pflag"
)

func isJSONString(s string) bool {
	var js string
	return json.Unmarshal([]byte(s), &js) == nil
}

func isJSON(s string) bool {
	var js json.RawMessage
	return json.Unmarshal([]byte(s), &js) == nil
}

func callDeviceService(
	ctx context.Context, devConn *dev.DevConn, method string, args string,
) (string, error) {
	if args != "" && !isJSON(args) {
		return "", errors.Errorf("Args [%s] is not a valid JSON string", args)
	}

	cmd := &frame.Command{Cmd: method}
	if args != "" {
		cmd.Args = ourjson.RawJSON([]byte(args))
	}

	resp, err := devConn.RPC.Call(ctx, devConn.Dest, cmd)
	if err != nil {
		return "", errors.Trace(err)
	}

	if resp.Status != 0 {
		return "", errors.Errorf("remote error: %s", resp.StatusMsg)
	}

	// TODO(dfrank): instead of that, we should probably add a separate function
	// for rebooting
	if method == "Sys.Reboot" {
		waitForReboot()
	}

	// Ignoring errors here, cause response could be empty which is a success
	str, _ := json.MarshalIndent(resp.Response, "", "  ")
	return string(str), nil
}

func call(ctx context.Context, devConn *dev.DevConn) error {
	args := flag.Args()[1:]
	if len(args) < 1 {
		return errors.Errorf("method required")
	}

	params := ""
	if len(args) > 1 {
		params = args[1]
	}

	if *timeout > 0 {
		ctx, _ = context.WithTimeout(ctx, *timeout)
	}

	result, err := callDeviceService(ctx, devConn, args[0], params)
	if err != nil {
		return err
	}

	fmt.Println(result)
	return nil
}
