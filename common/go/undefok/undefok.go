/*
Package undefok provides -undefok flag which allows you to specify a list of
command-line flags that are safe to ignore if application does not support them.

It does so by checking each if each flag is already registered and adding no-op
string flag for those that are missing. This means that using short form for
boolean flags ('-foo' instead of '-foo=true') for flags listed in -undefok will
break things, so don't do that.

To use it in your application do this:

    func main() {
      undefok.Register(nil)
      flag.Parse()

      ...
    }
*/
package undefok

import (
	"flag"
	"strings"
)

type value struct {
	flagset *flag.FlagSet
}

// Register adds -undefok flag to the given FlagSet. If fs is nil, flag is added
// to default flag.CommandLine flag set.
func Register(fs *flag.FlagSet) {
	// Shortcut.
	if fs == nil {
		fs = flag.CommandLine
	}

	fs.Var(&value{fs}, "undefok", "Comma-separated list of flag that should be not cause errors during command line parsing if application does not define them. Works only if appears before those flags.")
}

func (v *value) String() string {
	return "undefok"
}

func (v *value) Set(s string) error {
	for _, f := range strings.Split(s, ",") {
		f = strings.TrimSpace(f)
		if v.flagset.Lookup(f) == nil {
			v.flagset.String(f, "", "Added by -undefok. Not supported by the application and does nothing.")
		}
	}
	return nil
}
