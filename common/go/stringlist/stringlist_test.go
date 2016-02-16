package stringlist

import (
	"flag"
	"testing"
)

func TestValue(t *testing.T) {
	var sl Value

	fs := flag.NewFlagSet("test", flag.ContinueOnError)
	fs.Var(&sl, "dummy", "test")

	inputs := [][]string{
		{"--dummy=foo,bar"},
		{"--dummy", "foo,bar"},
		{"--dummy", "foo, bar"},
		{"--dummy", "foo", "--dummy", "bar"},
	}
	for _, i := range inputs {
		sl = nil

		err := fs.Parse(i)
		if err != nil {
			t.Error(err)
		}
		if want, got := 2, len(sl); want != got {
			t.Errorf("Parsing %q len: want %d, got %d", i, want, got)
		}

		want := []string{"foo", "bar"}
		eq := true
		for i := range sl {
			if sl[i] != want[i] {
				eq = false
				break
			}
		}
		if !eq {
			t.Errorf("Parsing %q: want %#v, got %#v", i, want, sl)
		}
	}
}
