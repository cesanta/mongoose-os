package mgrpc2

import (
	"testing"
)

func TestRPC(t *testing.T) {
	d := CreateDispatcher()
	if _, err := d.Connect("foo"); err == nil {
		t.Error("expecting error")
	}
	if _, err := d.Connect("ws://127.0.0.1:1234/rpc"); err == nil {
		t.Error("connection error:", err)
	}
}
