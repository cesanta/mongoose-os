package limitedwriter

import (
	"bytes"
	"fmt"
	"io"
	"testing"
)

func TestLimitedWriter(t *testing.T) {
	buf := bytes.NewBuffer(nil)
	w := New(buf, 3)
	_, err := fmt.Fprint(w, "ciao")

	if got, want := buf.String(), "cia"; got != want {
		t.Errorf("got: %v, want: %s", got, want)
	}

	if got, want := err, io.EOF; got != want {
		t.Errorf("got: %v, want: %s", got, want)
	}
}
