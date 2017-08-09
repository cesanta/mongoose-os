package ourutil

import (
	"bufio"
	"fmt"
	"io"
	"os"
	"os/exec"
	"strings"

	"github.com/cesanta/errors"
	"github.com/golang/glog"
)

func Reportf(f string, args ...interface{}) {
	fmt.Fprintf(os.Stderr, f+"\n", args...)
	glog.Infof(f, args...)
}

func Freportf(logFile io.Writer, f string, args ...interface{}) {
	fmt.Fprintf(logFile, f+"\n", args...)
	glog.Infof(f, args...)
}

func Prompt(text string) string {
	fmt.Fprintf(os.Stderr, "%s ", text)
	ans, _ := bufio.NewReader(os.Stdin).ReadString('\n')
	return strings.TrimSpace(ans)
}

// RunCmd prints the command it's about to execute, and executes it, with
// stdout and stderr set to those of the current process.
func RunCmd(args ...string) error {
	Reportf("Running %s", strings.Join(args, " "))

	cmd := exec.Command(args[0], args[1:]...)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	if err := cmd.Run(); err != nil {
		return errors.Trace(err)
	}

	return nil
}
