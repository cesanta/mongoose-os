package cfgfile

import (
	"flag"
	"io/ioutil"
	"os"
	"path/filepath"
	"runtime"

	moscommon "cesanta.com/mos/common"
	"github.com/cesanta/errors"
	yaml "gopkg.in/yaml.v2"
)

type MosConfig struct {
	MosVersion string `yaml:"mos_version"`
}

const (
	defaultMosVersion = "master" // TODO(dfrank): "release"
)

var (
	MosConfigCurrent MosConfig
	mosConfigFile    = ""
)

func init() {
	flag.StringVar(&mosConfigFile, "config-file", "~/.mos/config.yml", "Mos tool configuration file")
}

func MosConfigInit() error {
	homeEnvName := "HOME"
	if runtime.GOOS == "windows" {
		homeEnvName = "USERPROFILE"
	}
	homeDir := os.Getenv(homeEnvName)

	// Replace tilda with the actual path to home directory
	if len(mosConfigFile) > 0 && mosConfigFile[0] == '~' {
		mosConfigFile = homeDir + mosConfigFile[1:]
	}

	data, err := ioutil.ReadFile(mosConfigFile)
	if err == nil {
		if err := yaml.Unmarshal(data, &MosConfigCurrent); err != nil {
			return errors.Trace(err)
		}
	} else if !os.IsNotExist(err) {
		return errors.Trace(err)
	}

	if changed := MosConfigCurrent.fixupWithDefaults(); changed {
		data, err := yaml.Marshal(&MosConfigCurrent)
		if err != nil {
			return errors.Trace(err)
		}

		configFileDir := filepath.Dir(mosConfigFile)
		if err := os.MkdirAll(configFileDir, 0777); err != nil {
			return errors.Annotatef(err, "failed to create dir %q for config file", configFileDir)
		}

		if err := ioutil.WriteFile(mosConfigFile, data, 0666); err != nil {
			return errors.Annotatef(err, "failed to write config file")
		}
	}

	return nil
}

func (c *MosConfig) fixupWithDefaults() bool {
	ret := false

	if c.MosVersion == "" {
		c.MosVersion = defaultMosVersion
		ret = true
	}

	return ret
}

// GetMosVersionSuffix returns an appropriate suffix depending on current
// mos_version in the mos config: for "latest" or "master" or "" it returns an
// empty string; for any other version it returns the version prepended with a
// dash, e.g. "-1.5".
func GetMosVersionSuffix() string {
	return moscommon.GetVersionSuffix(MosConfigCurrent.MosVersion)
}
