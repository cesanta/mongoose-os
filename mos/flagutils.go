package main

import (
	goflag "flag"
	"fmt"
	"io"
	"os"
	"text/tabwriter"

	"cesanta.com/common/go/multierror"

	"github.com/cesanta/errors"
	flag "github.com/spf13/pflag"
)

var (
	hiddenFlags = []string{
		"alsologtostderr",
		"clubby.default_frame_version",
		"clubby.xxx_ubjson_input",
		"log_backtrace_at",
		"log_dir",
		"logbufsecs",
		"logtostderr",
		"stderrthreshold",
		"v",
		"vmodule",
	}
)

func initFlags() {
	initATCAFlags()
	flag.CommandLine.AddGoFlagSet(goflag.CommandLine)
	hideFlags()
	flag.Usage = usage
}

func hideFlags() {
	for _, f := range hiddenFlags {
		flag.CommandLine.MarkHidden(f)
	}
}

func unhideFlags() {
	for _, f := range hiddenFlags {
		f := flag.Lookup(f)
		if f != nil {
			f.Hidden = false
		}
	}
}

func checkFlags(fs []string) error {
	var errs error
	for _, req := range fs {
		f := flag.Lookup(req)
		if f == nil || !f.Changed {
			errs = multierror.Append(errs, errors.Errorf("--%s is required\t\t%s", f.Name, f.Usage))
		}
	}
	return errors.Trace(errs)
}

func printFlag(w io.Writer, opt string, name string) {
	f := flag.Lookup(name)
	arg := "<string>"
	if f.Value.Type() == "bool" {
		arg = ""
	}
	fmt.Fprintf(w, "  --%s %s\t%s. %s, default value: %q\n", name, arg, f.Usage, opt, f.DefValue)
}

func usage() {
	w := tabwriter.NewWriter(os.Stderr, 0, 0, 1, ' ', 0)

	if len(os.Args) == 3 && os.Args[1] == "help" {
		for _, c := range commands {
			if c.name == os.Args[2] {
				fmt.Fprintf(w, "%s %s FLAGS\n", os.Args[0], os.Args[2])
				fmt.Fprintf(w, "\nFlags:\n")
				for _, name := range c.required {
					printFlag(w, "Required", name)
				}
				for _, name := range c.optional {
					printFlag(w, "Optional", name)
				}
				w.Flush()
				os.Exit(1)
			}
		}
	}

	fmt.Fprintf(w, "The Mongoose OS command line tool, v. %s. Usage:\n", BuildId)
	fmt.Fprintf(w, "  %s <command>\n", os.Args[0])
	fmt.Fprintf(w, "\nCommands:\n")

	for _, c := range commands {
		fmt.Fprintf(w, "  %s\t\t%s\n", c.name, c.short)
	}

	fmt.Fprintf(w, "\nGlobal Flags:\n")
	if *helpFull {
		fmt.Fprintf(w, flag.CommandLine.FlagUsages())
	} else {
		printFlag(w, "Optional", "verbose")
		printFlag(w, "Optional", "logtostderr")
	}

	w.Flush()
}
