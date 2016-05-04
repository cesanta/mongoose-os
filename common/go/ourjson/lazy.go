package ourjson

import (
	"encoding/json"
	"fmt"
)

// LazyJSON returns a fmt.Stringer that will lazily marshal
// v into JSON when the String method is invoked.
func LazyJSON(v interface{}) fmt.Stringer {
	return lazyJSON{v}
}

type lazyJSON struct {
	v interface{}
}

func (l lazyJSON) String() string {
	b, err := json.Marshal(l.v)
	if err != nil {
		return err.Error()
	}
	return string(b)
}
