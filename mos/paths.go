package main

import (
	"flag"
	"fmt"
	"os"
	"path/filepath"
	"runtime"
	"strings"

	"github.com/cesanta/errors"
)

var (
	dirTplMosVersion = "${mos.version}"

	libsDirTpl    = fmt.Sprintf("~/.mos/libs-%s", dirTplMosVersion)
	appsDirTpl    = fmt.Sprintf("~/.mos/apps-%s", dirTplMosVersion)
	modulesDirTpl = fmt.Sprintf("~/.mos/modules-%s", dirTplMosVersion)

	tmpDir     = ""
	libsDir    = ""
	appsDir    = ""
	modulesDir = ""

	// TODO(dfrank): remove them after a while (2017/08/03)
	libsDirOld    = "~/.mos/libs"
	appsDirOld    = "~/.mos/apps"
	modulesDirOld = "~/.mos/modules"

	stateFilepath = ""
)

func init() {
	flag.StringVar(&tmpDir, "temp-dir", "~/.mos/tmp", "Directory to store temporary files")
	flag.StringVar(&libsDir, "libs-dir", libsDirTpl, "Directory to store libraries into")
	flag.StringVar(&appsDir, "apps-dir", appsDirTpl, "Directory to store apps into")
	flag.StringVar(&modulesDir, "modules-dir", modulesDirTpl, "Directory to store modules into")

	flag.StringVar(&stateFilepath, "state-file", "~/.mos/state.json", "Where to store internal mos state")
}

// pathsInit() should be called after all flags are parsed
func pathsInit() error {
	var err error
	tmpDir, err = normalizePath(tmpDir, getMosVersion())
	if err != nil {
		return errors.Trace(err)
	}

	libsDir, err = normalizePath(libsDir, getMosVersion())
	if err != nil {
		return errors.Trace(err)
	}

	appsDir, err = normalizePath(appsDir, getMosVersion())
	if err != nil {
		return errors.Trace(err)
	}

	modulesDir, err = normalizePath(modulesDir, getMosVersion())
	if err != nil {
		return errors.Trace(err)
	}

	// TODO(dfrank) remove after a while (2017/08/03) {{{
	libsDirOld, err = normalizePath(libsDirOld, "")
	if err != nil {
		return errors.Trace(err)
	}

	appsDirOld, err = normalizePath(appsDirOld, "")
	if err != nil {
		return errors.Trace(err)
	}

	modulesDirOld, err = normalizePath(modulesDirOld, "")
	if err != nil {
		return errors.Trace(err)
	}
	// }}}

	stateFilepath, err = normalizePath(stateFilepath, getMosVersion())
	if err != nil {
		return errors.Trace(err)
	}

	if err := os.MkdirAll(tmpDir, 0777); err != nil {
		return errors.Trace(err)
	}

	return nil
}

func normalizePath(p, version string) (string, error) {
	var err error

	// Replace tilda with the actual path to home directory
	if len(p) > 0 && p[0] == '~' {
		// Unfortunately user.Current() doesn't play nicely with static build, so
		// we have to get home directory from the environment
		homeEnvName := "HOME"
		if runtime.GOOS == "windows" {
			homeEnvName = "USERPROFILE"
		}
		p = os.Getenv(homeEnvName) + p[1:]
	}

	// Replace ${mos.version} with the actual version
	p = strings.Replace(p, dirTplMosVersion, version, -1)

	// Absolutize path
	p, err = filepath.Abs(p)
	if err != nil {
		return "", errors.Trace(err)
	}

	return p, nil
}
