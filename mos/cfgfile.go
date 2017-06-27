package main

import (
	"flag"
	"io/ioutil"
	"os"
	"path/filepath"
	"runtime"

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
	mosConfig     MosConfig
	mosConfigFile = ""
)

func init() {
	flag.StringVar(&mosConfigFile, "config-file", "~/.mos/config.yml", "Mos tool configuration file")
}

func mosConfigInit() error {
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
		if err := yaml.Unmarshal(data, &mosConfig); err != nil {
			return errors.Trace(err)
		}
	} else if !os.IsNotExist(err) {
		return errors.Trace(err)
	}

	if changed := mosConfig.fixupWithDefaults(); changed {
		data, err := yaml.Marshal(&mosConfig)
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
