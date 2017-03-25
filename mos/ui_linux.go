package main

import (
	"log"

	"github.com/skratchdot/open-golang/open"
)

func showUI(url string) {
	if err := open.Start(url); err != nil {
		log.Fatal(err)
	}
	// we've launched a http server, so wait forever.
	<-(chan struct{})(nil)
}
