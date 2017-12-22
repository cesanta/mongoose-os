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

type Channel io.ReadWriteCloser

type Dispatcher interface {
	Connect(address string) (Channel, error)
	Call(ctx context.Context, request *Frame) (*Frame, error)
	AddHandler(method string, handler Handler)
	AddChannel(channel Channel)
}

type dispImpl struct {
	addrMap      map[string]Channel
	addrMapLock  sync.Mutex
	handlers     map[string]Handler
	handlersLock sync.Mutex
	address      string
	nextId       int64
}

func (d *dispImpl) Connect(address string) (Channel, error) {
	if strings.HasPrefix(address, "ws://") || strings.HasPrefix(address, "wss://") {
		ws, err := websocket.Dial(address, "", "http://localhost")
		if err != nil {
			fmt.Println(fmt.Errorf("Error connecting: %v", err))
			return nil, err
		} else {
			d.addrMap[address] = ws
			return ws, err
		}
	} else {
		return nil, fmt.Errorf("Unknown address type: %s", address)
	}
}

func (d *dispImpl) AddHandler(method string, handler Handler) {
	d.handlersLock.Lock()
	defer d.handlersLock.Unlock()
	d.handlers[method] = handler
}

func (d *dispImpl) lookupChannel(address string) Channel {
	d.addrMapLock.Lock()
	defer d.addrMapLock.Unlock()
	c := d.addrMap[address]
	if c == nil && address != "" {
		c, _ = d.Connect(address)
	}
	return c
}

func (d *dispImpl) AddChannel(channel Channel) {
	addrMap := make(map[string]bool)

	// TODO(lsm): refactor this blocking thing
	for {
		log.Printf("Reading request from channel [%p]...", channel)
		buf := make([]byte, 1024*16)
		n, err := channel.Read(buf)
		if err != nil {
			log.Printf("Read error: %p", channel)
			break
		}
		frame := Frame{}
		err = json.Unmarshal(buf[:n], &frame)
		if err != nil {
			log.Printf("Invalid frame from %p: [%s]", channel, buf[:n])
			continue
		}

		if frame.Src != "" {
			// Associate the address of the peer with this channel
			addrMap[frame.Src] = true
			d.addrMapLock.Lock()
			d.addrMap[frame.Src] = channel
			d.addrMapLock.Unlock()
			log.Printf("Associating address [%s] with channel %p", frame.Src, channel)
		}

		log.Printf("Got: [%s]", buf[:n])
		var response *Frame
		callback, _ := d.handlers[frame.Method]
		if callback == nil {
			// Try to lookup the catch-all handler
			callback, _ = d.handlers["*"]
		}
		if callback != nil {
			response = callback(d, &frame)
		} else {
			response = &Frame{Error: &FrameError{Code: 404, Message: "Method not found"}}
		}
		response.ID = frame.ID
		response.Tag = frame.Tag
		response.Dst = frame.Src
		str, _ := json.Marshal(response)
		log.Printf("Reply: [%s]", string(str))
		channel.Write(str)
	}

	// Channel is closing, delete all associated addresses
	d.addrMapLock.Lock()
	for address, _ := range addrMap {
		delete(d.addrMap, address)
	}
	d.addrMapLock.Unlock()
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
	if request.Src == "" {
		request.Src = d.address
	}
	s, _ := json.Marshal(request)
	log.Printf("Sending: [%s]", string(s))
	n, werr := c.Write(s)
	if werr != nil {
		return nil, fmt.Errorf("Write error %p", werr)
	}
	log.Printf("Sent %d out of %d bytes, reading reply...", n, len(s))

	// TODO(lsm): do it properly. We may get a reply to a different request.
	var msg = make([]byte, 100*1024)
	n, err := c.Read(msg)
	log.Printf("Got reply: [%s], err %p", msg[:n], err)
	if err != nil {
		return nil, err
	}
	log.Printf("Received: [%s]", msg[:n])
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
		addrMap:  make(map[string]Channel),
		handlers: make(map[string]Handler),
		address:  fmt.Sprintf("rpc_%.4d", r.Int31()),
		nextId:   int64(r.Int31()),
	}
	return &d
}
