/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

/*
 * This is a quick prototype of a server that handles update requests from devices.
 * At present it doesn't do much, but shows what's possible.
 *
 * --fw_map specifies a map of version pattern -> update URL: --fw_map v1=u1,v2=u2,...
 * Version reported by the firmware is matched against patterns in order and if a pattern matches,
 * a redirect response to the corresponding URL is sent (note that firmware supports only absolute URLs).
 * An empty URL is an explicit instruction to do nothing (sends a 304 response).
 * The same happens if reported version does not match any of the patterns.
 * Example:
 * --fw_map 123=https://server1/v456.zip,456=https://server2/v789.zip
 */
package main

import (
	"flag"
	"net/http"
	"path"
	"strings"

	"github.com/golang/glog"
)

const (
	deviceIDHeader  = "X-MIOT-Device-ID"
	fwVersionheader = "X-MIOT-FW-Version"
)

var (
	addr  = flag.String("listen_addr", ":2345", "Address and port to listen on.")
	fwMap = flag.String("fw_map", "", "version=URL map, comma-separated. version is a glob.")
)

func main() {
	flag.Parse()
	http.HandleFunc("/update", updateRedirector)
	glog.Infof("Listening on %s", *addr)
	err := http.ListenAndServe(*addr, nil)
	if err != nil {
		glog.Exitf("ListenAndServe: %s", err)
	}
}

func sendError(w http.ResponseWriter, req *http.Request, ri *RequestInfo, message string, code int) {
	glog.Infof("%s %+v -> %d %s", req.RemoteAddr, *ri, code, message)
	http.Error(w, message, code)
}

func sendRedirect(w http.ResponseWriter, req *http.Request, ri *RequestInfo, url string) {
	glog.Infof("%s %+v -> %s", req.RemoteAddr, *ri, url)
	http.Redirect(w, req, url, http.StatusFound)
}

type RequestInfo struct {
	DeviceID   string
	DeviceMAC  string
	DeviceArch string
	FWVersion  string
	FWBuildID  string
}

func updateRedirector(w http.ResponseWriter, req *http.Request) {
	ri := &RequestInfo{}
	parts := strings.Split(req.Header.Get(deviceIDHeader), " ")
	ri.DeviceID = parts[0]
	if len(parts) > 1 {
		ri.DeviceMAC = parts[1]
	}
	parts = strings.Split(req.Header.Get(fwVersionheader), " ")
	ri.DeviceArch = parts[0]
	if len(parts) > 1 {
		ri.FWVersion = parts[1]
	}
	if len(parts) > 2 {
		ri.FWBuildID = parts[2]
	}
	if ri.DeviceArch == "" || ri.FWVersion == "" {
		sendError(w, req, ri, "Who are you?", http.StatusBadRequest)
		return
	}
	if *fwMap != "" {
		entries := strings.Split(*fwMap, ",")
		for _, e := range entries {
			parts = strings.Split(e, "=")
			vp, url := parts[0], parts[1]
			if matched, _ := path.Match(vp, ri.FWVersion); matched {
				if url != "" {
					sendRedirect(w, req, ri, url)
					return
				} else {
					/* Empty URL means do nothing. */
					break
				}
			}
		}
	}
	// Didn't match anything? No change.
	sendError(w, req, ri, "Steady as she goes", http.StatusNotModified)
}
