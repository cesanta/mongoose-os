package dev

import (
	"encoding/json"
	"strconv"
	"strings"

	"github.com/cesanta/errors"
)

// DevConf represents configuration of a device
type DevConf struct {
	data map[string]interface{}
}

// Get takes a path like "wifi.sta.ssid" and tries to get config value at the
// given path.
func (c *DevConf) Get(path string) (string, error) {
	var v interface{}
	if path != "" {
		m, key := getMapKey(path, c.data)
		if m == nil {
			return "", errors.Errorf("no config value at path %q", path)
		}
		v = m[key]
	} else {
		// We have to special-case empty path since getMapKey returns map and a
		// key, since we cannot take address of a map element.
		v = c.data
	}
	switch v.(type) {
	case string:
		return v.(string), nil
	case float64:
		return strconv.FormatFloat(v.(float64), 'f', -1, 64), nil
	case json.Number:
		return v.(json.Number).String(), nil
	case bool:
		if v.(bool) {
			return "true", nil
		} else {
			return "false", nil
		}
	case map[string]interface{}:
		bytes, err := json.MarshalIndent(v, "", "  ")
		return string(bytes), err
	default:
		return "", errors.Errorf("unknown value type: %T", v)
	}
}

// Set takes a path like "wifi.sta.ssid" and a value, and tries to set config
// value at the given path. Value is always a string, but if given path refers
// to a number or a boolean, then the given value string will be converted to
// the appropriate type.
func (c *DevConf) Set(path, value string) error {
	m, key := getMapKey(path, c.data)
	if m == nil {
		return errors.Errorf("no config value at path %q", path)
	}

	switch m[key].(type) {
	case string:
		m[key] = value
	case float64:
		valueFloat, err := strconv.ParseFloat(value, 64)
		if err != nil {
			return errors.Trace(err)
		}
		m[key] = valueFloat
	case json.Number:
		_, err := strconv.ParseFloat(value, 64)
		if err != nil {
			return errors.Trace(err)
		}
		m[key] = json.Number(value)
	case bool:
		if value == "true" {
			m[key] = true
			return nil
		} else if value == "false" {
			m[key] = false
			return nil
		} else {
			return errors.Errorf("can't convert %q to a boolean", value)
		}
	case map[string]interface{}:
		return errors.Errorf("only strings, numbers and booleans can be set, but path %q refers to an object", path)
	default:
		return errors.Errorf("unknown value type: %T", m[key])
	}

	return nil
}

// getMapKey is a helper function used by both Get and Set; it takes a path and
// returns a map and a key to which the path corresponds.
//
// We can't take address of a map element in Go, so, this function cannot
// handle empty paths. If empty path makes sense for the caller, then the
// caller should have a special case for it.
func getMapKey(path string, data map[string]interface{}) (m map[string]interface{}, key string) {
	parts := strings.SplitN(path, ".", 2)

	val, ok := data[parts[0]]
	if !ok {
		return nil, ""
	}

	if len(parts) == 1 {
		return data, parts[0]
	} else {
		valMap, ok := val.(map[string]interface{})
		if !ok {
			return nil, ""
		}
		return getMapKey(parts[1], valMap)
	}
}
