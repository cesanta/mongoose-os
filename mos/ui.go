package main

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"net"
	"net/http"
	"os"
	"path/filepath"
	"sort"
	"strconv"
	"sync"
	"time"

	"cesanta.com/mos/dev"
	"github.com/cesanta/errors"
	"github.com/elazarl/go-bindata-assetfs"
	"github.com/golang/glog"
	shellwords "github.com/mattn/go-shellwords"
	"github.com/skratchdot/open-golang/open"
	flag "github.com/spf13/pflag"

	"golang.org/x/net/websocket"
)

var (
	httpPort     = 1992
	wsClients    = make(map[*websocket.Conn]int)
	wsClientsMtx = sync.Mutex{}
	wwwRoot      = ""
)

type wsmessage struct {
	Cmd  string `json:"cmd"`
	Data string `json:"data"`
}

func wsSend(ws *websocket.Conn, m wsmessage) {
	t, _ := json.Marshal(m)
	websocket.Message.Send(ws, string(t))
}

func wsBroadcast(m wsmessage) {
	wsClientsMtx.Lock()
	defer wsClientsMtx.Unlock()

	for ws := range wsClients {
		wsSend(ws, m)
	}
}

type errmessage struct {
	Error string `json:"error"`
}

func wsHandler(ws *websocket.Conn) {
	defer func() {
		wsClientsMtx.Lock()
		defer wsClientsMtx.Unlock()
		delete(wsClients, ws)
		ws.Close()
	}()
	wsClientsMtx.Lock()
	wsClients[ws] = 1
	wsClientsMtx.Unlock()

	for {
		var text string
		err := websocket.Message.Receive(ws, &text)
		if err != nil {
			glog.Infof("Websocket recv error: %v, closing connection", err)
			return
		}
	}
}

func reportConsoleLogs() {
	for {
		data := <-consoleMsgs
		wsBroadcast(wsmessage{"uart", string(data)})
	}
}

func httpReply(w http.ResponseWriter, result interface{}, err error) {
	var msg []byte
	if err != nil {
		msg, _ = json.Marshal(errmessage{err.Error()})
	} else {
		s, ok := result.(string)
		if ok && isJSON(s) {
			msg = []byte(fmt.Sprintf(`{"result": %s}`, s))
		} else {
			r := map[string]interface{}{
				"result": result,
			}
			msg, err = json.Marshal(r)
		}
	}

	if err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		io.WriteString(w, string(msg))
	} else {
		w.WriteHeader(http.StatusOK)
		io.WriteString(w, string(msg))
	}
}

func init() {
	flag.StringVar(&wwwRoot, "web-root", "", "UI Web root to use instead of built-in")
	hiddenFlags = append(hiddenFlags, "web-root")
}

func reconnectToDevice(ctx context.Context) (*dev.DevConn, error) {
	return createDevConnWithJunkHandler(ctx, consoleJunkHandler)
}

func startUI(ctx context.Context, devConn *dev.DevConn) error {
	var devConnMtx sync.Mutex

	glog.CopyStandardLogTo("INFO")
	go reportConsoleLogs()
	http.Handle("/ws", websocket.Handler(wsHandler))

	r, w, _ := os.Pipe()
	os.Stdout = w
	os.Stderr = w
	go func() {
		for {
			data := make([]byte, 512)
			n, err := r.Read(data)
			if err != nil {
				break
			}
			wsBroadcast(wsmessage{"stderr", string(data[:n])})
		}
	}()

	http.HandleFunc("/flash", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		r.ParseForm()
		*firmware = r.FormValue("firmware")

		// TODO(lsm): the following snippet is similar to the one in "/terminal"
		// handler, refactor to reduce copypasta.
		devConnMtx.Lock()
		defer devConnMtx.Unlock()
		if devConn != nil {
			devConn.Disconnect(ctx)
			devConn = nil
		}
		defer func() {
			time.Sleep(time.Second)
			devConn, _ = reconnectToDevice(ctx)
		}()
		time.Sleep(time.Second) // Close really really
		ctx2, cancel := context.WithTimeout(ctx, 20*time.Second)
		defer cancel()

		os.Args = []string{
			"flash", "--port", *portFlag, "--firmware", *firmware,
			"--v", "4", "--logtostderr",
		}
		err := flash(ctx2, nil)
		httpReply(w, true, err)
	})

	http.HandleFunc("/wifi", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		args := []string{
			"wifi.ap.enable=false",
			"wifi.sta.enable=true",
			fmt.Sprintf("wifi.sta.ssid=%s", r.FormValue("ssid")),
			fmt.Sprintf("wifi.sta.pass=%s", r.FormValue("pass")),
		}
		// We need 15-seconds timeout in order for the bad password to be detected
		// properly. Previously we had 10 seconds, and the context was timing out
		// before the bad password result was returned.
		ctx2, cancel := context.WithTimeout(ctx, 15*time.Second)
		defer cancel()

		devConnMtx.Lock()
		defer devConnMtx.Unlock()

		err := internalConfigSet(ctx2, devConn, args)
		result := "false"
		if err == nil {
			for {
				time.Sleep(time.Millisecond * 500)
				res2, err := devConn.GetInfo(ctx2)
				if err != nil {
					httpReply(w, result, err)
					return
				}
				wifiStatus := *res2.Wifi.Status
				if wifiStatus == "got ip" {
					result = fmt.Sprintf(`"%s"`, *res2.Wifi.Sta_ip)
					break
				} else if wifiStatus == "connecting" || wifiStatus == "connected" || wifiStatus == "" || wifiStatus == "associated" {
					// Still connecting, wait
				} else {
					err = errors.Errorf("%s", wifiStatus)
					break
				}
			}
		}
		httpReply(w, result, err)
	})

	http.HandleFunc("/policies", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		awsRegion = r.FormValue("region")
		arr, err := getAWSIoTPolicyNames()
		if err == nil {
			sort.Strings(arr)
			// Include the default policy, even if not present - it will be created.
			if sort.SearchStrings(arr, awsIoTPolicyMOS) >= len(arr) {
				arr = append(arr, awsIoTPolicyMOS)
				sort.Strings(arr)
			}
		}
		httpReply(w, arr, err)
	})

	http.HandleFunc("/regions", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		httpReply(w, getAWSRegions(), nil)
	})

	http.HandleFunc("/connect", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		portArg := r.FormValue("port")
		reconnect := r.FormValue("reconnect")

		devConnMtx.Lock()
		defer devConnMtx.Unlock()

		// If we're already connected to the given port, and the caller didn't
		// explicitly ask to reconnect in any case, don't do anything and just
		// report success
		if portArg == *portFlag && devConn != nil && reconnect == "" {
			httpReply(w, true, nil)
			return
		}

		if devConn != nil {
			devConn.Disconnect(ctx)
			devConn = nil
		}
		if portArg != "" {
			*portFlag = portArg
		}
		var err error
		devConn, err = reconnectToDevice(ctx)
		httpReply(w, true, err)
	})

	http.HandleFunc("/version", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		httpReply(w, BuildId, nil)
	})

	http.HandleFunc("/get", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		ctx2, cancel := context.WithTimeout(ctx, 15*time.Second)
		defer cancel()

		devConnMtx.Lock()
		defer devConnMtx.Unlock()

		text, err := getFile(ctx2, devConn, r.FormValue("name"))
		if err == nil {
			text2, err2 := json.Marshal(text)
			if err2 == nil {
				text = string(text2)
			} else {
				err = err2
			}
		}
		httpReply(w, text, err)
	})

	http.HandleFunc("/aws-iot-setup", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		awsIoTPolicy = r.FormValue("policy")
		awsRegion = r.FormValue("region")

		ctx2, cancel := context.WithTimeout(ctx, 30*time.Second)
		defer cancel()

		devConnMtx.Lock()
		defer devConnMtx.Unlock()

		err := awsIoTSetup(ctx2, devConn)
		httpReply(w, true, err)
	})

	http.HandleFunc("/aws-store-creds", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		_, err := storeCreds(r.FormValue("key"), r.FormValue("secret"))
		httpReply(w, true, err)
	})

	http.HandleFunc("/check-aws-credentials", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		err := checkAwsCredentials()
		httpReply(w, err == nil, nil)
	})

	http.HandleFunc("/getports", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		devConnMtx.Lock()
		defer devConnMtx.Unlock()

		type GetPortsResult struct {
			IsConnected bool
			CurrentPort string
			Ports       []string
		}
		reply := GetPortsResult{false, "", enumerateSerialPorts()}
		if devConn != nil {
			reply.CurrentPort = devConn.ConnectAddr
			reply.IsConnected = devConn.IsConnected()
		}

		httpReply(w, reply, nil)
	})

	http.HandleFunc("/list_aws_things", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		reply, err := getAWSIoTThings()
		httpReply(w, reply, err)
	})

	http.HandleFunc("/infolog", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "text/plain")
		glog.Flush()
		pattern := fmt.Sprintf("%s/mos*INFO*.%d", os.TempDir(), os.Getpid())
		paths, err := filepath.Glob(pattern)
		if err == nil && len(paths) > 0 {
			http.ServeFile(w, r, paths[0])
		}
	})

	http.HandleFunc("/call", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		method := r.FormValue("method")

		if method == "" {
			httpReply(w, nil, errors.Errorf("Expecting method"))
			return
		}
		args := r.FormValue("args")

		fmt.Println("Calling", method, args)

		timeout, err2 := strconv.ParseInt(r.FormValue("timeout"), 10, 64)
		if err2 != nil {
			timeout = 10
		}
		ctx2, cancel := context.WithTimeout(ctx, time.Duration(timeout)*time.Second)
		defer cancel()

		devConnMtx.Lock()
		defer devConnMtx.Unlock()

		result, err := callDeviceService(ctx2, devConn, method, args)
		if method == "Config.Save" {
			// Saving config causes the device to reboot, so we have to wait a bit
			waitForReboot()
		}
		httpReply(w, result, err)
	})

	http.HandleFunc("/terminal", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		// Get the command line value, modify os.Args and re-parse flags
		str := r.FormValue("cmd")
		args, err := shellwords.Parse(str)
		if err != nil {
			httpReply(w, true, err)
			return
		}
		if len(args) > 0 && args[0] != "mos" {
			args = append([]string{"mos"}, args...)
		}
		os.Args = args
		if len(os.Args) > 1 && os.Args[1] == "-X" {
			os.Args = append(os.Args[:1], os.Args[2:]...)
			extendedMode = true
			commands = append(commands, extendedCommands...)
		}
		flag.CommandLine.Init("mos", flag.ContinueOnError)
		flag.Parse()

		// Some commands want special access to the serial port,
		// therefore close device connection here and schedule the reconnection
		// after this function exits.
		cmd := getCommand(flag.Arg(0))

		if cmd != nil && !cmd.needDevConn {
			devConnMtx.Lock()
			defer devConnMtx.Unlock()

			// On MacOS and Windows, sleep for 1 second after we close serial
			// port. Otherwise, open call fails for some reason we have
			// no idea about. Thus those time.Sleep() calls below.
			if devConn != nil {
				devConn.Disconnect(ctx)
				devConn = nil
			}
			defer func() {
				time.Sleep(time.Second)
				devConn, _ = reconnectToDevice(ctx)
			}()
			time.Sleep(time.Second)
		}

		ctx2, cancel := context.WithTimeout(ctx, 15*time.Second)
		defer cancel()
		err = run(cmd, ctx2, devConn)
		httpReply(w, true, err)
	})

	if wwwRoot != "" {
		http.Handle("/", http.FileServer(http.Dir(wwwRoot)))
	} else {
		assetInfo := func(path string) (os.FileInfo, error) {
			return os.Stat(path)
		}
		http.Handle("/", http.FileServer(&assetfs.AssetFS{Asset: Asset,
			AssetDir: AssetDir, AssetInfo: assetInfo, Prefix: "web_root"}))
	}
	addr := fmt.Sprintf("127.0.0.1:%d", httpPort)
	url := fmt.Sprintf("http://%s", addr)

	fmt.Printf("To get a list of available commands, start with --help\n")
	fmt.Printf("Starting Web UI. If the browser does not start, navigate to %s\n", url)
	listener, err := net.Listen("tcp", addr)
	if err != nil {
		return errors.Trace(err)
	}
	open.Start(url)
	http.Serve(listener, nil)

	// Unreacahble
	return nil
}
