package ourjson

import (
	"fmt"
	"testing"

	"github.com/cesanta/ubjson"
)

// TestNumbersRecoding verifies that numbers don't lose precision during
// recoding JSON into UBJSON. It happened before because ubjson library used
// `json.Unmarshal` on an `interface{}`, which converted all numbers to float64.
func TestNumbersRecoding(t *testing.T) {
	// TODO(imax): add similar test case to github.com/cesanta/ubjson
	const magic int64 = 1454700591424242205
	var s = struct {
		A RawMessage
	}{RawJSON([]byte(fmt.Sprintf("%d", magic)))}
	b2, err := ubjson.Marshal(&s)
	if err != nil {
		t.Fatalf("Failed to marshal struct as UBJSON: %s", err)
	}
	t.Logf("UBJSON: %#v", string(b2))
	var s2 struct {
		A RawMessage
	}
	if err := ubjson.Unmarshal(b2, &s2); err != nil {
		t.Fatalf("Failed to unmarshal UBJSON into struct: %s", err)
	}
	var v int64
	if err := s2.A.UnmarshalInto(&v); err != nil {
		t.Fatalf("Failed to unmarshal RawMessage %s into int64: %s", s2.A, err)
	}
	if v != magic {
		t.Errorf("v == %d, want %d", v, magic)
	}
}
