package gitutils

import (
	"bytes"
	"os/exec"
	"strings"

	"github.com/cesanta/errors"
)

const (
	minHashLen = 6
)

func GitGetCurrentHash(repo string) (string, error) {
	resp, err := git(repo, "rev-parse", "HEAD")
	if err != nil {
		return "", errors.Annotatef(err, "failed to get current hash")
	}
	if len(resp) == 0 {
		return "", errors.Errorf("failed to get current hash")
	}
	return resp, nil
}

func DoesGitBranchExist(repo string, branch string) (bool, error) {
	resp, err := git(repo, "branch", "--list", branch)
	if err != nil {
		return false, errors.Annotatef(err, "failed to check if branch %q exists", branch)
	}
	return len(resp) > 2 && resp[2:] == branch, nil
}

func DoesGitTagExist(repo string, tag string) (bool, error) {
	resp, err := git(repo, "tag", "--list", tag)
	if err != nil {
		return false, errors.Annotatef(err, "failed to check if tag %q exists", tag)
	}
	return resp == tag, nil
}

func GitGetCurrentAbbrev(repo string) (string, error) {
	resp, err := git(repo, "rev-parse", "--abbrev-ref", "HEAD")
	if err != nil {
		return "", errors.Annotatef(err, "failed to get current git branch")
	}
	return resp, nil
}

func GitGetToplevelDir(repo string) (string, error) {
	resp, err := git(repo, "rev-parse", "--show-toplevel")
	if err != nil {
		return "", errors.Annotatef(err, "failed to get git toplevel dir")
	}
	return resp, nil
}

func GitCheckout(repo string, id string) error {
	_, err := git(repo, "checkout", id)
	if err != nil {
		return errors.Annotatef(err, "failed to git checkout %s", id)
	}
	return nil
}

func GitPull(repo string) error {
	_, err := git(repo, "pull")
	if err != nil {
		return errors.Annotatef(err, "failed to git pull")
	}
	return nil
}

func GitFetch(repo string) error {
	_, err := git(repo, "fetch", "--tags")
	if err != nil {
		return errors.Annotatef(err, "failed to git fetch")
	}
	return nil
}

func GitClone(srcURL, targetDir, referenceDir string) error {
	args := []string{"clone"}
	if referenceDir != "" {
		args = append(args, "--reference", referenceDir)
	}
	args = append(args, srcURL, targetDir)
	cmd := exec.Command("git", args...)

	err := cmd.Run()
	if err != nil {
		return errors.Trace(err)
	}

	return nil
}

func HashesEqual(hash1, hash2 string) bool {
	minLen := len(hash1)
	if len(hash2) < minLen {
		minLen = len(hash2)
	}

	// Check if at least one of the hashes is too short
	if minLen < minHashLen {
		return false
	}

	return hash1[:minLen] == hash2[:minLen]
}

func git(repo string, subcmd string, args ...string) (string, error) {
	cmd := exec.Command("git", append([]string{subcmd}, args...)...)

	var b bytes.Buffer
	cmd.Dir = repo
	cmd.Stdout = &b
	err := cmd.Run()
	if err != nil {
		return "", errors.Trace(err)
	}
	resp := b.String()
	return strings.TrimRight(resp, "\r\n"), nil
}
