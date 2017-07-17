// +build !noflash

package main

import (
	"golang.org/x/net/context"
	"io/ioutil"

	"cesanta.com/mos/dev"
	"cesanta.com/mos/flash/esp32"
	"github.com/cesanta/errors"
	flag "github.com/spf13/pflag"
)

var (
	esp32FlashAddress uint32
)

func init() {
	flag.Uint32Var(&esp32FlashAddress, "esp32-flash-address", 0, "")
}

func esp32EncryptImage(ctx context.Context, devConn *dev.DevConn) error {
	if len(flag.Args()) != 3 {
		return errors.Errorf("input and output images are required")
	}
	inFile := flag.Args()[1]
	outFile := flag.Args()[2]
	inData, err := ioutil.ReadFile(inFile)
	if err != nil {
		return errors.Annotatef(err, "failed to read input file")
	}
	key, err := ioutil.ReadFile(espFlashOpts.ESP32EncryptionKeyFile)
	if err != nil {
		return errors.Annotatef(err, "failed to read encryption key")
	}
	outData, err := esp32.ESP32EncryptImageData(
		inData, key, esp32FlashAddress, espFlashOpts.ESP32FlashCryptConf)
	if err != nil {
		return errors.Annotatef(err, "failed to encrypt data")
	}
	err = ioutil.WriteFile(outFile, outData, 0644)
	if err != nil {
		return errors.Annotatef(err, "failed to write output")
	}
	return nil
}
