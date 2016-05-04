package ourjson

import (
	"fmt"
	"testing"
)

func TestLazyJSON(t *testing.T) {
	type foo struct {
		A int
		B string
	}

	v := foo{}
	l := LazyJSON(&v)

	if got, want := fmt.Sprint(l), `{"A":0,"B":""}`; got != want {
		t.Errorf("got: %q, want: %q", got, want)
	}

	v.A = 42
	v.B = "the answer"

	if got, want := fmt.Sprint(l), `{"A":42,"B":"the answer"}`; got != want {
		t.Errorf("got: %q, want: %q", got, want)
	}

	// by value
	v = foo{}
	l = LazyJSON(v)
	v.A = 42
	v.B = "the answer"

	if got, want := fmt.Sprint(l), `{"A":0,"B":""}`; got != want {
		t.Errorf("got: %q, want: %q", got, want)
	}
}
