// Copyright (c) 2014-2017 Cesanta Software Limited
// All rights reserved

package mosgit

import (
	"flag"

	"cesanta.com/common/go/ourgit"
)

var (
	useShellGit = flag.Bool("use-shell-git", false, "use external git binary instead of internal implementation")
)

// NewOurGit returns an instance of OurGit: if --use-shell-git is given it'll
// be a shell-based implementation; otherwise a go-git-based one.
func NewOurGit() ourgit.OurGit {
	if *useShellGit {
		return ourgit.NewOurGitShell()
	} else {
		return ourgit.NewOurGit()
	}
}
