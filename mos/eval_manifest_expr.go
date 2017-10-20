package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"os"
	"strings"

	"golang.org/x/net/context"

	"cesanta.com/mos/dev"
	"cesanta.com/mos/interpreter"
	"cesanta.com/mos/manifest_parser"
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

	// Create map of given module locations, via --module flag(s)
	customModuleLocations := map[string]string{}
	for _, m := range *modules {
		parts := strings.SplitN(m, ":", 2)
		customModuleLocations[parts[0]] = parts[1]
	}

	interp := interpreter.NewInterpreter(newMosVars())

	appDir, err := getCodeDirAbs()
	if err != nil {
		return errors.Trace(err)
	}

	// Never update libs on that command
	*noLibsUpdate = true

	logWriterStderr = os.Stderr

	if *verbose {
		logWriter = logWriterStderr
	} else {
		logWriter = &bytes.Buffer{}
	}

	compProvider := compProviderReal{
		bParams:   bParams,
		logWriter: logWriter,
	}

	manifest, _, err := manifest_parser.ReadManifestFinal(
		appDir, bParams.Platform, logWriter, interp,
		&manifest_parser.ReadManifestCallbacks{ComponentProvider: &compProvider}, false, *preferPrebuiltLibs,
	)
	if err != nil {
		return errors.Trace(err)
	}

	if err := interpreter.SetManifestVars(interp.MVars, manifest); err != nil {
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

	// TODO(dfrank): probably add a flag whether to expand vars (the default
	// being to expand)
	sdata, err := interpreter.ExpandVars(interp, string(data))
	if err != nil {
		return errors.Trace(err)
	}

	fmt.Println(sdata)

	return nil
}
