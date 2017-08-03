package main

import (
	"path/filepath"
)

func enumerateSerialPorts() []string {
	list1, _ := filepath.Glob("/dev/ttyUSB*")
	list2, _ := filepath.Glob("/dev/ttyACM*")
	return append(list1, list2...)
}
