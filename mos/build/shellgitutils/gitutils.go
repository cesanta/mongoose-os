package shellgitutils

import (
	"bytes"
	"os/exec"
	"runtime"
	"strings"

	"github.com/cesanta/errors"
)

func GitClone(srcURL, targetDir, referenceDir string) error {
	args := []string{"clone"}
	if referenceDir != "" {
		args = append(args, "--reference", referenceDir)
	}
	var berr bytes.Buffer
	args = append(args, srcURL, targetDir)
	cmd := exec.Command("git", args...)

	// By default, when the user tries to clone non-existing repo, git will
	// ask for username/password, just in case the repo exists but is private.
	// We want it to just fail in this case, so we're setting env variable
	// GIT_TERMINAL_PROMPT to 0.
	//
	// However, git on linux goes wild in this case:
	//   Error in GnuTLS initialization: Failed to acquire random data.
	//   fatal: unable to access 'https://github.com/mongoose-os-apps/blynk/':
	//   Couldn't resolve host 'github.com
	// So we have to avoid setting GIT_TERMINAL_PROMPT on windows.
	if runtime.GOOS != "windows" {
		cmd.Env = []string{"GIT_TERMINAL_PROMPT=0"}
	}
	cmd.Stderr = &berr

	err := cmd.Run()
	if err != nil {
		return errors.Annotatef(err, "cloning %s: %s", srcURL, berr.String())
	}

	return nil
}

func git(repo string, subcmd string, args ...string) (string, error) {
	cmd := exec.Command("git", append([]string{subcmd}, args...)...)

	var b bytes.Buffer
	var berr bytes.Buffer
	cmd.Dir = repo
	cmd.Stdout = &b
	cmd.Stderr = &berr
	err := cmd.Run()
	if err != nil {
		return "", errors.Annotate(err, berr.String())
	}
	resp := b.String()
	return strings.TrimRight(resp, "\r\n"), nil
}
