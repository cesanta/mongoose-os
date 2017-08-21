// Copyright (c) 2014-2017 Cesanta Software Limited
// All rights reserved

package moscommon

import "unicode"

const (
	BuildTargetDefault = "all"
)

func IdentifierFromString(name string) string {
	ret := ""
	for _, c := range name {
		if !(unicode.IsLetter(c) || unicode.IsDigit(c)) {
			c = '_'
		}
		ret += string(c)
	}
	return ret
}
