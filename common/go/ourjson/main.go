package ourjson

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"reflect"

	"cesanta.com/common/go/limitedwriter"
	"github.com/cesanta/errors"
)

// RawMessage must be a slice in order for `omitempty` flag to work properly.

type RawMessage []rawMessage

type rawMessage interface {
	MarshalJSON() ([]byte, error)
	UnmarshalInto(interface{}) error
	UnmarshalIntoUseNumber(interface{}) error
	String() string
}

func (m RawMessage) IsInitialized() bool {
	return len(m) > 0
}

func (m RawMessage) MarshalJSON() ([]byte, error) {
	if !m.IsInitialized() {
		return nil, errors.New("not initialized")
	}
	b, err := m[0].MarshalJSON()
	return b, errors.Trace(err)
}

func (m *RawMessage) UnmarshalJSON(data []byte) error {
	*m = []rawMessage{jsonRawMessage(data)}
	return nil
}

func (m RawMessage) UnmarshalInto(v interface{}) error {
	if !m.IsInitialized() {
		return errors.New("not initialized")
	}
	return errors.Trace(m[0].UnmarshalInto(v))
}

func (m RawMessage) UnmarshalIntoUseNumber(v interface{}) error {
	if !m.IsInitialized() {
		return errors.New("not initialized")
	}
	return errors.Trace(m[0].UnmarshalIntoUseNumber(v))
}

func (m RawMessage) String() string {
	if !m.IsInitialized() {
		return "uninitialized"
	}
	return m[0].String()
}

func RawJSON(data []byte) RawMessage {
	return RawMessage{jsonRawMessage(data)}
}

func DelayMarshaling(v interface{}) RawMessage {
	return RawMessage{delayMarshaling{v}}
}

/// JSON

type jsonRawMessage []byte

func (m jsonRawMessage) MarshalJSON() ([]byte, error) {
	return m, nil
}

func (m jsonRawMessage) UnmarshalInto(v interface{}) error {
	return json.Unmarshal(m, v)
}

func (m jsonRawMessage) UnmarshalIntoUseNumber(v interface{}) error {
	dec := json.NewDecoder(bytes.NewBuffer(m))
	dec.UseNumber()
	return dec.Decode(v)
}

func (m jsonRawMessage) String() string {
	if len(m) > 128 {
		return fmt.Sprintf("JSON: %#v... (%d)", string(m[:128]), len(m))
	}
	return fmt.Sprintf("JSON: %#v", string(m))
}

/// Delayed marshaling

type delayMarshaling struct {
	val interface{}
}

func MarshalJSONNoHTMLEscape(v interface{}) ([]byte, error) {
	w := bytes.NewBuffer(nil)
	e := json.NewEncoder(w)
	// With Go 1.7+ we can do this:
	// e.SetEscapeHTML(false)
	err := e.Encode(v)
	if err != nil {
		return nil, errors.Trace(err)
	}
	// However, Ubuntu 16.04 comes with Go 1.6, so instead we do this:
	res := w.Bytes()
	res = bytes.Replace(res, []byte(`\u003c`), []byte(`<`), -1)
	res = bytes.Replace(res, []byte(`\u003e`), []byte(`>`), -1)
	res = bytes.Replace(res, []byte(`\u0026`), []byte(`&`), -1)
	return res, nil
}

func (m delayMarshaling) MarshalJSON() ([]byte, error) {
	if b, ok := m.val.([]byte); ok {
		v := make([]uint16, len(b))
		for i, n := range b {
			v[i] = uint16(n)
		}
		return MarshalJSONNoHTMLEscape(v)
	}
	return MarshalJSONNoHTMLEscape(m.val)
}

func (m delayMarshaling) UnmarshalInto(v interface{}) error {
	rv := reflect.ValueOf(v)
	rval := reflect.ValueOf(m.val)

	if rv.Kind() != reflect.Ptr {
		return errors.Errorf("expecting pointer, got: %#v", v)
	}
	if rv.IsNil() {
		return errors.Errorf("cannot unmarshal into a nil pointer")
	}
	el := rv.Elem()
	if !el.CanSet() {
		return errors.Errorf("%#v is not writable", v)
	}

	if !rval.Type().AssignableTo(rv.Elem().Type()) {
		return errors.Errorf("%#v is not assignable from %#v", v, m.val)
	}

	el.Set(rval)
	return nil
}

func (m delayMarshaling) UnmarshalIntoUseNumber(v interface{}) error {
	return m.UnmarshalInto(v)
}

func (m delayMarshaling) String() string {
	buf := bytes.NewBuffer(nil)
	lim := limitedwriter.New(buf, 1024)
	if _, err := fmt.Fprintf(lim, "Delayed marshaler: %#v", m.val); err == io.EOF {
		fmt.Fprint(buf, "...")
	}

	return buf.String()
}
