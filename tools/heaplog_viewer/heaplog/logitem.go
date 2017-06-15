package heaplog

import (
	"fmt"
	"strings"

	"github.com/cesanta/errors"
)

type LogItemType int

const (
	LogItemTypeMalloc LogItemType = iota
	LogItemTypeCalloc
	LogItemTypeZalloc
	LogItemTypeFree
	LogItemTypeRealloc
)

type LogItem struct {
	ItemType LogItemType
	Addr1    int
	Addr2    int
	Size     int
	Shim     bool
	Descr    string
}

func (l *LogItem) String() string {
	tc, err := getLogItemTypeChar(l.ItemType)
	if err != nil {
		return err.Error()
	}

	switch l.ItemType {
	case LogItemTypeMalloc, LogItemTypeCalloc, LogItemTypeZalloc:
		return fmt.Sprintf("hl{%c,%d,%d,%x}%s", tc, l.Size, getShimInt(l.Shim), l.Addr1, l.Descr)
	case LogItemTypeRealloc:
		return fmt.Sprintf("hl{%c,%d,%d,%x,%x}%s", tc, l.Size, getShimInt(l.Shim), l.Addr1, l.Addr2, l.Descr)
	case LogItemTypeFree:
		return fmt.Sprintf("hl{%c,%x,%d}%s", tc, l.Addr1, l.Shim, l.Descr)
	default:
		return "[error wrong logitem]"
	}
}

func ParseLogItem(text string) (*LogItem, error) {
	var t int
	num, err := fmt.Sscanf(text, "hl{%c,", &t)
	if err != nil {
		return nil, errors.Trace(err)
	}
	if num < 1 {
		// Given text is not a log item
		return nil, errors.Errorf("failed to parse type")
	}

	itemType, err := getLogItemType(t)
	if err != nil {
		return nil, errors.Trace(err)
	}
	switch itemType {

	case LogItemTypeMalloc, LogItemTypeCalloc, LogItemTypeZalloc:
		var size, shim, addr int
		num, err := fmt.Sscanf(text, "hl{%c,%d,%d,%x}", &t, &size, &shim, &addr)
		if err != nil {
			return nil, errors.Annotatef(err, "failed to parse malloc/calloc/zalloc item")
		}
		if num != 4 {
			return nil, errors.Errorf("failed to parse malloc/calloc/zalloc item")
		}
		return &LogItem{
			ItemType: itemType,
			Addr1:    addr,
			Size:     size,
			Shim:     shim != 0,
			Descr:    text[strings.Index(text, "}")+1:],
		}, nil

	case LogItemTypeRealloc:
		var size, shim, addr1, addr2 int
		num, err := fmt.Sscanf(text, "hl{%c,%d,%d,%x,%x}", &t, &size, &shim, &addr1, &addr2)
		if err != nil {
			return nil, errors.Annotatef(err, "failed to parse realloc item")
		}
		if num != 5 {
			return nil, errors.Errorf("failed to parse realloc item")
		}
		return &LogItem{
			ItemType: itemType,
			Addr1:    addr1,
			Addr2:    addr2,
			Size:     size,
			Shim:     shim != 0,
			Descr:    text[strings.Index(text, "}")+1:],
		}, nil

	case LogItemTypeFree:
		var shim, addr int
		num, err := fmt.Sscanf(text, "hl{%c,%x,%d}", &t, &addr, &shim)
		if err != nil {
			return nil, errors.Annotatef(err, "failed to parse free item")
		}
		if num != 3 {
			return nil, errors.Errorf("failed to parse free item")
		}
		return &LogItem{
			ItemType: itemType,
			Addr1:    addr,
			Shim:     shim != 0,
			Descr:    text[strings.Index(text, "}")+1:],
		}, nil

	default:
		fmt.Printf("logItemType: %v\n", t)
		panic("should never be here")
	}
}

func getLogItemType(t int) (LogItemType, error) {
	var ret LogItemType
	switch t {
	case 'm':
		ret = LogItemTypeMalloc
	case 'c':
		ret = LogItemTypeCalloc
	case 'z':
		ret = LogItemTypeZalloc
	case 'f':
		ret = LogItemTypeFree
	case 'r':
		ret = LogItemTypeRealloc
	default:
		return ret, errors.Errorf("unexpected log item type: %c", t)
	}
	return ret, nil
}

func getLogItemTypeChar(t LogItemType) (int, error) {
	var ret int
	switch t {
	case LogItemTypeMalloc:
		ret = 'm'
	case LogItemTypeCalloc:
		ret = 'c'
	case LogItemTypeZalloc:
		ret = 'z'
	case LogItemTypeFree:
		ret = 'f'
	case LogItemTypeRealloc:
		ret = 'r'
	default:
		return ret, errors.Errorf("unexpected log item type: %v", t)
	}
	return ret, nil
}

func getShimInt(shim bool) int {
	if shim {
		return 1
	}
	return 0
}
