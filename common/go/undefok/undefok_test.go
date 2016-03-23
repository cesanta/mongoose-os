package undefok

import (
	"flag"
	"testing"
)

func TestUndefok(t *testing.T) {
	fs := flag.NewFlagSet("test", flag.PanicOnError)
	fs.String("foo", "", "Foo.")
	Register(fs)
	err := fs.Parse([]string{"-foo", "foo", "-undefok", "bar", "-bar", "bar"})
	if err != nil {
		t.Fatalf("Parse failed: %s", err)
	}
}
