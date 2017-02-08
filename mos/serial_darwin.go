package main

import (
	"path/filepath"
	"strings"
)

func enumerateSerialPorts() []string {
	list, _ := filepath.Glob("/dev/cu.*")
	var filteredList []string
	for _, s := range list {
		if !strings.Contains(s, "Bluetooth-") {
			filteredList = append(filteredList, s)
		}
	}
	return filteredList
}
