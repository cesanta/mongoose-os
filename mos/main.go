//go:generate ../common/tools/fw_meta.py gen_build_info --go_output=./version.go  --json_output=./version.json
//go:generate go-bindata -pkg main -nocompress -modtime 1 -mode 420 data/
//go:generate go-bindata-assetfs -pkg main -nocompress -modtime 1 -mode 420 data/ web_root/...

package main

import (
	"context"
	cRand "crypto/rand"
	"fmt"
	"math/big"
	mRand "math/rand"
	"os"
	"path/filepath"
	"time"

	"cesanta.com/common/go/pflagenv"
	"cesanta.com/mos/build"
	"cesanta.com/mos/dev"
	"github.com/cesanta/errors"
	"github.com/golang/glog"
	flag "github.com/spf13/pflag"
)

const (
	envPrefix = "MOS_"
)

// This section contains all "simple" flags, i.e. flags that our great leader loves and cares about.
// Each command can also register more flags but they should be hidden by default so the tool doesn't seem complex.
// Full help can be shown with --helpfull anyway.
var (
	arch       = flag.String("arch", "", "Hardware architecture. Possible values: esp8266, cc3200")
	user       = flag.String("user", "", "Cloud username")
	pass       = flag.String("pass", "", "Cloud password or token")
	server     = flag.String("server", "http://mongoose.cloud", "Cloud server")
	local      = flag.Bool("local", false, "Local build.")
	mosRepo    = flag.String("repo", "", "Path to the mongoose-os repository; if omitted, the mongoose-os repository will be cloned as ./mongoose-os")
	deviceID   = flag.String("device-id", "", "Device ID")
	devicePass = flag.String("device-pass", "", "Device pass/key")
	firmware   = flag.String("firmware", filepath.Join(buildDir, build.FirmwareFileName), "Firmware .zip file location (file of HTTP URL)")
	portFlag   = flag.String("port", "auto", "Serial port where the device is connected. "+
		"If set to 'auto', ports on the system will be enumerated and the first will be used.")
	timeout   = flag.Duration("timeout", 10*time.Second, "Timeout for the device connection")
	reconnect = flag.Bool("reconnect", false, "Enable reconnection")
	force     = flag.Bool("force", false, "Use the force")
	verbose   = flag.Bool("verbose", false, "Verbose output")

	versionFlag = flag.Bool("version", false, "Print version and exit")
	helpFull    = flag.Bool("helpfull", false, "Show full help, including advanced flags")

	extendedMode = false
	isUI         = false
)

var (
	// put all commands here
	commands = []command{
		{"ui", startUI, `Start GUI`, nil, nil, false},
		{"init", initFW, `Initialise firmware directory structure in the current directory`, nil, []string{"arch", "force"}, false},
		{"build", doBuild, `Build a firmware from the sources located in the current directory`, nil, []string{"arch", "local", "repo", "clean", "server"}, false},
		{"flash", flash, `Flash firmware to the device`, nil, []string{"port", "firmware"}, false},
		{"console", console, `Simple serial port console`, nil, []string{"port"}, false}, //TODO: needDevConn
		{"ls", fsLs, `List files at the local device's filesystem`, nil, []string{"port"}, true},
		{"get", fsGet, `Read file from the local device's filesystem and print to stdout`, nil, []string{"port"}, true},
		{"put", fsPut, `Put file from the host machine to the local device's filesystem`, nil, []string{"port"}, true},
		{"rm", fsRm, `Delete a file from the device's filesystem`, nil, []string{"port"}, true},
		{"config-get", configGet, `Get config value from the locally attached device`, nil, []string{"port"}, true},
		{"config-set", configSet, `Set config value at the locally attached device`, nil, []string{"port"}, true},
		{"call", call, `Perform a device API call. "mos call RPC.List" shows available methods`, nil, []string{"port"}, true},
		{"aws-iot-setup", awsIoTSetup, `Provision the device for AWS IoT cloud`, nil, []string{"atca-slot", "aws-region", "port", "use-atca"}, true},
		{"wifi", wifi, `Setup WiFi - shortcut to config-set wifi...`, nil, nil, true},
	}
	// These commands are only available when invoked with -X
	extendedCommands = []command{
		{"atca-get-config", atcaGetConfig, `Get ATCA chip config`, nil, []string{"format", "port"}, true},
		{"atca-set-config", atcaSetConfig, `Set ATCA chip config`, nil, []string{"format", "dry-run", "port"}, true},
		{"atca-lock-zone", atcaLockZone, `Lock config or data zone`, nil, []string{"dry-run", "port"}, true},
		{"atca-set-key", atcaSetKey, `Set key in a given slot`, nil, []string{"dry-run", "port", "write-key"}, true},
		{"atca-gen-key", atcaGenKey, `Generate a random key in a given slot`, nil, []string{"dry-run", "port"}, true},
		{"atca-get-pub-key", atcaGetPubKey, `Retrieve public ECC key from a given slot`, nil, []string{"port"}, true},
		{"esp32-efuse-get", esp32EFuseGet, `Get ESP32 eFuses`, nil, nil, false},
		{"esp32-efuse-set", esp32EFuseSet, `Set ESP32 eFuses`, nil, nil, false},
		{"esp32-encrypt-image", esp32EncryptImage, `Encrypt a ESP32 firmware image`, []string{"esp32-encryption-key-file", "esp32-flash-address"}, nil, false},
	}
)

type command struct {
	name        string
	handler     handler
	short       string
	required    []string
	optional    []string
	needDevConn bool
}

type handler func(ctx context.Context, devConn *dev.DevConn) error

// channel of "junk" messages, which go to the console
var consoleMsgs chan []byte

func unimplemented() error {
	fmt.Println("TODO")
	return nil
}

func run(c *command, ctx context.Context, devConn *dev.DevConn) error {
	if c != nil {
		// check required flags
		if err := checkFlags(c.required); err != nil {
			return errors.Trace(err)
		}

		// run the handler
		if err := c.handler(ctx, devConn); err != nil {
			return errors.Trace(err)
		}
		return nil
	}

	// not found
	usage()
	return nil
}

// getCommand returns a pointer to the command which needs to run, or nil if
// there's no such command
func getCommand() *command {
	if isUI {
		return &commands[0]
	}
	for _, c := range commands {
		if c.name == flag.Arg(0) {
			return &c
		}
	}
	return nil
}

func consoleJunkHandler(data []byte) {
	removeNonText(data)
	select {
	case consoleMsgs <- data:
	default:
		// Junk overflow; do nothing
	}
}

func main() {
	seed1 := time.Now().UnixNano()
	seed2, _ := cRand.Int(cRand.Reader, big.NewInt(4000000000))
	mRand.Seed(seed1 ^ seed2.Int64())

	// If no arguments are given, show help and start UI
	isUI = len(os.Args) == 1

	consoleMsgs = make(chan []byte, 10)

	// -X, if given, must be the first arg.
	if len(os.Args) > 1 && os.Args[1] == "-X" {
		os.Args = append(os.Args[:1], os.Args[2:]...)
		extendedMode = true
		commands = append(commands, extendedCommands...)
	}
	initFlags()
	flag.Parse()
	pflagenv.Parse(envPrefix)
	if flag.Arg(0) == "ui" {
		isUI = true
	}

	if isUI {
		*reconnect = true
	}

	if *helpFull {
		unhideFlags()
		usage()
	} else if *versionFlag {
		fmt.Printf(
			"%s\nVersion: %s\nBuild ID: %s\n",
			"The Mongoose OS command line tool", Version, BuildId,
		)
		return
	}

	ctx := context.Background()
	var devConn *dev.DevConn

	cmd := getCommand()
	if cmd != nil && cmd.needDevConn {
		var err error
		devConn, err = createDevConnWithJunkHandler(ctx, consoleJunkHandler)
		if err != nil {
			fmt.Println(errors.Trace(err))
			return
		}
	}

	if err := run(cmd, ctx, devConn); err != nil {
		glog.Infof("Error: %+v", err)
		fmt.Fprintf(os.Stderr, "Error: %s\n", err)
		os.Exit(1)
	}
}
