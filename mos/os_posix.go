// +build !windows

package main

func getDefaultPort() string {
	ports := enumerateSerialPorts()
	if len(ports) == 0 {
		return ""
	}
	return ports[0]
}
