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

type Channel io.ReadWriteCloser

type Handler func(Dispatcher, Channel, *Frame) *Frame

type Dispatcher interface {
	Connect(address string) (Channel, error)
	Call(ctx context.Context, request *Frame) (*Frame, error)
	AddHandler(method string, handler Handler)
	AddChannel(channel Channel)
}

type dispImpl struct {
	lock     sync.Mutex
	addrMap  map[string]Channel
	handlers map[string]Handler
	calls    map[int64]chan *Frame
	address  string
	nextId   int64
}

func (d *dispImpl) Connect(address string) (Channel, error) {
	if strings.HasPrefix(address, "ws://") || strings.HasPrefix(address, "wss://") {
		ws, err := websocket.Dial(address, "", "http://localhost")
		if err != nil {
			fmt.Println(fmt.Errorf("Error connecting: %v", err))
			return nil, err
		} else {
			d.addrMap[address] = ws
			go d.AddChannel(ws)
			return ws, err
		}
	} else {
		return nil, fmt.Errorf("Unknown address type: %s", address)
	}
}

func (d *dispImpl) AddHandler(method string, handler Handler) {
	d.lock.Lock()
	defer d.lock.Unlock()
	d.handlers[method] = handler
}

func (d *dispImpl) lookupChannel(address string) Channel {
	d.lock.Lock()
	defer d.lock.Unlock()
	c := d.addrMap[address]
	if c == nil && address != "" {
		c, _ = d.Connect(address)
	}
	return c
}

func (d *dispImpl) Dispatch(frame *Frame) bool {
	d.lock.Lock()
	ch, ok := d.calls[frame.ID]
	d.lock.Unlock()
	if ok {
		str, _ := json.Marshal(frame)
		log.Printf("Response (ch): [%s]", string(str))
		ch <- frame
	}
	return ok
}

func (d *dispImpl) AddChannel(channel Channel) {
	addrMap := make(map[string]bool)

	// TODO(lsm): refactor this blocking thing
	for {
		log.Printf("Reading request from channel [%p]...", channel)
		frame := Frame{}
		err := json.NewDecoder(channel).Decode(&frame)
		if err != nil {
			log.Printf("Invalid frame from %p: [%s]", channel)
			break
		}
		s, _ := json.Marshal(frame)
		log.Printf("Got: [%s]", string(s))

		if frame.Method == "" {
			// Reply
			d.Dispatch(&frame)
		} else {
			// Request
			if frame.Src != "" {
				// Associate the address of the peer with this channel
				addrMap[frame.Src] = true
				d.lock.Lock()
				d.addrMap[frame.Src] = channel
				d.lock.Unlock()
				log.Printf("Associating address [%s] with channel %p", frame.Src, channel)
			}

			var response *Frame
			callback, _ := d.handlers[frame.Method]
			if callback == nil {
				// Try to lookup the catch-all handler
				callback, _ = d.handlers["*"]
			}
			if callback != nil {
				response = callback(d, channel, &frame)
			} else {
				response = &Frame{Error: &FrameError{Code: 404, Message: "Method not found"}}
			}
			// Do not send any response if we're told
			if frame.NoResponse {
				continue
			}

			response.ID = frame.ID
			response.Tag = frame.Tag
			response.Dst = frame.Src

			if !d.Dispatch(response) {
				str, _ := json.Marshal(response)
				log.Printf("Response (io): [%s]", string(str))
				if _, err := channel.Write(str); err != nil {
					log.Printf("Write error: %v", err)
					break
				}
			}
		}
	}

	// Channel is closing, delete all associated addresses
	d.lock.Lock()
	for address, _ := range addrMap {
		delete(d.addrMap, address)
	}
	d.lock.Unlock()
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
	n, err := c.Write(s)
	if err != nil {
		return nil, fmt.Errorf("Write error %p", err)
	}
	log.Printf("Sent %d out of %d bytes, ID %d, waiting for reply...", n, len(s), request.ID)

	ch := make(chan *Frame)
	d.lock.Lock()
	d.calls[request.ID] = ch
	d.lock.Unlock()

	defer func() {
		d.lock.Lock()
		delete(d.calls, request.ID)
		d.lock.Unlock()
	}()

	log.Printf("Sent %d out of %d bytes, ID %d, waiting for reply...", n, len(s), request.ID)
	select {
	case res := <-ch:
		return res, nil
	case <-ctx.Done():
		return nil, ctx.Err()
	}
}

func CreateDispatcher() Dispatcher {
	r := rand.New(rand.NewSource(time.Now().UnixNano()))
	d := dispImpl{
		addrMap:  make(map[string]Channel),
		handlers: make(map[string]Handler),
		address:  fmt.Sprintf("rpc_%.4d", r.Int31()),
		nextId:   int64(r.Int31()),
		calls:    make(map[int64]chan *Frame),
	}
	return &d
}
