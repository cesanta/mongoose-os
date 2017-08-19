package gitutils

import (
	"bytes"
	"os/exec"
	"runtime"
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
	_, err := git(repo, "pull", "--all")
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

// IsClean returns true if there are no modified, deleted or untracked files,
// and no non-pushed commits since the given version.
func IsClean(repo, version string) (bool, error) {
	// First, check if there are modified, deleted or untracked files
	resp, err := git(repo, "ls-files", "--exclude-standard", "--modified", "--others", "--deleted")
	if err != nil {
		return false, errors.Annotatef(err, "failed to git ls-files")
	}

	if resp != "" {
		// Working dir is dirty
		return false, nil
	}

	// Unfortunately, git ls-files is unable to show staged and uncommitted files.
	// So, specifically for these files, we'll have to run git diff --cached:

	resp, err = git(repo, "diff", "--cached", "--name-only")
	if err != nil {
		return false, errors.Annotatef(err, "failed to git diff --cached")
	}

	if resp != "" {
		// Working dir is dirty
		return false, nil
	}

	// Working directory is clean, now we need to check if there are some
	// non-pushed commits. Unfortunately there is no way (that I know of) which
	// would work with both branches and tags. So, we do this:
	//
	// Invoke "git cherry". If the repo is on a branch, this command will print
	// list of commits to be pushed to upstream. If, however, the repo is not on
	// a branch (e.g. it's often on a tag), then this command will fail, and in
	// that case we invoke it again, but with the version specified:
	// "git cherry <version>". In either case, non-empty output means the
	// precense of some commits which would not be fetched by the remote builder,
	// so the repo is dirty.

	resp, err = git(repo, "cherry")
	if err != nil {
		// Apparently the repo is not on a branch, retry with the version
		resp, err = git(repo, "cherry", version)
		if err != nil {
			// We can get an error at this point if given version does not exist
			// in the repository; in this case assume the repo is clean
			return true, nil
		}
	}

	if resp != "" {
		// Some commits need to be pushed to upstream
		return false, nil
	}

	// Working dir is clean
	return true, nil
}

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

func GitGetOriginUrl(repo string) (string, error) {
	resp, err := git(repo, "remote", "get-url", "origin")
	if err != nil {
		return "", errors.Annotatef(err, "failed to get origin URL")
	}
	if len(resp) == 0 {
		return "", errors.Errorf("failed to get origin URL")
	}
	return resp, nil
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
