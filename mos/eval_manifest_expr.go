package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"os"

	"golang.org/x/net/context"

	"cesanta.com/mos/common/paths"
	"cesanta.com/mos/dev"
	"cesanta.com/mos/interpreter"
	"github.com/cesanta/errors"
	flag "github.com/spf13/pflag"
)

func evalManifestExpr(ctx context.Context, devConn *dev.DevConn) error {
	cll, err := getCustomLibLocations()
	if err != nil {
		return errors.Trace(err)
	}

	args := flag.Args()[1:]

	if len(args) == 0 {
		return errors.Errorf("expression is required")
	}

	expr := args[0]

	bParams := &buildParams{
		Platform:           *platform,
		CustomLibLocations: cll,
	}

	interp := interpreter.NewInterpreter(newMosVars())

	appDir, err := getCodeDirAbs()
	if err != nil {
		return errors.Trace(err)
	}

	// Never update libs on that command
	*noLibsUpdate = true

	var logWriter io.Writer

	if *verbose {
		logWriter = os.Stderr
	} else {
		logWriter = &bytes.Buffer{}
	}

	_, _, err = readManifestWithLibs(
		appDir, bParams, logWriter, paths.LibsDir, interp,
		false /* require arch */, false /* skip clean */, true, /* finalize */
	)
	if err != nil {
		return errors.Trace(err)
	}

	res, err := interp.EvaluateExpr(expr)
	if err != nil {
		return errors.Trace(err)
	}

	data, err := json.MarshalIndent(res, "", "  ")
	if err != nil {
		return errors.Trace(err)
	}

	fmt.Println(string(data))

	return nil
}
