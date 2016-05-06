package ourtrace

import "golang.org/x/net/trace"

// Trace wraps a "x/net/trace".Trace structure but provides
// access to the Trace and SpanID.
type Trace struct {
	trace.Trace
	TraceID uint64
	SpanID  uint64
}

// SetTraceInfo hooks "x/net/trace".Trace
func (t *Trace) SetTraceInfo(traceID, spanID uint64) {
	t.TraceID = traceID
	t.SpanID = spanID
	t.Trace.SetTraceInfo(traceID, spanID)
}

// New returns our Trace wrapper around "x/net/trace".Trace
// because the upstream implementation is incomplete and they
// decided to hide the traceID/spanID members
func New(family, title string) trace.Trace {
	tr := trace.New(family, title)
	return &Trace{Trace: tr}
}
