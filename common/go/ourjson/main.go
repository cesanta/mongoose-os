package ourjson

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"reflect"

	"cesanta.com/common/go/limitedwriter"
	"github.com/cesanta/errors"
	"github.com/cesanta/ubjson"
)

// RawMessage must be a slice in order for `omitempty` flag to work properly.

type RawMessage []rawMessage

type rawMessage interface {
	MarshalJSON() ([]byte, error)
	MarshalUBJSON() ([]byte, error)
	UnmarshalInto(interface{}) error
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

func (m RawMessage) MarshalUBJSON() ([]byte, error) {
	if !m.IsInitialized() {
		return nil, errors.New("not initialized")
	}
	b, err := m[0].MarshalUBJSON()
	return b, errors.Trace(err)
}

func (m *RawMessage) UnmarshalUBJSON(data []byte) error {
	*m = []rawMessage{ubjsonRawMessage(data)}
	return nil
}

func (m RawMessage) UnmarshalInto(v interface{}) error {
	if !m.IsInitialized() {
		return errors.New("not initialized")
	}
	return errors.Trace(m[0].UnmarshalInto(v))
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

func RawUBJSON(data []byte) RawMessage {
	return RawMessage{ubjsonRawMessage(data)}
}

func DelayMarshaling(v interface{}) RawMessage {
	return RawMessage{delayMarshaling{v}}
}

/// JSON

type jsonRawMessage []byte

func (m jsonRawMessage) MarshalJSON() ([]byte, error) {
	return m, nil
}

func (m jsonRawMessage) MarshalUBJSON() ([]byte, error) {
	t := json.RawMessage(m)
	b, err := ubjson.Marshal(&t)
	return b, errors.Trace(err)
}

func (m jsonRawMessage) UnmarshalInto(v interface{}) error {
	return json.Unmarshal(m, v)
}

func (m jsonRawMessage) String() string {
	if len(m) > 128 {
		return fmt.Sprintf("JSON: %#v... (%d)", string(m[:128]), len(m))
	}
	return fmt.Sprintf("JSON: %#v", string(m))
}

/// UBJSON

type ubjsonRawMessage []byte

func (m ubjsonRawMessage) MarshalJSON() ([]byte, error) {
	// TODO(imax): translate UBJSON into JSON directly, without full deserialization.
	var v interface{}
	if err := ubjson.Unmarshal(m, &v); err != nil {
		return nil, errors.Annotatef(err, "UBJSON unmarshaling")
	}
	b, err := json.Marshal(v)
	return b, errors.Trace(err)
}

func (m ubjsonRawMessage) MarshalUBJSON() ([]byte, error) {
	return m, nil
}

func (m ubjsonRawMessage) UnmarshalInto(v interface{}) error {
	return ubjson.Unmarshal(m, v)
}

func (m ubjsonRawMessage) String() string {
	if len(m) > 128 {
		return fmt.Sprintf("UBJSON: %#v... (%d)", string(m[:128]), len(m))
	}
	return fmt.Sprintf("UBJSON: %#v", string(m))
}

/// Delayed marshaling

type delayMarshaling struct {
	val interface{}
}

func (m delayMarshaling) MarshalJSON() ([]byte, error) {
	if b, ok := m.val.([]byte); ok {
		v := make([]uint16, len(b))
		for i, n := range b {
			v[i] = uint16(n)
		}
		return json.Marshal(v)
	}
	return json.Marshal(m.val)
}

func (m delayMarshaling) MarshalUBJSON() ([]byte, error) {
	return ubjson.Marshal(m.val)
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

func (m delayMarshaling) String() string {
	buf := bytes.NewBuffer(nil)
	lim := limitedwriter.New(buf, 1024)
	if _, err := fmt.Fprintf(lim, "Delayed marshaler: %#v", m.val); err == io.EOF {
		fmt.Fprint(buf, "...")
	}

	return buf.String()
}
