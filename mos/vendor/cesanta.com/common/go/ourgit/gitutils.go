// Copyright (c) 2014-2017 Cesanta Software Limited
// All rights reserved

package ourgit

type OurGit interface {
	GetCurrentHash(localDir string) (string, error)
	DoesBranchExist(localDir string, branchName string) (bool, error)
	DoesTagExist(localDir string, tagName string) (bool, error)
	GetToplevelDir(localDir string) (string, error)
	Checkout(localDir string, id string, refType RefType) error
	ResetHard(localDir string) error
	Pull(localDir string) error
	Fetch(localDir string) error
	IsClean(localDir, version string) (bool, error)
	Clone(srcURL, localDir string) error
	CloneReferenced(srcURL, localDir, referenceLocalDir string) error
	GetOriginUrl(localDir string) (string, error)
}

type RefType string

const (
	RefTypeBranch RefType = "branch"
	RefTypeTag    RefType = "tag"
	RefTypeHash   RefType = "hash"

	minHashLen  = 6
	fullHashLen = 40
)

// NewOurGit returns a go-git-based implementation of OurGit
// (it doesn't require an external git binary, but is somewhat limited; for
// example, it doesn't support referenced repositories)
func NewOurGit() OurGit {
	return &ourGitGoGit{}
}

// NewOurGit returns a shell-based implementation of OurGit
// (external git binary is required for that to work)
func NewOurGitShell() OurGit {
	return &ourGitShell{}
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
