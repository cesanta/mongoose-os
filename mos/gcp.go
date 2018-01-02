package main

import (
	"bytes"
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/rand"
	"crypto/x509"
	"encoding/pem"
	"flag"
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"path/filepath"
	"strings"

	"github.com/cesanta/errors"

	"golang.org/x/net/context"

	"cesanta.com/mos/dev"
)

var (
	gcpProject  = ""
	gcpRegion   = ""
	gcpRegistry = ""
)

func init() {
	flag.StringVar(&gcpProject, "gcp-project", "", "Google IoT project ID")
	flag.StringVar(&gcpRegion, "gcp-region", "", "Google IoT region")
	flag.StringVar(&gcpRegistry, "gcp-registry", "", "Google IoT device registry")
	hiddenFlags = append(hiddenFlags, "gcp-project", "gcp-region", "gcp-registry")
}

func FileExists(path string) bool {
	_, err := os.Stat(path)
	return err == nil
}

func genPem(name string, ktype string, data []byte) error {
	buf := bytes.NewBuffer(nil)
	pem.Encode(buf, &pem.Block{Type: ktype, Bytes: data})
	err := ioutil.WriteFile(name, buf.Bytes(), 0400)
	if err != nil {
		return errors.Annotatef(err, "failed to write private key to %s")
	}
	reportf("Wrote %s", name)
	return nil
}

func gcpIoTSetup(ctx context.Context, devConn *dev.DevConn) error {
	if gcpProject == "" || gcpRegion == "" || gcpRegistry == "" {
		return errors.Errorf("Please set --gcp-project, --gcp-region, --gcp-registry")
	}

	reportf("Connecting to the device...")
	devInfo, err := devConn.GetInfo(ctx)
	if err != nil {
		return errors.Annotatef(err, "failed to connect to device")
	}
	devArch, devMAC := *devInfo.Arch, *devInfo.Mac
	reportf("  %s %s running %s", devArch, devMAC, *devInfo.App)

	devConf, err := devConn.GetConfig(ctx)
	if err != nil {
		return errors.Annotatef(err, "failed to connect to get device config")
	}
	devId, err := devConf.Get("device.id")
	if err != nil {
		return errors.Annotatef(err, "failed to get device ID")
	}
	mqttConf, err := devConf.Get("mqtt")
	if err != nil {
		return errors.Annotatef(err, "failed to get device MQTT config. Make sure firmware supports MQTT")
	}
	gcpConf, err := devConf.Get("gcp")
	if err != nil {
		return errors.Annotatef(err, "failed to get GCP config. Make sure the firmware supports GCP")
	}

	privName := "gcp-" + devId + ".priv.pem"
	pubName := "gcp-" + devId + ".pub.pem"
	if FileExists(privName) && FileExists(pubName) {
		reportf("%s %s exist, skipping generation", privName, pubName)
	} else {
		key, err := ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
		if err != nil {
			return errors.Annotatef(err, "failed to generate EC key")
		}
		privData, _ := x509.MarshalECPrivateKey(key)
		err = genPem(privName, "EC PRIVATE KEY", privData)
		if err != nil {
			return errors.Annotatef(err, "failed to write %s", privName)
		}
		pubData, _ := x509.MarshalPKIXPublicKey(key.Public())
		err = genPem(pubName, "PUBLIC KEY", pubData)
		if err != nil {
			return errors.Annotatef(err, "failed to write %s", pubName)
		}
	}

	reportf("(Re)creating the cloud device...")
	out, err := exec.Command("gcloud", "beta", "iot", "devices", "delete", devId,
		"--project", gcpProject, "--region", gcpRegion, "--registry", gcpRegistry).Output()
	if err != nil {
		reportf("gcloud device deletion: %s", string(out))
	}

	out, err = exec.Command("gcloud", "beta", "iot", "devices", "create", devId,
		"--project", gcpProject, "--region", gcpRegion, "--registry", gcpRegistry,
		"--public-key", fmt.Sprintf("path=%s,type=es256", pubName)).Output()
	if err != nil {
		return errors.Annotatef(err, "gcloud device create: %s", string(out))
	}

	reportf("Uploading key...")
	err = fsPutFile(ctx, devConn, privName, filepath.Base(privName))
	if err != nil {
		return errors.Annotatef(err, "failed to upload %s", privName)
	}

	// ca.pem has both roots in it, so, for platforms other than CC32XX, we can just use that.
	// CC32XX do not support cert bundles and will always require specific CA cert.
	// http://e2e.ti.com/support/wireless_connectivity/simplelink_wifi_cc31xx_cc32xx/f/968/t/634431
	caCertFile := "ca.pem"
	uploadCACert := false
	if strings.HasPrefix(strings.ToLower(*devInfo.Arch), "cc32") {
		caCertFile = "data/ca-globalsign.crt.pem"
		uploadCACert = true
	}

	if uploadCACert {
		caCertData := MustAsset(caCertFile)
		reportf("Uploading CA certificate...")
		err = fsPutData(ctx, devConn, bytes.NewBuffer(caCertData), filepath.Base(caCertFile))
		if err != nil {
			return errors.Annotatef(err, "failed to upload %s", filepath.Base(caCertFile))
		}
	}

	// GCP does not support bi-di MQTT comms, RPC won't work.
	devConf.Set("rpc.mqtt.enable", "false")
	devConf.Set("sntp.enable", "true")
	devConf.Set("mqtt.enable", "true")
	devConf.Set("mqtt.server", "mqtt.googleapis.com:8883")
	devConf.Set("mqtt.ssl_ca_cert", filepath.Base(caCertFile))
	devConf.Set("gcp.enable", "true")
	devConf.Set("gcp.project", gcpProject)
	devConf.Set("gcp.region", gcpRegion)
	devConf.Set("gcp.registry", gcpRegistry)
	devConf.Set("gcp.device", devId)
	devConf.Set("gcp.key", privName)

	mqttConf, _ = devConf.Get("mqtt")
	gcpConf, _ = devConf.Get("gcp")
	reportf("New config:\ngcp %+v\nmqtt %+v", gcpConf, mqttConf)

	err = configSetAndSave(ctx, devConn, devConf)
	if err != nil {
		return errors.Annotatef(err, "failed to apply new configuration")
	}

	return nil
}
