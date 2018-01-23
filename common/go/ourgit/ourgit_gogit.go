// Copyright (c) 2014-2017 Cesanta Software Limited
// All rights reserved

package ourgit

import (
	"encoding/hex"
	"fmt"
	"os"
	"path/filepath"
	"strings"

	moscommon "cesanta.com/mos/common"
	"github.com/cesanta/errors"
	"github.com/golang/glog"
	git "gopkg.in/src-d/go-git.v4"
	"gopkg.in/src-d/go-git.v4/plumbing"
	"gopkg.in/src-d/go-git.v4/plumbing/storer"
)

type ourGitGoGit struct{}

func (m *ourGitGoGit) GetCurrentHash(localDir string) (string, error) {
	repo, err := git.PlainOpen(localDir)
	if err != nil {
		return "", errors.Trace(err)
	}

	head, err := repo.Head()
	if err != nil {
		return "", errors.Trace(err)
	}

	return head.Hash().String(), nil
}

func doesRefExist(iter storer.ReferenceIter, name string) (bool, error) {
	exists := false

	err := iter.ForEach(func(branch *plumbing.Reference) error {
		if branch.Name().Short() == name {
			exists = true
		}
		return nil
	})
	if err != nil {
		return false, errors.Trace(err)
	}

	return exists, nil
}

func (m *ourGitGoGit) DoesBranchExist(localDir string, branchName string) (bool, error) {
	repo, err := git.PlainOpen(localDir)
	if err != nil {
		return false, errors.Trace(err)
	}

	branches, err := repo.Branches()
	if err != nil {
		return false, errors.Trace(err)
	}

	exists, err := doesRefExist(branches, branchName)
	if err != nil {
		return false, errors.Trace(err)
	}

	return exists, nil
}

func (m *ourGitGoGit) DoesTagExist(localDir string, tagName string) (bool, error) {
	repo, err := git.PlainOpen(localDir)
	if err != nil {
		return false, errors.Trace(err)
	}

	tags, err := repo.Tags()
	if err != nil {
		return false, errors.Trace(err)
	}

	exists, err := doesRefExist(tags, tagName)
	if err != nil {
		return false, errors.Trace(err)
	}

	return exists, nil
}

func (m *ourGitGoGit) GetToplevelDir(localDir string) (string, error) {
	localDir, err := filepath.Abs(localDir)
	if err != nil {
		return "", errors.Trace(err)
	}

	for localDir != "" {
		if _, err := os.Stat(filepath.Join(localDir, ".git")); err == nil {
			return localDir, nil
		}

		localDirNew, err := filepath.Abs(filepath.Join(localDir, ".."))
		if err != nil {
			return "", errors.Trace(err)
		}

		if localDirNew == localDir {
			return "", nil
		}

		localDir = localDirNew
	}

	return localDir, nil
}

func (m *ourGitGoGit) Checkout(localDir string, id string, refType RefType) error {
	repo, err := git.PlainOpen(localDir)
	if err != nil {
		return errors.Trace(err)
	}

	wt, err := repo.Worktree()
	if err != nil {
		return errors.Trace(err)
	}

	switch refType {
	case RefTypeBranch:
		err = wt.Checkout(&git.CheckoutOptions{
			Branch: plumbing.ReferenceName("refs/heads/" + id),
		})

	case RefTypeTag:
		err = wt.Checkout(&git.CheckoutOptions{
			Branch: plumbing.ReferenceName("refs/tags/" + id),
		})

	case RefTypeHash:
		var hash plumbing.Hash
		hash, err = newHashSafe(id)
		if err == nil {
			err = wt.Checkout(&git.CheckoutOptions{
				Hash: hash,
			})
		}
	}
	if err != nil {
		return errors.Annotatef(err, "checking out a %s %q in %q", refType, id, localDir)
	}

	return nil
}

func (m *ourGitGoGit) ResetHard(localDir string) error {
	repo, err := git.PlainOpen(localDir)
	if err != nil {
		return errors.Trace(err)
	}

	wt, err := repo.Worktree()
	if err != nil {
		return errors.Trace(err)
	}

	err = wt.Reset(&git.ResetOptions{
		Mode: git.HardReset,
	})
	if err != nil {
		return errors.Trace(err)
	}

	return nil
}

func (m *ourGitGoGit) Pull(localDir string) error {
	glog.Infof("Pulling %s", localDir)

	repo, err := git.PlainOpen(localDir)
	if err != nil {
		return errors.Trace(err)
	}

	wt, err := repo.Worktree()
	if err != nil {
		return errors.Trace(err)
	}

	err = wt.Pull(&git.PullOptions{})
	if err != nil && errors.Cause(err) != git.NoErrAlreadyUpToDate {
		return errors.Trace(err)
	}

	return nil
}

func (m *ourGitGoGit) Fetch(localDir string) error {
	repo, err := git.PlainOpen(localDir)
	if err != nil {
		return errors.Annotatef(err, "failed to open repo %s", localDir)
	}

	err = repo.Fetch(&git.FetchOptions{
		Tags: git.AllTags,
	})
	if err != nil && errors.Cause(err) != git.NoErrAlreadyUpToDate {
		return errors.Annotatef(err, "failed to git fetch %s", localDir)
	}

	return nil
}

// IsClean returns true if there are no modified, deleted or untracked files,
// and no non-pushed commits since the given version.
func (m *ourGitGoGit) IsClean(localDir, version string) (bool, error) {
	repo, err := git.PlainOpen(localDir)
	if err != nil {
		return false, errors.Trace(err)
	}

	wt, err := repo.Worktree()
	if err != nil {
		return false, errors.Trace(err)
	}

	status, err := wt.Status()
	if err != nil {
		return false, errors.Trace(err)
	}

	return isCleanWithLib(status), nil
}

func (m *ourGitGoGit) Clone(srcURL, localDir string) error {
	_, err := git.PlainClone(localDir, false, &git.CloneOptions{
		URL: srcURL,
	})
	if err != nil {
		return errors.Annotatef(err, "cloning %q to %q", srcURL, localDir)
	}

	return nil
}

func (m *ourGitGoGit) CloneReferenced(srcURL, targetDir, referenceDir string) error {
	return errors.Errorf("not implemented")
}

func (m *ourGitGoGit) GetOriginUrl(localDir string) (string, error) {
	repo, err := git.PlainOpen(localDir)
	if err != nil {
		return "", errors.Trace(err)
	}

	remotes, err := repo.Remotes()
	if err != nil {
		return "", errors.Trace(err)
	}

	for _, r := range remotes {
		if r.Config().Name == "origin" {
			return r.Config().URLs[0], nil
		}
	}

	return "", errors.Errorf("failed to get origin URL")
}

// NewHash return a new Hash from a hexadecimal hash representation
func newHashSafe(s string) (plumbing.Hash, error) {
	// TODO(dfrank): at the moment (10/11/2017) git-go doesn't support partial
	// hashes; hopefully it will be fixed in the future.
	if len(s) != fullHashLen {
		return plumbing.Hash{}, errors.Errorf(
			"partial git hashes are not supported, hash should have exactly %d characters",
			fullHashLen,
		)
	}

	b, err := hex.DecodeString(s)
	if err != nil {
		return plumbing.Hash{}, errors.Trace(err)
	}

	var h plumbing.Hash
	copy(h[:], b)

	return h, nil
}

// isCleanWithLib is like s.IsClean(), but ignores binary libs (i.e. files
// under the dir returned by moscommon.GetBinaryLibsDir())
func isCleanWithLib(s git.Status) bool {
	for n, status := range s {
		if strings.HasPrefix(n, fmt.Sprintf("%s%c", moscommon.GetBinaryLibsDir(""), filepath.Separator)) {
			continue
		}
		if status.Worktree != git.Unmodified || status.Staging != git.Unmodified {
			return false
		}
	}

	return true
}
