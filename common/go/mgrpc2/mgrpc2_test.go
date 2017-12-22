package mgrpc2

import (
	"context"
	"net/http"
	"testing"

	"golang.org/x/net/websocket"
)

var dispatcher Dispatcher
var listeningAddr = ":12345"
var rpcAddr = "ws://127.0.0.1" + listeningAddr + "/rpc"

func WSServer(ws *websocket.Conn) {
	dispatcher.AddChannel(ws)
}

func TestRPC(t *testing.T) {
	dispatcher = CreateDispatcher()
	dispatcher.AddHandler("*", func(d Dispatcher, req *Frame) *Frame {
		return &Frame{Error: &FrameError{Code: 500, Message: "Random error"}}
	})

	go func() {
		http.Handle("/rpc", websocket.Handler(WSServer))
		err := http.ListenAndServe(listeningAddr, nil)
		if err != nil {
			t.Error("error creating http listener:", err)
		}
	}()

	if _, err := dispatcher.Connect("foo"); err == nil {
		t.Error("expecting error")
	}
	res, err := dispatcher.Call(context.Background(), &Frame{ID: 123, Tag: "xyz", Dst: rpcAddr})
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
}
