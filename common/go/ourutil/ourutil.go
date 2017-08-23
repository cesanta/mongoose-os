package ourutil

import (
	"bufio"
	"fmt"
	"io"
	"io/ioutil"
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

type CmdOutMode int

const (
	CmdOutNever CmdOutMode = iota
	CmdOutAlways
	CmdOutOnError
)

// RunCmd prints the command it's about to execute, and executes it, with
// stdout and stderr set to those of the current process.
func RunCmd(outMode CmdOutMode, args ...string) error {
	Reportf("Running %s", strings.Join(args, " "))

	cmd := exec.Command(args[0], args[1:]...)
	var so, se io.ReadCloser
	switch outMode {
	case CmdOutNever:
		// Nothing
	case CmdOutAlways:
		cmd.Stdout = os.Stdout
		cmd.Stderr = os.Stderr
	case CmdOutOnError:
		so, _ = cmd.StdoutPipe()
		se, _ = cmd.StderrPipe()
	}

	if err := cmd.Start(); err != nil {
		return errors.Trace(err)
	}

	var soData, seData []byte
	if so != nil && se != nil {
		soData, _ = ioutil.ReadAll(so)
		seData, _ = ioutil.ReadAll(se)
	}

	if err := cmd.Wait(); err != nil {
		if so != nil && se != nil {
			os.Stdout.Write(soData)
			os.Stderr.Write(seData)
		}
		return errors.Trace(err)
	}

	return nil
}
