package main

import (
	"path/filepath"
)

func enumerateSerialPorts() []string {
	list, _ := filepath.Glob("/dev/ttyUSB*")
	return list
}
