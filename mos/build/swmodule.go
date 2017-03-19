package build

import (
	"fmt"
	"io"
	"io/ioutil"
	"net/url"
	"os"
	"path"
	"path/filepath"
	"strings"

	"github.com/cesanta/errors"
	"github.com/golang/glog"

	"cesanta.com/mos/build/gitutils"
)

type SWModule struct {
	Type    string `yaml:"type,omitempty"`
	Origin  string `yaml:"origin,omitempty"`
	Version string `yaml:"version,omitempty"`
	Name    string `yaml:"name,omitempty"`
}

type SWModuleType int

const (
	SWModuleTypeNone SWModuleType = iota
	SWModuleTypeGit
)

func (m *SWModule) GetName() (string, error) {
	if m.Name != "" {
		return m.Name, nil
	}

	switch m.GetType() {
	case SWModuleTypeGit:
		// Take last path fragment
		u, err := url.Parse(m.Origin)
		if err != nil {
			return "", errors.Trace(err)
		}

		parts := strings.Split(u.Path, "/")
		if len(parts) == 0 {
			return "", errors.Errorf("path is empty in the URL %q", u.Path)
		}

		return parts[len(parts)-1], nil
	default:
		return "", errors.Errorf("name is not specified, and swmodule type is unknown")
	}
}

func (m *SWModule) GetType() SWModuleType {
	stype := m.Type

	if stype == "" {
		u, err := url.Parse(m.Origin)
		if err != nil {
			return SWModuleTypeNone
		}

		switch u.Host {
		case "github.com":
			stype = "git"
		}
	}

	switch stype {
	case "git":
		return SWModuleTypeGit
	default:
		return SWModuleTypeNone
	}

	return SWModuleTypeNone
}

func (m *SWModule) PrepareLocalCopy(
	targetDir string, logFile io.Writer, deleteIfFailed bool,
) error {
	switch m.GetType() {
	case SWModuleTypeGit:
		return m.prepareLocalCopyGit(targetDir, logFile, deleteIfFailed)
	default:
		return errors.Errorf("unknown swmodule type")
	}
}

func (m *SWModule) prepareLocalCopyGit(
	targetDir string, logFile io.Writer, deleteIfFailed bool,
) error {
	version := m.Version
	if version == "" {
		version = "master"
	}

	// Check if we should clone or pull git repo inside of targetDir.
	// Valid cases are:
	//
	// - it does not exist: it will be cloned
	// - it exists, and is empty: it will be cloned
	// - it exists, and is a git repo: it will be pulled
	//
	// All other cases are considered as an error.
	mgosRepoExists := false
	if _, err := os.Stat(targetDir); err == nil {
		// targetDir exists; let's see if it's a git repo
		if _, err := os.Stat(filepath.Join(targetDir, ".git")); err == nil {
			// Yes it is a git repo
			mgosRepoExists = true
		} else {
			// No it's not a git repo; let's see if it's empty; if not, it's an error.
			files, err := ioutil.ReadDir(targetDir)
			if err != nil {
				return errors.Trace(err)
			}
			if len(files) > 0 {
				return errors.Errorf(
					"%q is not empty, but is not a git repository either", targetDir,
				)
			}
		}
	} else if !os.IsNotExist(err) {
		// Some error other than non-existing dir
		return errors.Trace(err)
	}

	if !mgosRepoExists {
		fmt.Printf("Repository %q does not exist, cloning...\n", targetDir)
		err := gitutils.GitClone(m.Origin, targetDir, "")
		if err != nil {
			return errors.Trace(err)
		}
	}

	// Now, we'll try to checkout the desired mongoose-os version.
	//
	// It's optimized for two common cases:
	// - We're already on the desired branch (in this case, pull will be performed)
	// - We're already on the desired tag (nothing will be performed)
	// - We're already on the desired SHA (nothing will be performed)
	//
	// All other cases will result in `git fetch`, which is much longer than
	// pull, but we don't care because it will happen if only we switch to
	// another version.

	// First of all, get current SHA
	curHash, err := gitutils.GitGetCurrentHash(targetDir)
	if err != nil {
		if deleteIfFailed {
			// Instead of returning an error, try to delete the directory and
			// clone the fresh copy
			glog.Warningf("%s\n", err)
			glog.V(2).Infof("removing everything under %q", targetDir)

			files, err := ioutil.ReadDir(targetDir)
			if err != nil {
				return errors.Trace(err)
			}
			for _, f := range files {
				glog.V(2).Infof("removing %q", f.Name())
				if err := os.RemoveAll(path.Join(targetDir, f.Name())); err != nil {
					return errors.Trace(err)
				}
			}

			glog.V(2).Infof("calling prepareLocalCopyGit() again")
			return m.prepareLocalCopyGit(targetDir, logFile, false)
		} else {
			return errors.Trace(err)
		}
	}

	glog.V(2).Infof("hash: %q\n", curHash)

	// Check if it's equal to the desired one
	if gitutils.HashesEqual(curHash, version) {
		glog.V(2).Infof("hashes are equal %q, %q\n", curHash, version)
		// Desired mongoose iot version is a fixed SHA, and it's equal to the
		// current commit: we're all set.
		return nil
	}

	var branchExists, tagExists bool

	// Check if MongooseOsVersion is a known branch name
	branchExists, err = gitutils.DoesGitBranchExist(targetDir, version)
	if err != nil {
		return errors.Trace(err)
	}

	glog.V(2).Infof("branch %q exists=%v\n", version, branchExists)

	// Check if MongooseOsVersion is a known tag name
	tagExists, err = gitutils.DoesGitTagExist(targetDir, version)
	if err != nil {
		return errors.Trace(err)
	}

	glog.V(2).Infof("tag %q exists=%v\n", version, tagExists)

	// If the desired mongoose-os version isn't a known branch, do git fetch
	if !branchExists && !tagExists {
		glog.V(2).Infof("neither branch nor tag exists, fetching..\n")
		err = gitutils.GitFetch(targetDir)
		if err != nil {
			return errors.Trace(err)
		}
	}

	// Try to checkout to the requested version
	glog.V(2).Infof("checking out..\n")
	err = gitutils.GitCheckout(targetDir, version)
	if err != nil {
		return errors.Trace(err)
	}

	// Check abbreviated rev name, and if it's not "HEAD", assume we're at the
	// branch, and perform git pull
	//
	// It will fail if there's no tracking information about the branch, but
	// it's not a supported use case anyway
	curRevAbbr, err := gitutils.GitGetCurrentAbbrev(targetDir)
	if err != nil {
		return errors.Trace(err)
	}
	glog.V(2).Infof("rev abbr=%q\n", curRevAbbr)

	if curRevAbbr != "HEAD" {
		glog.V(2).Infof("pulling..\n")
		err = gitutils.GitPull(targetDir)
		if err != nil {
			return errors.Trace(err)
		}
	}

	// To be safe, do `git checkout .`, so that any possible corruptions
	// of the working directory will be fixed
	glog.V(2).Infof("doing checkout .\n")
	err = gitutils.GitCheckout(targetDir, ".")
	if err != nil {
		return errors.Trace(err)
	}

	return nil
}
