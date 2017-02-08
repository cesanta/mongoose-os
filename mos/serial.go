package main

import "github.com/cesanta/errors"

var defaultPort string

func getPort() (string, error) {
	if *portFlag != "auto" {
		return *portFlag, nil
	}
	if defaultPort == "" {
		defaultPort = getDefaultPort()
		if defaultPort == "" {
			return "", errors.Errorf("--port not specified and none were found")
		}
		reportf("Using port %s", defaultPort)
	}
	return defaultPort, nil
}
