package mgrpc2

import (
	"encoding/json"
	"fmt"
	"io"
	"log"
	"math/rand"
	"strings"
	"sync"
	"time"

	"golang.org/x/net/context"

	"golang.org/x/net/websocket"
)

type Frame struct {
	Src        string          `json:"src,omitempty"`
	Dst        string          `json:"dst,omitempty"`
	Key        string          `json:"key,omitempty"`
	Tag        string          `json:"tag,omitempty"`
	ID         int64           `json:"id,omitempty"`
	Method     string          `json:"method,omitempty"`
	Args       json.RawMessage `json:"args,omitempty"`
	Deadline   int64           `json:"deadline,omitempty"`
	Timeout    int64           `json:"timeout,omitempty"`
	Result     json.RawMessage `json:"result,omitempty"`
	Error      *FrameError     `json:"error,omitempty"`
	Auth       *FrameAuth      `json:"auth,omitempty"`
	NoResponse bool            `json:"nr,omitempty"`
}

type FrameError struct {
	Code    int    `json:"code"`
	Message string `json:"message,omitempty"`
}

type FrameAuth struct {
	Realm    string `json:"realm"`
	Username string `json:"username"`
	Nonce    int    `json:"nonce"`
	CNonce   int    `json:"cnonce"`
	Response string `json:"response"`
}

type Handler func(Dispatcher, *Frame) *Frame

type Dispatcher interface {
	Connect(address string) (io.ReadWriteCloser, error)
	Call(ctx context.Context, request *Frame) (*Frame, error)
	AddHandler(method string, handler Handler)
}

type dispImpl struct {
	channels     map[string]io.ReadWriteCloser
	channelsLock sync.Mutex
	handlers     map[string]Handler
	handlersLock sync.Mutex
	src          string
	nextId       int64
}

func (d *dispImpl) Connect(address string) (io.ReadWriteCloser, error) {
	if strings.HasPrefix(address, "ws://") || strings.HasPrefix(address, "wss://") {
		ws, err := websocket.Dial(address, "", "http://localhost")
		if err != nil {
			fmt.Println(fmt.Errorf("Error connecting: %v", err))
			return nil, err
		} else {
			d.channels[address] = ws
			return ws, err
		}
	} else {
		return nil, fmt.Errorf("Unknown address type: %s", address)
	}
}

func (d *dispImpl) AddHandler(method string, handler Handler) {
	// TODO(lsm): implement
}

func (d *dispImpl) lookupChannel(address string) io.ReadWriteCloser {
	c := d.channels[address]
	if c == nil && address != "" {
		c, _ = d.Connect(address)
	}
	return c
}

func (d *dispImpl) Call(ctx context.Context, request *Frame) (*Frame, error) {
	c := d.lookupChannel(request.Dst)
	if c == nil {
		return nil, fmt.Errorf("No channel for %s", request.Dst)
	}
	if request.ID == 0 {
		d.nextId++
		request.ID = d.nextId
	}
	request.Dst = ""
	if request.Src == "" {
		request.Src = d.src
	}
	s, _ := json.Marshal(request)
	log.Printf("Sending: %s", string(s))
	c.Write(s)

	// TODO(lsm): do it properly. We may get a reply to a different request.
	var msg = make([]byte, 100*1024)
	n, err := c.Read(msg)
	if err != nil {
		return nil, err
	}
	log.Printf("Received: %s", msg[:n])
	var res Frame
	err = json.Unmarshal(msg[:n], &res)
	if err != nil {
		return nil, err
	}
	return &res, nil
}

func CreateDispatcher() Dispatcher {
	r := rand.New(rand.NewSource(time.Now().UnixNano()))
	d := dispImpl{
		channels: make(map[string]io.ReadWriteCloser),
		handlers: make(map[string]Handler),
		src:      fmt.Sprintf("rpc_%.4d", r.Int31()),
		nextId:   int64(r.Int31()),
	}
	return &d
}
