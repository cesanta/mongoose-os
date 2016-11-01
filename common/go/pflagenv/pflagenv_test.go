package pflagenv

import (
	"os"
	"testing"

	"github.com/spf13/pflag"
)

func TestParseFlagSet(t *testing.T) {
	fs := pflag.NewFlagSet("pflagenv-test", pflag.ContinueOnError)

	var myFlag1, myFlag2, myFlag3, myFlag4 string
	fs.StringVar(&myFlag1, "my-flag1", "def1", "")
	fs.StringVar(&myFlag2, "my-flag2", "def2", "")
	fs.StringVar(&myFlag3, "my-flag3", "def3", "")
	fs.StringVar(&myFlag4, "my-flag4", "def4", "")
	fs.Parse([]string{"--my-flag1=cl1", "--my-flag2="})

	os.Setenv("TEST_MY_FLAG1", "env1")
	os.Setenv("TEST_MY_FLAG2", "env2")
	os.Setenv("TEST_MY_FLAG3", "env3")
	ParseFlagSet(fs, "TEST_")

	if got, want := myFlag1, "cl1"; got != want {
		t.Errorf("got: %q, want: %q", got, want)
	}

	if got, want := myFlag2, ""; got != want {
		t.Errorf("got: %q, want: %q", got, want)
	}

	if got, want := myFlag3, "env3"; got != want {
		t.Errorf("got: %q, want: %q", got, want)
	}

	if got, want := myFlag4, "def4"; got != want {
		t.Errorf("got: %q, want: %q", got, want)
	}
}
