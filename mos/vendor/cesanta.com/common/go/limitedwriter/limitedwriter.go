package limitedwriter

import (
	"io"

	"github.com/cesanta/errors"
)

type limitedWriter struct {
	io.Writer
	limit int
}

// New returns a new limited writer.
//
// A limited writer will return error after writing limit bytes.
func New(w io.Writer, limit int) io.Writer {
	return &limitedWriter{w, limit}
}

func (w *limitedWriter) Write(b []byte) (int, error) {
	limited := false
	if len(b) > w.limit {
		b = b[:w.limit]
		limited = true
	}
	w.limit -= len(b)

	n, err := w.Writer.Write(b)
	if limited && err == nil {
		return n, io.EOF
	}
	return n, errors.Trace(err)
}
