package ourtrace

import (
	"context"
	"testing"

	"golang.org/x/net/trace"
)

func TestNewContext(t *testing.T) {
	traceID := uint64(1)
	spanID := uint64(2)

	var tr trace.Trace
	tr = New("foo", "bar")
	tr.SetTraceInfo(traceID, spanID)

	ctx := trace.NewContext(context.Background(), tr)
	tr, ok := trace.FromContext(ctx)
	if !ok {
		t.Fatal("context should contain our trace")
	}
	gotr := tr.(*Trace)

	if got, want := gotr.TraceID, traceID; got != want {
		t.Errorf("got: %d, want: %d", got, want)
	}
	if got, want := gotr.SpanID, spanID; got != want {
		t.Errorf("got: %d, want: %d", got, want)
	}
}

func TestNullSampler(t *testing.T) {
	var s TraceSampler
	s = nullTraceSampler{}
	r := s.Sample("foo", "bar")
	r.Finish()
}
