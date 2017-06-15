package main

import (
	"bufio"
	"flag"
	"fmt"
	"os"
	"strings"

	"cesanta.com/tools/heaplog_viewer/heaplog"

	"github.com/cesanta/errors"
	"github.com/golang/glog"
)

var (
	consoleLog = flag.String("console_log", "", "Console log to shorten.")
	maxLine    = flag.String("max_line", "", "TODO: Max line up to which the log should be swallowed and replaced by the equivalent set of mallocs. If omitted, all log is swallowed.")
)

func main() {
	flag.Parse()
	if *consoleLog == "" {
		glog.Exitf("--console_log is required")
	}

	f, err := os.Open(*consoleLog)
	if err != nil {
		glog.Exitf("%s", errors.Trace(err))
	}
	defer f.Close()

	r := bufio.NewReader(f)

	hlOpts := &heaplog.Opts{
		ResolveConflicts: true,
		MsgWriter:        os.Stderr,
	}

	heap, err := heaplog.MkHeap(0, 10000000, hlOpts)
	if err != nil {
		glog.Exitf("%s", errors.Trace(err))
	}

	for i := 0; ; i++ {
		line, err := r.ReadString('\n')
		if err != nil {
			break
		}

		// if the line contains more than one heaplog items, split it further
		parts := strings.Split(line[:len(line)-1], "hl{")
		if len(parts) > 1 {
			parts = parts[1:]
			for i, _ := range parts {
				parts[i] = "hl{" + parts[i] + "\n"
			}
		}

		for _, part := range parts {
			hlParam, err := heaplog.ParseHeapLogParam(part)
			if err == nil {
				// Got new heap params
				heap, err = heaplog.MkHeap(
					hlParam.HeapStart,
					hlParam.HeapEnd-hlParam.HeapStart,
					hlOpts,
				)
				if err != nil {
					glog.Exitf("%s", errors.Trace(err))
				}
			}

			logItem, err := heaplog.ParseLogItem(part)
			if err != nil {
				continue
			}

			switch logItem.ItemType {
			case heaplog.LogItemTypeMalloc, heaplog.LogItemTypeCalloc, heaplog.LogItemTypeZalloc:
				heap.Malloc(logItem.Addr1, logItem.Size, logItem.Shim, logItem.Descr)
			case heaplog.LogItemTypeRealloc:
				if logItem.Addr1 != 0 {
					heap.Free(logItem.Addr1)
				}
				heap.Malloc(logItem.Addr2, logItem.Size, logItem.Shim, logItem.Descr)
			case heaplog.LogItemTypeFree:
				if logItem.Addr1 != 0 {
					heap.Free(logItem.Addr1)
				}
			}
		}

		// TODO(dfrank): check maxLine
	}

	// Emit heap params
	hlParam := heaplog.HeapLogParam{
		HeapStart: heap.StartAddr,
		HeapEnd:   heap.StartAddr + heap.Size,
	}
	fmt.Fprintf(os.Stdout, "%s\n", hlParam.String())

	// Emit all the swallowed allocations as a series of mallocs
	for _, alloc := range heap.Allocations() {
		logItem := &heaplog.LogItem{
			ItemType: heaplog.LogItemTypeMalloc,
			Addr1:    alloc.Addr,
			Size:     alloc.Size,
			Descr:    alloc.Descr,
			Shim:     alloc.Shim,
		}
		fmt.Fprintf(os.Stdout, "%s", logItem.String())
	}

	// TODO(dfrank): emit all the lines after maxLine as they were
}
