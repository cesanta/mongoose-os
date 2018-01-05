// Package ucl implements parser and serializer for UCL (https://rspamd.com/doc/configuration/ucl.html).
// Currently it supports only plain JSON, but I'm working on that :)
package ucl

import (
	"bufio"
	"fmt"
	"io"
	"sort"
	"strings"
)

//go:generate ragel -Z ucl.rl

// FormatConfig represents the formatter configuration that affects how
// data is layed out in the output file.
type FormatConfig struct {
	// Indent is a string to use for each indentation level. If not set defaults to 2 spaces.
	Indent string `json:",omitempty"`
	// MultilineObjectThreshold is the maximum length for an object to be written out on a single line.
	// Default is 0, meaning that any non-empty object will be written out with key-value pairs on separate lines.
	MultilineObjectThreshold int `json:",omitempty"`
	// MultilineObjectThreshold is the maximum length for an array to be written out on a single line.
	// Default is 0, meaning that any non-empty array will be written out with items on separate lines.
	MultilineArrayThreshold int `json:",omitempty"`
	// PreserveObjectKeysOrder must be set to true if you want to keep keys in the same order as in the input.
	// By default keys are sorted in lexicographical order.
	PreserveObjectKeysOrder bool `json:",omitempty"`
	placeholder             struct{}
}

// Value represents a UCL value.
type Value interface {
	// String returns simple string representation of the value.
	String() string
	// format returns a properly formatted representation of the value. Used internally
	// by Format function.
	format(indent string, config *FormatConfig) string
}

// Format writes v, formatted according to c, to w.
func Format(v Value, c *FormatConfig, w io.Writer) error {
	if c == nil {
		c = &FormatConfig{} // keep this empty, zero values for options should mean default format.
	}
	_, err := w.Write([]byte(v.format("", c)))
	return err
}

// Parse reads UTF-8 text from r and parses it.
func Parse(r io.Reader) (Value, error) {
	rr := bufio.NewReader(r)
	data := []rune{}
	for {
		c, _, err := rr.ReadRune()
		if err == io.EOF {
			break
		}
		if err != nil {
			return nil, err
		}
		data = append(data, c)
	}
	return parse(data)
}

func parse(data []rune) (Value, error) {
	v, _, err := parse_json(data)
	if err != nil {
		return nil, err
	}
	return v, nil
}

// Null represents "null" JSON value.
type Null struct{}

func (Null) String() string {
	return "null"
}

func (Null) format(indent string, config *FormatConfig) string {
	return "null"
}

// Bool represents boolean value.
type Bool struct {
	Value bool
}

func (v Bool) String() string {
	if v.Value {
		return "true"
	}
	return "false"
}

func (v Bool) format(indent string, config *FormatConfig) string {
	return v.String()
}

// Number represents a numerical value.
type Number struct {
	Value float64
}

func (v Number) String() string {
	return fmt.Sprintf("%g", v.Value)
}

func (v Number) format(indent string, config *FormatConfig) string {
	return v.String()
}

// Integer represents an integer numerical value.
type Integer struct {
	Value int64
}

func (v Integer) String() string {
	return fmt.Sprintf("%d", v.Value)
}

func (v Integer) format(indent string, config *FormatConfig) string {
	return v.String()
}

// String represents a string value.
type String struct {
	Value string
}

func (v String) String() string {
	return fmt.Sprintf("\"%s\"", jsonEscape(v.Value))
}

func (v String) format(indent string, config *FormatConfig) string {
	return v.String()
}

// Array represents an array.
type Array struct {
	Value []Value
}

func (v Array) String() string {
	t := make([]string, len(v.Value))
	for i, item := range v.Value {
		t[i] = item.String()
	}
	return "[" + strings.Join(t, ",") + "]"
}

func (v Array) format(indent string, config *FormatConfig) string {
	if len(v.Value) == 0 {
		return "[]"
	}
	newIndent := config.Indent
	if newIndent == "" {
		newIndent = "  "
	}
	items := make([]string, len(v.Value))
	for i, item := range v.Value {
		items[i] = item.format(indent+newIndent, config)
	}
	shortFormatLen := 2                    // brackets
	shortFormatLen += (len(items) - 1) * 2 // ", " between items
	for _, item := range items {
		if strings.IndexRune(item, '\n') >= 0 {
			// One of the items spans multiple lines, bail out.
			shortFormatLen = config.MultilineArrayThreshold + 1
			break
		}
		shortFormatLen += len(item)
		if shortFormatLen > config.MultilineArrayThreshold {
			break
		}
	}
	if shortFormatLen <= config.MultilineArrayThreshold {
		return "[" + strings.Join(items, ", ") + "]"
	}
	r := "[\n"
	for i := 0; i < len(items)-1; i++ {
		r += indent + newIndent + items[i] + ",\n"
	}
	r += indent + newIndent + items[len(items)-1] + "\n"
	r += indent + "]"
	return r
}

// Key represents keys in objects.
type Key struct {
	Value string
	Index int
}

func (v Key) String() string {
	return fmt.Sprintf("\"%s\"", jsonEscape(v.Value))
}

func (v Key) format(indent string, config *FormatConfig) string {
	return v.String()
}

type byValue []Key

func (s byValue) Len() int           { return len(s) }
func (s byValue) Swap(i, j int)      { s[i], s[j] = s[j], s[i] }
func (s byValue) Less(i, j int) bool { return s[i].Value < s[j].Value }

type byIndex []Key

func (s byIndex) Len() int           { return len(s) }
func (s byIndex) Swap(i, j int)      { s[i], s[j] = s[j], s[i] }
func (s byIndex) Less(i, j int) bool { return s[i].Index < s[j].Index }

// Object represents an object.
type Object struct {
	Value map[Key]Value
}

func (v Object) String() string {
	t := make([]string, 0, len(v.Value))
	for key, item := range v.Value {
		t = append(t, key.String()+":"+item.String())
	}
	return "{" + strings.Join(t, ",") + "}"
}

func (v Object) Find(key string) Value {
	val, _ := v.Lookup(key)
	return val
}

func (v Object) Lookup(key string) (Value, bool) {
	for k, v := range v.Value {
		if k.Value == key {
			return v, true
		}
	}
	return nil, false
}

func (v Object) format(indent string, config *FormatConfig) string {
	if len(v.Value) == 0 {
		return "{}"
	}
	newIndent := config.Indent
	if newIndent == "" {
		newIndent = "  "
	}
	// Make sure that order of properties is stable.
	keys := make([]Key, 0, len(v.Value))
	for k := range v.Value {
		keys = append(keys, k)
	}
	if config.PreserveObjectKeysOrder {
		sort.Sort(byIndex(keys))
	} else {
		sort.Sort(byValue(keys))
	}

	items := make([]string, len(keys))
	for i, k := range keys {
		items[i] = k.format(indent+newIndent, config) + ": " + v.Value[k].format(indent+newIndent, config)
	}
	shortFormatLen := 2                    // brackets
	shortFormatLen += (len(items) - 1) * 2 // ", " between items
	for _, item := range items {
		if strings.IndexRune(item, '\n') >= 0 {
			// One of the items spans multiple lines, bail out.
			shortFormatLen = config.MultilineObjectThreshold + 1
			break
		}
		shortFormatLen += len(item)
		if shortFormatLen > config.MultilineObjectThreshold {
			break
		}
	}
	if shortFormatLen <= config.MultilineObjectThreshold {
		return "{" + strings.Join(items, ", ") + "}"
	}

	r := "{\n"
	for i := 0; i < len(keys)-1; i++ {
		r += indent + newIndent + keys[i].format(indent+newIndent, config) + ": " + v.Value[keys[i]].format(indent+newIndent, config) + ",\n"
	}
	r += indent + newIndent + keys[len(keys)-1].format(indent+newIndent, config) + ": " + v.Value[keys[len(keys)-1]].format(indent+newIndent, config) + "\n"
	r += indent + "}"
	return r
}
