package main

import (
	"sort"
	"strconv"
	"strings"

	"golang.org/x/sys/windows/registry"
)

func enumerateSerialPorts() []string {
	emptyList := []string{}
	k, err := registry.OpenKey(registry.LOCAL_MACHINE, `HARDWARE\DEVICEMAP\SERIALCOMM\`, registry.QUERY_VALUE)
	if err != nil {
		return emptyList
	}
	defer k.Close()
	list, err := k.ReadValueNames(0)
	if err != nil {
		return emptyList
	}
	vsm := make([]string, len(list))
	for i, v := range list {
		val, _, _ := k.GetStringValue(v)
		vsm[i] = val
	}
	return vsm
}

func getCOMNumber(port string) int {
	if !strings.HasPrefix(port, "COM") {
		return -1
	}
	cn, err := strconv.Atoi(port[3:])
	if err != nil {
		return -1
	}
	return cn
}

type byCOMNumber []string

func (a byCOMNumber) Len() int      { return len(a) }
func (a byCOMNumber) Swap(i, j int) { a[i], a[j] = a[j], a[i] }
func (a byCOMNumber) Less(i, j int) bool {
	cni := getCOMNumber(a[i])
	cnj := getCOMNumber(a[j])
	if cni < 0 || cnj < 0 {
		return a[i] < a[j]
	}
	return cni < cnj
}

func getDefaultPort() string {
	ports := enumerateSerialPorts()
	var filteredPorts []string
	for _, p := range ports {
		// COM1 and COM2 are commonly mapped to on-board serial ports which are usually not a good guess.
		if p != "COM1" && p != "COM2" {
			filteredPorts = append(filteredPorts, p)
		}
	}
	if len(filteredPorts) == 0 {
		return ""
	}
	sort.Strings(filteredPorts)
	return filteredPorts[0]
}
