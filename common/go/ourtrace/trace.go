package ourtrace

import "golang.org/x/net/trace"

var (
	// Sampler is meant to be overriden
	Sampler = TraceSampler(nullTraceSampler{})
)

// A TraceSampler decides whether and how to record a trace
type TraceSampler interface {
	Sample(family, title string) Recorder
}

// A Recorder receives all events set on a trace
type Recorder interface {
	SetSpan(traceID, spanID, parentSpanID uint64)
	Printf(format string, args ...interface{})
	Finish()
}

type nullTraceSampler struct{}

func (nullTraceSampler) Sample(family, title string) Recorder {
	return NullRecorder()
}

type nullRecorder struct{}

func (*nullRecorder) SetSpan(traceID, spanID, parentSpanID uint64) {}
func (*nullRecorder) Printf(format string, args ...interface{})    {}
func (*nullRecorder) Finish()                                      {}

// NullRecorder returns a recorder that doesn't do anything
func NullRecorder() Recorder {
	return (*nullRecorder)(nil)
}

// Trace wraps a "x/net/trace".Trace structure but provides
// access to the Trace and SpanID. It also adds stores the parent span ID.
type Trace struct {
	trace.Trace
	TraceID      uint64
	SpanID       uint64
	ParentSpanID uint64

	recorder Recorder
}

// SetTraceInfo hooks "x/net/trace".Trace
func (t *Trace) SetTraceInfo(traceID, spanID uint64) {
	t.TraceID = traceID
	t.SpanID = spanID
	t.Trace.SetTraceInfo(traceID, spanID)
}

// SetTraceInfo hooks "x/net/trace".LazyPritnf
func (t *Trace) LazyPrintf(format string, args ...interface{}) {
	t.Trace.LazyPrintf(format, args...)
	t.recorder.Printf(format, args...)
}

// SetSpan is like SetTraceInfo but also sets the ParentSpanID
// It also passes the full trace identification to the recorder
// is a prerequisite for capturing logs, hence you should call this
// before invoking LazyPrintf
func (t *Trace) SetSpan(traceID, spanID, parentSpanID uint64) {
	t.SetTraceInfo(traceID, spanID)
	t.ParentSpanID = parentSpanID
	t.recorder.SetSpan(traceID, spanID, parentSpanID)
}

// Finish hooks "x/net/trace".Finish and invokes TraceFinished
func (t *Trace) Finish() {
	t.recorder.Finish()
	t.Trace.Finish()
}

// New returns our Trace wrapper around "x/net/trace".Trace
// because the upstream implementation is incomplete and they
// decided to hide the traceID/spanID members
func New(family, title string) *Trace {
	tr := trace.New(family, title)
	res := &Trace{Trace: tr}
	res.recorder = Sampler.Sample(family, title)
	return res
}
