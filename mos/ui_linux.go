package main

import (
	"github.com/skratchdot/open-golang/open"
)

func showUI(url string) {
	open.Start(url)
	<-(chan struct{})(nil)
}
