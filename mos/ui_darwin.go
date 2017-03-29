package main

import (
	"log"
	"os"

	"github.com/alexflint/gallium"
)

func showUI(url string) {
	err := gallium.Loop(os.Args, func(app *gallium.App) {
		opts := gallium.FramedWindow
		opts.Title = "Mongoose OS"
		opts.Shape = gallium.Rect{
			Width:  1280,
			Height: 720,
			Left:   320,
			Bottom: 180,
		}
		_, err := app.OpenWindow(url, opts)
		if err != nil {
			log.Fatal(err)
		}
		app.SetMenu([]gallium.Menu{
			{
				Title: "Mongoose OS",
				Entries: []gallium.MenuEntry{
					gallium.MenuItem{
						Title:    "Quit",
						Shortcut: gallium.MustParseKeys("cmd q"),
						OnClick:  func() { os.Exit(0) },
					},
				},
			},
		})
	})
	if err != nil {
		log.Fatal(err)
	}
}
