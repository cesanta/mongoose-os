// Copyright (c) 2014-2017 Cesanta Software Limited
// All rights reserved

package ourgit

import (
	"encoding/hex"
	"fmt"
	"io/ioutil"
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

func (m *ourGitGoGit) Fetch(localDir string, opts FetchOptions) error {
	repo, err := git.PlainOpen(localDir)
	if err != nil {
		return errors.Annotatef(err, "failed to open repo %s", localDir)
	}

	err = repo.Fetch(&git.FetchOptions{
		Tags:  git.AllTags,
		Depth: opts.Depth,
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

func (m *ourGitGoGit) Clone(srcURL, localDir string, opts CloneOptions) error {
	// Check if the dir existed before we try to do the clone
	existed := false
	if _, err := os.Stat(localDir); !os.IsNotExist(err) {
		files, err := ioutil.ReadDir(localDir)
		if err != nil {
			return errors.Trace(err)
		}

		existed = len(files) > 0
	}

	if opts.ReferenceDir != "" {
		return errors.Errorf("ReferenceDir is not implemented for go-git impl")
	}

	goGitOpts := []git.CloneOptions{
		git.CloneOptions{
			URL:   srcURL,
			Depth: opts.Depth,
			Tags:  git.TagFollowing,
		},
	}

	// If depth is non-zero, also assume we should only clone a single branch
	if opts.Depth != 0 {
		goGitOpts[0].Depth = opts.Depth
		goGitOpts[0].SingleBranch = true
	}

	if opts.Ref != "" {
		// We asked to clone at the certain ref instead of master. Unfortunately
		// there's no way (that I know of) in go-git to specify a name of a branch
		// OR a name of tag OR a hash, so we have to try all of the three
		// separately.
		goGitOpts = append(goGitOpts, goGitOpts[0], goGitOpts[0])
		goGitOpts[0].ReferenceName = plumbing.ReferenceName("refs/heads/" + opts.Ref)
		goGitOpts[1].ReferenceName = plumbing.ReferenceName("refs/tags/" + opts.Ref)
		goGitOpts[2].ReferenceName = "" // TODO(dfrank): use hash
	}

	// Do the clone. If opts.Ref was empty, goGitOpts contains just a single
	// element, so there will be just one iteration of the loop. If opts.Ref was
	// non-empty, there will be up to 3 iterations (try branch, try tag, try
	// hash)
	var err error
	for _, o := range goGitOpts {
		if !existed {
			os.RemoveAll(localDir)
		}
		_, err = git.PlainClone(localDir, false, &o)
		if err == nil {
			break
		}
	}

	if err != nil {
		return errors.Annotatef(err, "cloning %q to %q", srcURL, localDir)
	}

	return nil
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
	b, err := hex.DecodeString(s)
	if err != nil {
		return plumbing.Hash{}, errors.Annotatef(err, "trying to interpret %q as a git hash", s)
	}

	// TODO(dfrank): at the moment (10/11/2017) git-go doesn't support partial
	// hashes; hopefully it will be fixed in the future.
	if len(s) != fullHashLen {
		return plumbing.Hash{}, errors.Errorf(
			"partial git hashes are not supported (given: %s), hash should have exactly %d characters",
			s, fullHashLen,
		)
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
