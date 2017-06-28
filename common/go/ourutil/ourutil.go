package ourutil

import (
	"bufio"
	"fmt"
	"io"
	"os"
	"strings"

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
