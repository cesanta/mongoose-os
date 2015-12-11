package ourjson

import "encoding/json"

type RawMessage struct {
	json.RawMessage
}

func (m RawMessage) MarshalJSON() ([]byte, error) {
	return m.RawMessage.MarshalJSON()
}

func (m *RawMessage) UnmarshalJSON(data []byte) error {
	return m.RawMessage.UnmarshalJSON(data)
}
