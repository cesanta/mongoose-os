package main

import (
	"log"

	"github.com/lxn/walk"
	. "github.com/lxn/walk/declarative"
	"golang.org/x/sys/windows"
)

var (
	procFreeConsole = windows.NewLazyDLL("kernel32.dll").NewProc("FreeConsole")
)

func showUI(url string) {
	var wv *walk.WebView
	procFreeConsole.Call()
	_, err := MainWindow{
		Title:   "Mongoose OS",
		MinSize: Size{1280, 720},
		Layout:  VBox{MarginsZero: true},
		Children: []Widget{
			/*LineEdit{
				AssignTo: &le,
				Text:     Bind("wv.URL"),
				OnKeyDown: func(key walk.Key) {
					if key == walk.KeyReturn {
						wv.SetURL(le.Text())
					}
				},
			},*/
			WebView{
				AssignTo: &wv,
				Name:     "wv",
				URL:      url,
			},
		},
	}.Run()
	if err != nil {
		log.Fatal(err)
	}
}
