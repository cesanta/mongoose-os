package schema

import (
	"bytes"
	"fmt"
	"strconv"
	"strings"

	json "github.com/cesanta/ucl"
)

func unescapeRefToken(s string) (string, error) {
	b := bytes.NewBuffer(nil)
	const (
		Normal = iota
		AfterEscape
	)
	state := Normal
	for _, c := range s {
		switch state {
		case Normal:
			switch c {
			case '~':
				state = AfterEscape
			default:
				b.WriteRune(c)
			}
		case AfterEscape:
			switch c {
			case '0':
				b.WriteRune('~')
				state = Normal
			case '1':
				b.WriteRune('/')
				state = Normal
			default:
				return "", fmt.Errorf("invalid escape %q in token %q", string(c), s)
			}
		}
	}
	if state != Normal {
		return "", fmt.Errorf("token %q ends with '~'", s)
	}
	return b.String(), nil
}

func resolveRef(v json.Value, ref string) (json.Value, error) {
	if ref == "" {
		return v, nil
	}
	if ref[0] != '/' {
		return nil, fmt.Errorf("%q does not start with '/'", ref)
	}
	tokens := strings.Split(ref[1:], "/")
	for i, tok := range tokens {
		t, err := unescapeRefToken(tok)
		if err != nil {
			return nil, err
		}
		switch val := v.(type) {
		case *json.Array:
			index, err := strconv.ParseUint(t, 10, 0)
			if err != nil {
				return nil, err
			}
			if int(index) >= len(val.Value) {
				return nil, fmt.Errorf("index %d is out of bounds of %q", index, "/"+strings.Join(tokens[:i], "/"))
			}
			v = val.Value[index]
		case *json.Object:
			p, ok := val.Lookup(t)
			if !ok {
				return nil, fmt.Errorf("%q does not have property %q", "/"+strings.Join(tokens[:i], "/"), t)
			}
			v = p
		default:
			return nil, fmt.Errorf("%q is not and array or object", "/"+strings.Join(tokens[:i], "/"))
		}
	}
	return v, nil
}
