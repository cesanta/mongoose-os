package codec

import (
	"encoding/json"
	"fmt"
	"golang.org/x/net/context"
	"io"
	"net/http"
	"strconv"
	"strings"
	"sync"

	"cesanta.com/common/go/mgrpc/frame"
	"github.com/cesanta/errors"
	"github.com/golang/glog"
)

type inboundHttpCodec struct {
	sync.Mutex
	req           *http.Request
	rw            http.ResponseWriter
	cloudHost     string
	in            *frame.Frame
	out           *frame.Frame
	closeNotifier chan struct{}
	closeOnce     sync.Once
	isSingleShot  bool
}

func InboundHTTP(rw http.ResponseWriter, req *http.Request, cloudHost string) Codec {
	f := &frame.Frame{}
	c := &inboundHttpCodec{
		req:           req,
		rw:            rw,
		cloudHost:     cloudHost,
		in:            f,
		closeNotifier: make(chan struct{}),
	}
	if req.URL.Path != "/" {
		// Path is a method, destination is derived from Host or left empty/implicit.
		// http://src:key@my.dst.api.mongoose-os.com/Method?id=123&timeout=456
		// Args, if any, are passed as a JSON object in the body.
		f.Version = 2
		f.Src, f.Key, _ = req.BasicAuth()
		if cloudHost != "" && strings.HasSuffix(req.Host, cloudHost) {
			f.Dst = strings.TrimSuffix(strings.TrimSuffix(req.Host, cloudHost), ".")
		}
		f.Method = req.URL.Path
		if req.ContentLength > 0 {
			if err := json.NewDecoder(req.Body).Decode(&f.Args); err != nil {
				http.Error(rw, fmt.Sprintf("Invalid args: %s", err), http.StatusBadRequest)
				return nil
			}
		}
		// We only expect response to this request.
		c.isSingleShot = true
		params := req.URL.Query()
		idStr := params.Get("id")
		if idStr != "" {
			id, err := strconv.ParseInt(idStr, 10, 64)
			if err != nil {
				http.Error(rw, fmt.Sprintf("Invalid id value %q: %s", idStr, err), http.StatusBadRequest)
				return nil
			}
			f.ID = id
		}
		deadlineStr := params.Get("deadline")
		if deadlineStr != "" {
			d, err := strconv.ParseInt(deadlineStr, 10, 64)
			if err != nil {
				http.Error(rw, fmt.Sprintf("Invalid deadline value %q: %s", deadlineStr, err), http.StatusBadRequest)
				return nil
			}
			f.Deadline = d
		}
		timeoutStr := params.Get("timeout")
		if timeoutStr != "" {
			t, err := strconv.ParseInt(timeoutStr, 10, 64)
			if err != nil {
				http.Error(rw, fmt.Sprintf("Invalid timeout value %q: %s", timeoutStr, err), http.StatusBadRequest)
				return nil
			}
			f.Timeout = t
		}
	} else {
		if err := json.NewDecoder(req.Body).Decode(f); err != nil {
			glog.Infof("Invalid json (%s): %+v", err, req)
			http.Error(rw, fmt.Sprintf("Invalid frame: %s", err), http.StatusBadRequest)
			return nil
		}
	}

	// httptest.ResponseRecorder used in tests does not implement CloseNotifier.
	var httpCloseNotifier <-chan bool
	if cn, ok := c.rw.(http.CloseNotifier); ok {
		httpCloseNotifier = cn.CloseNotify()
	}

	go c.monitorHTTPConnection(httpCloseNotifier)
	return c
}

func (c *inboundHttpCodec) monitorHTTPConnection(httpCloseNotifier <-chan bool) {
	select {
	case <-c.CloseNotify():
	case <-httpCloseNotifier:
		c.Lock()
		c.rw = nil
		c.Unlock()
		c.Close()
	}
}

func (c *inboundHttpCodec) String() string {
	return fmt.Sprintf("[inboundHttpCodec from %s]", c.req.RemoteAddr)
}

func (c *inboundHttpCodec) Recv(ctx context.Context) (*frame.Frame, error) {
	c.Lock()
	defer c.Unlock()
	if c.in == nil {
		return nil, errors.Trace(io.EOF)
	}
	f := c.in
	c.in = nil
	return f, nil
}

func (c *inboundHttpCodec) Send(ctx context.Context, f *frame.Frame) error {
	glog.V(2).Infof("%s: Sending %+v", c, f)
	select {
	case <-c.closeNotifier:
		return errors.Trace(io.EOF)
	default:
	}
	c.Lock()
	defer c.Unlock()
	if c.out != nil {
		return errors.Errorf("Trying to send more than one frame. Existing: %v, new: %v", c.out, f)
	}
	c.out = f
	return nil
}

func (c *inboundHttpCodec) Close() {
	c.Lock()
	defer c.Unlock()
	if c.rw != nil {
		glog.V(2).Infof("Response finished, frame: %v", c.out)
		if c.out != nil {
			c.rw.Header().Set("Content-Type", "application/json")
			var err error
			if c.isSingleShot {
				// Elide fields that are implicit.
				f := c.out
				f.Src = ""
				f.Dst = ""
				err = json.NewEncoder(c.rw).Encode(f)
			} else {
				err = json.NewEncoder(c.rw).Encode(c.out)
			}
			if err != nil {
				glog.Errorf("Failed to serialize the response (%s): %+v", err, c.out)
				// Too late to return an error to the client.
			}
		} else {
			// For channel to be closed without a response you must've done something bad.
			// TODO(rojer): Find a way to propagate error details.
			http.Error(c.rw, "Bad request.", http.StatusBadRequest)
		}
	} else if c.out != nil {
		glog.Warningf("HTTP connection to %s closed before response was sent, lost frame: %v.", c.req.RemoteAddr, c.out)
	}
	c.closeOnce.Do(c.sendAndClose)
}

func (c *inboundHttpCodec) sendAndClose() {
	close(c.closeNotifier)
}

func (c *inboundHttpCodec) CloseNotify() <-chan struct{} {
	return c.closeNotifier
}

func (c *inboundHttpCodec) MaxNumFrames() int {
	if c.isSingleShot {
		return 1
	} else {
		return -1
	}
}

func (c *inboundHttpCodec) Info() ConnectionInfo {
	ci := ConnectionInfo{
		TLS:        c.req.TLS != nil,
		RemoteAddr: c.req.RemoteAddr,
	}
	if c.req.TLS != nil {
		ci.PeerCertificates = c.req.TLS.PeerCertificates
	}
	return ci
}

func (c *inboundHttpCodec) SetOptions(opts *Options) error {
	return errors.NotImplementedf("SetOptions")
}
