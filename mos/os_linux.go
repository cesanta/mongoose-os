package main

import (
	"fmt"
	"path/filepath"
	"sort"
)

func enumerateSerialPorts() []string {
	// Note: Prefer ttyUSB* to ttyACM*.
	list1, _ := filepath.Glob("/dev/ttyUSB*")
	sort.Strings(list1)
	list2, _ := filepath.Glob("/dev/ttyACM*")
	sort.Strings(list2)
	return append(list1, list2...)
}

func osSpecificInit() {
}

func webview(url string) {
	fmt.Println("WebView for Linux is not yet supported.")
}
