package mgrpc2

import (
	"context"
	"encoding/json"
	"log"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"

	"golang.org/x/net/websocket"
)

func TestRPC(t *testing.T) {
	dispatcher := CreateDispatcher()
	dispatcher.AddHandler("Boo", func(d Dispatcher, req *Frame) *Frame {
		return &Frame{Error: &FrameError{Code: 500, Message: "Random error"}}
	})

	// Create first server
	mux := http.NewServeMux()
	mux.Handle("/rpc", websocket.Handler(func(ws *websocket.Conn) {
		log.Println("1st server")
		dispatcher.AddChannel(ws)
	}))
	srv := httptest.NewServer(mux)
	defer srv.Close()
	rpcAddr := strings.Replace(srv.URL, "http://", "ws://", 1) + "/rpc"

	// Create a second server with its own dispatcher
	d2 := CreateDispatcher()
	d2.AddHandler("Foo", func(d Dispatcher, req *Frame) *Frame {
		return &Frame{Result: json.RawMessage(`true`)}
	})
	mux2 := http.NewServeMux()
	mux2.Handle("/rpc", websocket.Handler(func(ws *websocket.Conn) {
		log.Println("2nd server")
		d2.AddChannel(ws)
	}))
	srv2 := httptest.NewServer(mux2)
	defer srv2.Close()
	rpc2Addr := strings.Replace(srv2.URL, "http://", "ws://", 1) + "/rpc"

	log.Printf("Servers: [%s] [%s]", rpcAddr, rpc2Addr)

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
	s, _ := json.Marshal(res2)
	log.Println(string(s))
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
