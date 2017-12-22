package mgrpc2

import (
	"context"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"

	"golang.org/x/net/websocket"
)

func TestRPC(t *testing.T) {
	dispatcher := CreateDispatcher()
	dispatcher.AddHandler("*", func(d Dispatcher, req *Frame) *Frame {
		return &Frame{Error: &FrameError{Code: 500, Message: "Random error"}}
	})

	mux := http.NewServeMux()
	mux.Handle("/rpc", websocket.Handler(func(ws *websocket.Conn) {
		dispatcher.AddChannel(ws)
	}))

	srv := httptest.NewServer(mux)
	defer srv.Close()
	rpcAddr := strings.Replace(srv.URL, "http://", "ws://", 1) + "/rpc"

	if _, err := dispatcher.Connect("foo"); err == nil {
		t.Error("expecting error")
	}
	t.Log(rpcAddr)
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
