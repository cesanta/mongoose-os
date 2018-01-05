// Copyright (c) 2014-2017 Cesanta Software Limited
// All rights reserved

// This package is helpful when one needs to get a pointer to a literal value
package lptr

// String is a helper routine that allocates a new string value to store v and returns a pointer to it.
func String(v string) *string {
	return &v
}

// Int is a helper routine that allocates a new int value to store v and returns a pointer to it.
func Int(v int) *int {
	return &v
}

// Int32 is a helper routine that allocates a new int32 value to store v and returns a pointer to it.
func Int32(v int32) *int32 {
	return &v
}

// Int64 is a helper routine that allocates a new int64 value to store v and returns a pointer to it.
func Int64(v int64) *int64 {
	return &v
}

// Bool is a helper routine that allocates a new bool value to store v and returns a pointer to it.
func Bool(v bool) *bool {
	return &v
}

// Float64 is a helper routine that allocates a new float64 value to store v and returns a pointer to it.
func Float64(f float64) *float64 {
	return &f
}
