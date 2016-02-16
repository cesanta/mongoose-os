/*
	Package stringlist implements a custom "stringlist" flag
	It accepts comma separated values and repeated flag occurrence.
	Leading and trailing whitespace will be trimmed.

	Usage:

		var (
			myFlag stringlist.Value
		)

		func init() {
			flag.Var(&myFlag, "myflag", "a list of strings")
		}

	Or:

		var (
			myFlag = stringlist.Flag("myflag", "a list of strings")
		)

		func foo() {
			for _, i = range *myFlag {
				// note: you need to dereference the slice
			}
		}

	If you invoke it with:

		--myflag a,b --myflag c --myflag "c, d"

	myFlag will contain: []string{"a", "b", "c", "d"}
*/
package stringlist

import (
	"flag"
	"strings"
)

// Value implements flag.Value
type Value []string

// Set implements the flag.Value interface
func (v *Value) Set(args string) error {
	tokens := strings.Split(args, ",")
	for _, t := range tokens {
		*v = append(*v, strings.TrimSpace(t))
	}
	return nil
}

func (v *Value) String() string {
	return strings.Join(*v, ",")
}

// Flag returns a stringlist flag
func Flag(name string, usage string) *Value {
	var r Value
	flag.Var(&r, name, usage)
	return &r
}
