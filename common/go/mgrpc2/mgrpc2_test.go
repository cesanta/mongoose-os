package mgrpc2

import (
	"context"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"reflect"
	"strings"
	"sync"
	"testing"

	"golang.org/x/net/websocket"
)

type serverCloser func()

func mkdispatcher(t *testing.T) (Dispatcher, string, serverCloser) {
	d := CreateDispatcher(nil)
	mux := http.NewServeMux()
	mux.Handle("/rpc", websocket.Handler(func(ws *websocket.Conn) {
		d.AddChannel(ws)
	}))
	srv := httptest.NewServer(mux)
	rpcAddr := strings.Replace(srv.URL, "http://", "ws://", 1) + "/rpc"
	t.Log("Created server @ ", rpcAddr)
	return d, rpcAddr, srv.Close
}

func TestUnorderedRPC(t *testing.T) {
	actionsOrder := []int{}
	d, rpcAddr, done := mkdispatcher(t)
	defer done()

	// Simulate two devices that make requests in one order, but receive replies
	// in the opposite order. First device runs in this thread, and second one
	// runs in goroutine. First device calls Foo, second device calls Boo.
	m1 := &sync.Mutex{}
	m2 := &sync.Mutex{}
	m1.Lock()
	m2.Lock()

	d.AddHandler("Foo", func(d Dispatcher, c Channel, req *Frame) *Frame {
		m2.Unlock() // Let the goroutine (second device) unlock and proceed
		m1.Lock()   // and wait until the second device finishes
		actionsOrder = append(actionsOrder, 5)
		return &Frame{Result: json.RawMessage("1")}
	})
	d.AddHandler("Boo", func(d Dispatcher, c Channel, req *Frame) *Frame {
		actionsOrder = append(actionsOrder, 3)
		return &Frame{Result: json.RawMessage("2")}
	})

	go func() {
		m2.Lock() // Wait until first device makes a call and triggers a handler
		d2, _, done2 := mkdispatcher(t)
		defer done2()
		actionsOrder = append(actionsOrder, 2)
		req := &Frame{Tag: "xyz", Dst: rpcAddr, Method: "Boo", ID: 888}
		res, err := d2.Call(context.Background(), req)
		actionsOrder = append(actionsOrder, 4)
		if err != nil {
			t.Error("call error:", err)
		} else if res.ID != req.ID {
			t.Error("invalid frame ID")
		} else if res.Error != nil {
			t.Error("expecting success")
		} else if string(res.Result) != "2" {
			t.Error("wrong result")
		}
		m1.Unlock() // Unlock first device handler
	}()

	actionsOrder = append(actionsOrder, 1)
	req := &Frame{Tag: "xyz", Dst: rpcAddr, Method: "Foo", ID: 777}
	res, err := d.Call(context.Background(), req)
	actionsOrder = append(actionsOrder, 6)
	if err != nil {
		t.Error("call error:", err)
	} else if res.ID != req.ID {
		t.Error("invalid frame ID")
	} else if res.Error != nil {
		t.Error("expecting success")
	} else if string(res.Result) != "1" {
		t.Error("wrong result")
	}

	expectedOrder := []int{1, 2, 3, 4, 5, 6}
	if !reflect.DeepEqual(actionsOrder, expectedOrder) {
		t.Error("Wrong actions order:", actionsOrder, " expecting ", expectedOrder)
	}
}

func TestRPC(t *testing.T) {
	dispatcher, rpcAddr, done := mkdispatcher(t)
	defer done()
	dispatcher.AddHandler("Boo", func(d Dispatcher, c Channel, req *Frame) *Frame {
		return &Frame{Error: &FrameError{Code: 500, Message: "Random error"}}
	})

	d2, rpc2Addr, done2 := mkdispatcher(t)
	defer done2()
	d2.AddHandler("Foo", func(d Dispatcher, c Channel, req *Frame) *Frame {
		return &Frame{Result: json.RawMessage(`true`)}
	})

	// Connect to a junk address, expect error
	if _, err := dispatcher.Connect("foo"); err == nil {
		t.Error("expecting error")
	}

	// Talk to the first server - share a dispatcher with it
	req := &Frame{ID: 123, Tag: "xyz", Dst: rpcAddr, Method: "Boo"}
	res, err := dispatcher.Call(context.Background(), req)
	if err != nil {
		t.Error("call error:", err)
	} else if res.ID != 123 {
		t.Error("invalid frame ID")
	} else if res.Tag != "xyz" {
		t.Error("invalid tag")
	} else if res.Error == nil {
		t.Error("expecting error")
	} else if res.Error.Code != 500 {
		t.Error("expecting error code 500")
	}

	// Talk to the second server. Call a non-existent method
	req2 := &Frame{Tag: "xyz", Dst: rpc2Addr, Method: "Blaaaaah"}
	res2, err2 := dispatcher.Call(context.Background(), req2)
	if err2 != nil {
		t.Error("call error:", err2)
	} else if res2.ID != req2.ID {
		t.Error("invalid frame ID")
	} else if res2.Error == nil {
		t.Error("expecting error")
	} else if res2.Error.Code != 404 {
		t.Error("expecting error code 404")
	}

	// Talk to the second server. Expect success.
	req3 := &Frame{Tag: "xyz", Dst: rpc2Addr, Method: "Foo"}
	res3, err3 := dispatcher.Call(context.Background(), req3)
	if err3 != nil {
		t.Error("call error:", err3)
	} else if res3.ID == res2.ID {
		t.Error("frame ID does not increment")
	} else if res3.ID != req3.ID {
		t.Error("invalid frame ID")
	} else if res3.Error != nil {
		t.Error("expecting success")
	} else if string(res3.Result) != `true` {
		t.Error("wrong result")
	}
}
