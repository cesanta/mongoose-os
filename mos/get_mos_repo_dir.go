package main

import (
	"fmt"
	"io"
	"os"
	"path/filepath"
	"time"

	"golang.org/x/net/context"

	"cesanta.com/mos/dev"
	"cesanta.com/mos/interpreter"
	"cesanta.com/mos/manifest_parser"
	"github.com/cesanta/errors"
)

func getMosRepoDir(ctx context.Context, devConn *dev.DevConn) error {
	logWriterStderr = io.MultiWriter(&logBuf, os.Stderr)
	logWriter = io.MultiWriter(&logBuf)
	if *verbose {
		logWriter = logWriterStderr
	}

	cll, err := getCustomLibLocations()
	if err != nil {
		return errors.Trace(err)
	}

	bParams := &buildParams{
		Platform:           *platform,
		CustomLibLocations: cll,
	}

	appDir, err := getCodeDirAbs()
	if err != nil {
		return errors.Trace(err)
	}

	interp := interpreter.NewInterpreter(newMosVars())

	manifest, _, err := manifest_parser.ReadManifest(appDir, bParams.Platform, interp)
	if err != nil {
		return errors.Trace(err)
	}

	mosDirEffective, err := getMosDirEffective(manifest.MongooseOsVersion, time.Hour*99999)
	if err != nil {
		return errors.Trace(err)
	}

	mosDirEffectiveAbs, err := filepath.Abs(mosDirEffective)
	if err != nil {
		return errors.Annotatef(err, "getting absolute path of %q", mosDirEffective)
	}

	fmt.Println(mosDirEffectiveAbs)
	return nil
}
