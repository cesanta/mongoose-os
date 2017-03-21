package main

import (
	"log"
	"strings"

	"github.com/lxn/walk"
	. "github.com/lxn/walk/declarative"
	"golang.org/x/sys/windows"
)

var (
	procFreeConsole = windows.NewLazyDLL("kernel32.dll").NewProc("FreeConsole")
)

func showUI(url string) {
	var le *walk.LineEdit
	var wv *walk.WebView
	procFreeConsole.Call()
	_, err := MainWindow{
		Title:   "Mongoose OS",
		MinSize: Size{1024, 768},
		Layout:  VBox{MarginsZero: true},
		Children: []Widget{
			LineEdit{
				AssignTo: &le,
				Text:     Bind("wv.URL"),
				OnKeyDown: func(key walk.Key) {
					if key == walk.KeyReturn {
						wv.SetURL(le.Text())
					}
				},
			},
			WebView{
				AssignTo: &wv,
				Name:     "wv",
				URL:      url,
			},
		},
		Functions: map[string]func(args ...interface{}) (interface{}, error){
			"icon": func(args ...interface{}) (interface{}, error) {
				if strings.HasPrefix(args[0].(string), "https") {
					return "check", nil
				}
				return "stop", nil
			},
		},
	}.Run()
	log.Fatal(err)
}
