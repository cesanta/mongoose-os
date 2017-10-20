// Copyright (c) 2014-2017 Cesanta Software Limited
// All rights reserved

package datamap

import (
	"strings"
)

// GetFailHandler, if specified, is called when DataMap.Get fails to get the
// value normally. This way, client can create some "phantom" values.
type GetFailHandler func(dm *DataMap, name string) (interface{}, bool)

// DataMap is a hierarchical data structure which allows clients to get and
// set values by their JavaScript-like paths, e.g. "foo.bar.baz". It can also
// have get-fail-handler, which is invoked if Get fails to find value at the
// provided path. This way, clients can have some "phantom" values.
type DataMap struct {
	data           map[string]interface{}
	getFailHandler GetFailHandler
}

// New creates new DataMap with the provided GetFailHandler.
func New(getFailHandler GetFailHandler) *DataMap {
	return &DataMap{
		data:           make(map[string]interface{}),
		getFailHandler: getFailHandler,
	}
}

func (dm *DataMap) Copy() *DataMap {
	ret := New(dm.getFailHandler)

	ret.data = deepCopyMap(dm.data)

	return ret
}

func deepCopy(val interface{}) interface{} {
	switch v := val.(type) {
	case map[string]interface{}:
		return deepCopyMap(v)
	case []interface{}:
		return deepCopySlice(v)
	}
	return val
}

func deepCopyMap(m map[string]interface{}) map[string]interface{} {
	ret := map[string]interface{}{}
	for k, v := range m {
		ret[k] = deepCopy(v)
	}
	return ret
}

func deepCopySlice(in []interface{}) []interface{} {
	ret := []interface{}{}
	for _, v := range in {
		ret = append(ret, deepCopy(v))
	}
	return ret
}

// Set sets new value at the provided name (path), like "foo.bar.baz". All
// intermediary non-existing parts will be silently created.
func (dm *DataMap) Set(name string, value interface{}) {
	// Note: we opted to use ${foo} instead of {{foo}}, because {{foo}} needs to
	// be quoted in yaml, whereas ${foo} does not.
	m, key := getMapKey(name, dm.data, true)
	m[key] = value
}

// Get gets value at the provided name (path), like "foo.bar.baz". If value is
// not found, returned bool is false.
func (dm *DataMap) Get(name string) (interface{}, bool) {
	m, key := getMapKey(name, dm.data, false)
	if m == nil {
		// Non-existing var, resort to getFailHandler if it exists; otherwise
		// return nil
		if dm.getFailHandler != nil {
			return dm.getFailHandler(dm, name)
		}
		return nil, false
	}

	return m[key], true
}

// getMapKey is a helper function; it takes a path and returns a map and a key
// to which the path corresponds.
//
// We can't take address of a map element in Go, so, this function cannot
// handle empty paths. If empty path makes sense for the caller, then the
// caller should have a special case for it.
func getMapKey(
	path string, data map[string]interface{}, addMissing bool,
) (m map[string]interface{}, key string) {
	parts := strings.SplitN(path, ".", 2)

	val, ok := data[parts[0]]
	if !ok {
		if addMissing {
			if len(parts) == 2 {
				val = map[string]interface{}{}
				data[parts[0]] = val
			} else {
				// We'll return data, parts[0] below
			}
		} else {
			return nil, ""
		}
	}

	if len(parts) == 1 {
		return data, parts[0]
	} else {
		valMap, ok := val.(map[string]interface{})
		if !ok {
			return nil, ""
		}
		return getMapKey(parts[1], valMap, addMissing)
	}
}
