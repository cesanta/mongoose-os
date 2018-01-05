package schema

import (
	"fmt"

	json "github.com/cesanta/ucl"
)

func equal(a json.Value, b json.Value) bool {
	switch x := a.(type) {
	case *json.Array:
		b, ok := b.(*json.Array)
		if !ok {
			return false
		}
		if len(x.Value) != len(b.Value) {
			return false
		}
		for i, item := range x.Value {
			if !equal(item, b.Value[i]) {
				return false
			}
		}
		return true
	case *json.Bool:
		b, ok := b.(*json.Bool)
		if !ok {
			return false
		}
		return x.Value == b.Value
	case *json.Number:
		switch b := b.(type) {
		case *json.Number:
			return x.Value == b.Value // XXX: comparing floating point numbers.
		case *json.Integer:
			return x.Value == float64(b.Value) // XXX: comparing floating point numbers.
		default:
			return false
		}
	case *json.Integer:
		switch b := b.(type) {
		case *json.Number:
			return float64(x.Value) == b.Value // XXX: comparing floating point numbers.
		case *json.Integer:
			return x.Value == b.Value
		default:
			return false
		}
	case *json.Null:
		_, ok := b.(*json.Null)
		if !ok {
			return false
		}
		return true
	case *json.Object:
		b, ok := b.(*json.Object)
		if !ok {
			return false
		}
		if len(x.Value) != len(b.Value) {
			return false
		}
		for i, item := range x.Value {
			if !equal(item, b.Find(i.Value)) {
				return false
			}
		}
		return true
	case *json.String:
		b, ok := b.(*json.String)
		if !ok {
			return false
		}
		return x.Value == b.Value
	default:
		return false
	}
}

func uniqueItems(val *json.Array) error {
	for i := range val.Value {
		for j := i + 1; j < len(val.Value); j++ {
			if equal(val.Value[i], val.Value[j]) {
				return fmt.Errorf("all items must be unique, but item %d is equal to item %d", i, j)
			}
		}
	}
	return nil
}
