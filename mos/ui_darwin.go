package main

import (
	"os"

	"github.com/alexflint/gallium"
)

func showUI(url string) {
	gallium.Loop(os.Args, func(app *gallium.App) {
		opts := gallium.FramedWindow
		app.OpenWindow(url, opts)
	})
}
