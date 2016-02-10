package ourjson

import (
	"encoding/json"
	"fmt"

	"github.com/cesanta/ubjson"
	"github.com/juju/errors"
)

type RawMessage struct {
	value rawMessage
}

type rawMessage interface {
	MarshalJSON() ([]byte, error)
	MarshalUBJSON() ([]byte, error)
	UnmarshalInto(interface{}) error
	String() string
}

func (m RawMessage) MarshalJSON() ([]byte, error) {
	if m.value == nil {
		return nil, errors.New("not initialized")
	}
	b, err := m.value.MarshalJSON()
	return b, errors.Trace(err)
}

func (m *RawMessage) UnmarshalJSON(data []byte) error {
	m.value = jsonRawMessage(data)
	return nil
}

func (m RawMessage) MarshalUBJSON() ([]byte, error) {
	if m.value == nil {
		return nil, errors.New("not initialized")
	}
	b, err := m.value.MarshalUBJSON()
	return b, errors.Trace(err)
}

func (m *RawMessage) UnmarshalUBJSON(data []byte) error {
	m.value = ubjsonRawMessage(data)
	return nil
}

func (m RawMessage) UnmarshalInto(v interface{}) error {
	if m.value == nil {
		return errors.New("not initialized")
	}
	return errors.Trace(m.value.UnmarshalInto(v))
}

func (m RawMessage) String() string {
	if m.value == nil {
		return "uninitialized"
	}
	return m.value.String()
}

func RawJSON(data []byte) RawMessage {
	return RawMessage{value: jsonRawMessage(data)}
}

func RawUBJSON(data []byte) RawMessage {
	return RawMessage{value: ubjsonRawMessage(data)}
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
	return fmt.Sprintf("UBJSON: %#v", string(m))
}
