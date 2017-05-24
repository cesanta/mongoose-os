package main

import (
	"context"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"net"
	"net/http"
	"os"
	"path/filepath"
	"sort"
	"strconv"
	"strings"
	"sync"
	"time"

	"cesanta.com/mos/build"
	"cesanta.com/mos/dev"
	"github.com/cesanta/errors"
	"github.com/elazarl/go-bindata-assetfs"
	"github.com/golang/glog"
	shellwords "github.com/mattn/go-shellwords"
	"github.com/skratchdot/open-golang/open"
	flag "github.com/spf13/pflag"

	"golang.org/x/net/websocket"
)

const (
	expireTime = 1 * time.Minute
)

type projectType string

const (
	projectTypeApp projectType = "app"
	projectTypeLib projectType = "lib"
)

var (
	httpPort     = 1992
	wsClients    = make(map[*websocket.Conn]int)
	wsClientsMtx = sync.Mutex{}
	wwwRoot      = ""
	startBrowser = true
)

type wsmessage struct {
	Cmd  string `json:"cmd"`
	Data string `json:"data"`
}

// Single entry in the list of apps/libs
type appLibEntry map[string]*build.FWAppManifest

// Params given to /import-app or /import-lib
type importAppLibParams struct {
	URL string `yaml:"url"`
}

type saveAppLibFileParams struct {
	// base64-encoded data
	Data string `yaml:"data"`
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

	flag.BoolVar(&startBrowser, "start-browser", true, "Automatically start browser")
	hiddenFlags = append(hiddenFlags, "start-browser")
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

		if devConn == nil {
			httpReply(w, nil, errors.Errorf("Device is not connected"))
			return
		}

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

		if devConn == nil {
			httpReply(w, nil, errors.Errorf("Device is not connected"))
			return
		}

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
		glog.Errorf("Calling: %+v, args: %+v", method, args)

		timeout, err2 := strconv.ParseInt(r.FormValue("timeout"), 10, 64)
		if err2 != nil {
			timeout = 10
		}
		ctx2, cancel := context.WithTimeout(ctx, time.Duration(timeout)*time.Second)
		defer cancel()

		devConnMtx.Lock()
		defer devConnMtx.Unlock()

		if devConn == nil {
			httpReply(w, nil, errors.Errorf("Device is not connected"))
			return
		}

		result, err := callDeviceService(ctx2, devConn, method, args)
		if method == "Config.Save" {
			// Saving config causes the device to reboot, so we have to wait a bit
			waitForReboot()
		}
		glog.Errorf("Call result: %+v, error: %+v", result, err)
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

	http.HandleFunc("/update", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		result := true

		err := update(ctx, devConn)
		if err != nil {
			err = errors.Trace(err)
			result = false
		}

		httpReply(w, result, err)
	})

	http.HandleFunc("/list-libs", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		result, err := getAppsLibs(libsDir)
		if err != nil {
			err = errors.Trace(err)
		}

		httpReply(w, result, err)
	})

	http.HandleFunc("/list-apps", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		result, err := getAppsLibs(appsDir)
		if err != nil {
			err = errors.Trace(err)
		}

		httpReply(w, result, err)
	})

	http.HandleFunc("/import-lib", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		result, err := importAppLib(libsDir, r)
		if err != nil {
			err = errors.Trace(err)
		}

		httpReply(w, result, err)
	})

	http.HandleFunc("/import-app", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		result, err := importAppLib(appsDir, r)
		if err != nil {
			err = errors.Trace(err)
		}

		httpReply(w, result, err)
	})

	http.HandleFunc("/app/ls", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		result, err := listAppLibFiles(projectTypeApp, r)
		if err != nil {
			err = errors.Trace(err)
		}

		httpReply(w, result, err)
	})

	http.HandleFunc("/lib/ls", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		result, err := listAppLibFiles(projectTypeLib, r)
		if err != nil {
			err = errors.Trace(err)
		}

		httpReply(w, result, err)
	})

	http.HandleFunc("/app/get", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		result, err := getAppLibFile(projectTypeApp, r)
		if err != nil {
			err = errors.Trace(err)
		}

		httpReply(w, result, err)
	})

	http.HandleFunc("/lib/get", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		result, err := getAppLibFile(projectTypeLib, r)
		if err != nil {
			err = errors.Trace(err)
		}

		httpReply(w, result, err)
	})

	http.HandleFunc("/app/set", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		result := true

		err := saveAppLibFile(projectTypeApp, r)
		if err != nil {
			err = errors.Trace(err)
			result = false
		}

		httpReply(w, result, err)
	})

	http.HandleFunc("/lib/set", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		result := true

		err := saveAppLibFile(projectTypeLib, r)
		if err != nil {
			err = errors.Trace(err)
			result = false
		}

		httpReply(w, result, err)
	})

	if wwwRoot != "" {
		http.HandleFunc("/", addExpiresHeader(0, http.FileServer(http.Dir(wwwRoot))))
	} else {
		assetInfo := func(path string) (os.FileInfo, error) {
			return os.Stat(path)
		}
		http.Handle("/", addExpiresHeader(expireTime, http.FileServer(&assetfs.AssetFS{Asset: Asset,
			AssetDir: AssetDir, AssetInfo: assetInfo, Prefix: "web_root"})))
	}
	addr := fmt.Sprintf("127.0.0.1:%d", httpPort)
	url := fmt.Sprintf("http://%s", addr)

	fmt.Printf("To get a list of available commands, start with --help\n")
	fmt.Printf("Starting Web UI. If the browser does not start, navigate to %s\n", url)
	listener, err := net.Listen("tcp", addr)
	if err != nil {
		return errors.Trace(err)
	}
	if startBrowser {
		open.Start(url)
	}
	http.Serve(listener, nil)

	// Unreacahble
	return nil
}

// getAppsLibs takes a path and returns a slice of directory entries. In
// the future each entry may contain some apps- or libs-specific things, but
// for now it's just a name.
//
// If given path does not exist, it's not an error: empty slice is returned.
func getAppsLibs(dirPath string) ([]appLibEntry, error) {
	ret := []appLibEntry{}

	files, err := ioutil.ReadDir(dirPath)
	if err != nil {
		if os.IsNotExist(err) {
			// On non-existing path, just return an empty slice
			return ret, nil
		}
		// On some other error, return an error
		return nil, errors.Trace(err)
	}

	for _, f := range files {
		name := f.Name()
		manifest, _, err := readManifest(filepath.Join(dirPath, name))
		if err != nil {
			return nil, errors.Trace(err)
		}
		entry := map[string]*build.FWAppManifest{
			name: manifest,
		}
		ret = append(ret, entry)
	}

	return ret, nil
}

func importAppLib(dirPath string, r *http.Request) (string, error) {

	par := importAppLibParams{}

	decoder := json.NewDecoder(r.Body)
	if err := decoder.Decode(&par); err != nil {
		return "", errors.Trace(err)
	}

	if par.URL == "" {
		return "", errors.Errorf("url is required")
	}

	swmod := build.SWModule{
		Origin: par.URL,
	}

	_, err := swmod.PrepareLocalDir(dirPath, os.Stdout, true)
	if err != nil {
		return "", errors.Trace(err)
	}

	name, err := swmod.GetName()
	if err != nil {
		return "", errors.Trace(err)
	}

	return name, nil
}

func listAppLibFiles(pt projectType, r *http.Request) ([]string, error) {
	ret := []string{}

	rootDir, err := getRootByProjectType(pt)
	if err != nil {
		return nil, errors.Trace(err)
	}

	pname := r.FormValue(string(pt))
	if pname == "" {
		return nil, errors.Errorf("%s is required", pt)
	}

	projectPath := filepath.Join(rootDir, pname)

	if _, err := os.Stat(projectPath); err != nil {
		return nil, errors.Trace(err)
	}

	err = filepath.Walk(projectPath, func(path string, fi os.FileInfo, _ error) error {

		// Ignore the root path
		if path == projectPath {
			return nil
		}

		// Strip the path to dir
		path = path[len(projectPath)+1:]

		// Ignore ".git"
		if strings.HasPrefix(path, ".git") {
			return nil
		}

		ret = append(ret, path)

		return nil
	})
	if err != nil {
		return nil, errors.Trace(err)
	}

	return ret, nil
}

func getAppLibFile(pt projectType, r *http.Request) (string, error) {
	rootDir, err := getRootByProjectType(pt)
	if err != nil {
		return "", errors.Trace(err)
	}

	pname := r.FormValue(string(pt))
	if pname == "" {
		return "", errors.Errorf("%s is required", pt)
	}

	filename := r.FormValue("filename")
	if filename == "" {
		return "", errors.Errorf("filename is required")
	}

	data, err := ioutil.ReadFile(filepath.Join(rootDir, pname, filename))
	if err != nil {
		return "", errors.Trace(err)
	}

	return base64.StdEncoding.EncodeToString(data), nil
}

func saveAppLibFile(pt projectType, r *http.Request) error {
	rootDir, err := getRootByProjectType(pt)
	if err != nil {
		return errors.Trace(err)
	}

	// Read body and get file data from it
	//
	// NOTE: we have to read data from r.Body before calling r.FormValue, because
	// r.FormValue reads the body on its own.
	par := saveAppLibFileParams{}

	decoder := json.NewDecoder(r.Body)
	if err := decoder.Decode(&par); err != nil {
		return errors.Annotatef(err, "decoding JSON")
	}

	data, err := base64.StdEncoding.DecodeString(par.Data)
	if err != nil {
		return errors.Annotatef(err, "decoding base64")
	}

	// get pname and filename from the query string
	// (we may have put it into saveAppLibFileParams as well, but just to make
	// it symmetric with the getAppLibFile, they are in they query string)
	pname := r.FormValue(string(pt))
	if pname == "" {
		return errors.Errorf("%s is required", pt)
	}

	filename := r.FormValue("filename")
	if filename == "" {
		return errors.Errorf("filename is required")
	}

	err = ioutil.WriteFile(filepath.Join(rootDir, pname, filename), data, 0755)
	if err != nil {
		return errors.Trace(err)
	}

	return nil
}

func getRootByProjectType(pt projectType) (string, error) {
	switch pt {
	case projectTypeApp:
		return appsDir, nil
	case projectTypeLib:
		return libsDir, nil
	}
	return "", errors.Errorf("invalid project type: %q", pt)
}

func addExpiresHeader(d time.Duration, handler http.Handler) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		const layout = "Mon, 02 Jan 2006 15:04:05 GMT"
		w.Header().Set("Expires", time.Now().UTC().Add(d).Format(layout))
		handler.ServeHTTP(w, r)
	}
}
