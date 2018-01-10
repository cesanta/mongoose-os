/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

package main

import (
	"bufio"
	"bytes"
	"debug/elf"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"net/http"
	"os"
	"time"

	"github.com/golang/glog"
	"golang.org/x/net/websocket"
)

const (
	resetMarker = "hlog_param:{"
)

var (
	addr       = flag.String("listen_addr", ":8910", "Address and port to listen on.")
	binary     = flag.String("binary", "", "Binary to extract symbols from.")
	consoleLog = flag.String("console_log", "", "Console log to tail.")
	docRoot    = flag.String("document_root", ".", "Serve static files form this directory.")
	tail       = flag.Bool("tail", false, "Keep the connection alive and tail the log after reaching the end.")
)

func main() {
	flag.Parse()
	if *consoleLog == "" {
		glog.Exitf("--console_log is required")
	}

	// Test that binary can be opened.
	if *binary != "" {
		f, err := elf.Open(*binary)
		if err != nil {
			glog.Exitf("Error opening ELF binary: %s", err)
		}
		f.Close()
	}

	http.Handle("/", http.FileServer(http.Dir(*docRoot)))

	http.HandleFunc("/addr2sym", addrToSymHandler)
	http.Handle("/log", websocket.Server{Handshake: wsHandshake, Handler: logHandler})

	glog.Infof("Listening on %s, doc root %s, symbols from %s, log %s",
		*addr, *docRoot, *binary, *consoleLog)
	err := http.ListenAndServe(*addr, nil)
	if err != nil {
		glog.Exitf("ListenAndServe: %s", err)
	}
}

func findStartOffset(f *os.File) {
	// Go to the end of the file.
	pos, err := f.Seek(0, 2)
	// If there was an error, asusme file is not seekable (e.g. serial port).
	if err != nil {
		glog.Infof("%s is not seekable, not looking for sync marker", *consoleLog)
		return
	}
	glog.Infof("%s: size %d", *consoleLog, pos)
	blockSize := int64(1024)
	buf := make([]byte, int(blockSize)+len(resetMarker))
	origPos := pos
	for pos > 0 {
		prevPos := pos
		pos = prevPos - blockSize
		if pos < 0 {
			pos = 0
		}
		_, err = f.Seek(pos, 0)
		if err != nil {
			// Previously we were able to see, so this is a genuine error.
			return
		}
		n, err := f.ReadAt(buf, pos)
		glog.V(2).Infof("%d -> %d %s", pos, n, err)
		if err != nil && !(err == io.EOF && n > 0) {
			f.Seek(origPos, 0)
			return
		}
		i := int64(bytes.LastIndex(buf[:n], []byte(resetMarker)))
		if i >= 0 {
			pos += i
			glog.Infof("Found starting position at %d", pos)
			f.Seek(pos, 0)
			return
		}
	}
	f.Seek(origPos, 0)
}

func emitSymbols(fname string, w io.Writer) {
	si := map[string]string{}
	f, err := elf.Open(fname)
	if err == nil {
		defer f.Close()
		syms, _ := f.Symbols()
		for _, s := range syms {
			if s.Value == 0 {
				continue
			}
			si[fmt.Sprintf("%x", s.Value)] = s.Name
		}
	} else {
		glog.Errorf("Error opening ELF binary: %s", err)
	}
	json.NewEncoder(w).Encode(si)
}

func addrToSymHandler(w http.ResponseWriter, req *http.Request) {
	emitSymbols(*binary, w)
}

func wsHandshake(c *websocket.Config, req *http.Request) error {
	return nil
}

func logHandler(ws *websocket.Conn) {
	f, err := os.Open(*consoleLog)
	if err != nil {
		return
	}
	defer f.Close()
	findStartOffset(f)
	if *binary != "" {
		emitSymbols(*binary, ws)
	}
	br := bufio.NewReader(f)
	n := 0
	for {
		l, err := br.ReadBytes('\n')
		if err == nil {
			// Strip timestamps
			if len(l) > 0 && l[0] == '[' {
				if si := bytes.Index(l, []byte("] ")); si > 0 {
					l = l[si+2:]
				}
			}
			_, err = ws.Write(l)
			n++
			if n%10000 == 0 {
				glog.Infof("Served %d lines", n)
			}
		} else if err == io.EOF {
			if *tail {
				time.Sleep(100 * time.Millisecond)
				err = nil
			}
		}
		if err != nil {
			break
		}
	}
	glog.Infof("Finished, served %d lines", n)
}
