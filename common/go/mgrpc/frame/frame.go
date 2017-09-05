package frame

import (
	"bytes"
	"fmt"
	"math/rand"
	"time"

	"cesanta.com/common/go/limitedwriter"
	"cesanta.com/common/go/ourjson"
)

// Frame is a basic data structure that contains request or response.
type Frame struct {
	// Version denotes the protocol version in use. Must be set to 1.
	Version int `json:"v,omitempty"`

	// Src is the ID of the sender.
	Src string `json:"src,omitempty"`

	// Dst is the ID of the recipient.
	Dst string `json:"dst,omitempty"`

	// Key should contains pre-shared key if client certificates are not used.
	Key string `json:"key,omitempty"`

	// Tag, if present, should be copied verbatim to the response frame.
	Tag string `json:"tag,omitempty"`

	ID int64 `json:"id,omitempty"`

	// Request
	Method string             `json:"method,omitempty"`
	Args   ourjson.RawMessage `json:"args,omitempty"`
	// Timestamp (as number of seconds since Epoch) of when the command result is no longer relevant.
	Deadline int64 `json:"deadline,omitempty"`
	// Number of seconds after reception of the command after when the command result is no longer relevant.
	Timeout int64 `json:"timeout,omitempty"`

	// Response
	Result ourjson.RawMessage `json:"result,omitempty"`
	Error  *Error             `json:"error,omitempty"`

	Trace *Trace `json:"trace,omitempty"`

	Auth *FrameAuth `json:"auth,omitempty"`

	// Size hint, if present, gives approximate size of the frame in memory.
	SizeHint int `json:"-"`

	// Send no response to this frame
	NoResponse bool `json:"nr,omitempty"`
}

type Error struct {
	Code    int    `json:"code"`
	Message string `json:"message,omitempty"`
}

type Command struct {
	Cmd  string             `json:"cmd"`
	ID   int64              `json:"id,omitempty"`
	Args ourjson.RawMessage `json:"args,omitempty"`

	Auth *FrameAuth `json:"auth,omitempty"`

	// Timestamp (as number of seconds since Epoch) of when the command result is no longer relevant.
	Deadline int64 `json:"deadline,omitempty"`

	// Number of seconds after reception of the command after when the command result is no longer relevant.
	Timeout int64 `json:"timeout,omitempty"`

	Trace *Trace `json:"trace,omitempty"`
}

type FrameAuth struct {
	Realm    string `json:"realm"`
	Username string `json:"username"`
	Nonce    int    `json:"nonce"`
	CNonce   int    `json:"cnonce"`
	Response string `json:"response"`
}

// Trace groups optional call tracing info.
type Trace struct {
	// Dapper trace ID
	TraceID int64 `json:"id,omitempty"`
	// Dapper span ID
	SpanID int64 `json:"span,omitempty"`
}

type Response struct {
	ID int64 `json:"id"`

	// Status code. Non-zero value means error.
	Status int `json:"status"`

	// Human-readable explanation of an error, if any.
	StatusMsg string `json:"status_msg,omitempty"`

	// Application defined response payload
	Response ourjson.RawMessage `json:"resp,omitempty"`
}

// Auto-generated uids should be "large but not ginormous".
const autoUidPrefix int64 = 1 << 40

// CreateCommandUID creates a unique UID for commands.
func CreateCommandUID() int64 {
	return rand.Int63n(autoUidPrefix) | autoUidPrefix
}

func (f *Frame) IsRequest() bool {
	return f.Method != ""
}

const frameSizeStringifyLimit = 2048

func (f *Frame) String() string {
	buf := bytes.NewBuffer(nil)
	lim := limitedwriter.New(buf, frameSizeStringifyLimit) // in case the hint is missing or wrong
	fmt.Fprintf(lim, "%q -> %q v=%d id=%d ", f.Src, f.Dst, f.Version, f.ID)
	if f.SizeHint < frameSizeStringifyLimit {
		if f.IsRequest() {
			fmt.Fprintf(lim, "%s args=%v %d", f.Method, f.Args, f.SizeHint)
		} else {
			fmt.Fprintf(lim, "result=%v error=%v %d", f.Result, f.Error, f.SizeHint)
		}
	} else {
		if f.IsRequest() {
			fmt.Fprintf(lim, "%s args=(too big) %d", f.Method, f.SizeHint)
		} else {
			fmt.Fprintf(lim, "result=(too big) error=%v %d", f.Error, f.SizeHint)
		}
	}
	return buf.String()
}

func (c Command) String() string {
	r := fmt.Sprintf("{%s id=%d", c.Cmd, c.ID)
	if len(c.Args) > 0 {
		r += fmt.Sprintf(" args=%q", c.Args)
	}
	if c.Deadline != 0 {
		r += fmt.Sprintf(" deadline=%d (%+ds from now)", c.Deadline, c.Deadline-time.Now().Unix())
	}
	if c.Auth != nil {
		r += fmt.Sprintf(" auth=%v", c.Auth)
	}
	return r + "}"
}

func (r Response) String() string {
	ret := "{"
	if r.Status == 0 {
		ret += "OK"
	} else {
		ret += fmt.Sprintf("status=%d", r.Status)
	}
	ret += fmt.Sprintf(" id=%d", r.ID)
	if r.StatusMsg != "" {
		ret += fmt.Sprintf(" msg=%q", r.StatusMsg)
	}
	if len(r.Response) > 0 {
		ret += fmt.Sprintf(" resp=%q", r.Response)
	}
	return ret + "}"
}

func NewRequestFrame(src string, dst string, key string, cmd *Command) *Frame {
	return &Frame{
		Src:      src,
		Dst:      dst,
		Key:      key,
		ID:       cmd.ID,
		Method:   cmd.Cmd,
		Args:     cmd.Args,
		Auth:     cmd.Auth,
		Deadline: cmd.Deadline,
		Timeout:  cmd.Timeout,
		Trace:    cmd.Trace,
	}
}

func NewResponseFrame(src string, dst string, key string, resp *Response) *Frame {
	f := &Frame{
		Version: 2,
		Src:     src,
		Dst:     dst,
		Key:     key,
		ID:      resp.ID,
		Result:  resp.Response,
	}
	if resp.Status != 0 {
		f.Error = &Error{Code: resp.Status, Message: resp.StatusMsg}
	}
	return f
}

func NewCommandFromFrame(f *Frame) *Command {
	return &Command{
		Cmd:      f.Method,
		ID:       f.ID,
		Args:     f.Args,
		Deadline: f.Deadline,
		Timeout:  f.Timeout,
		Trace:    f.Trace,
	}
}

func NewResponseFromFrame(f *Frame) *Response {
	r := &Response{ID: f.ID, Response: f.Result}
	if f.Error != nil {
		r.Status = f.Error.Code
		r.StatusMsg = f.Error.Message
	}
	return r
}

func MarshalJSON(f *Frame) ([]byte, error) {
	return ourjson.MarshalJSONNoHTMLEscape(f)
}
