package heaplog

import (
	"encoding/json"
	"strings"

	"github.com/cesanta/errors"
)

const (
	hpHeader = "hlog_param:"
)

type HeapLogParam struct {
	HeapStart int `json:"heap_start"`
	HeapEnd   int `json:"heap_end"`
}

func (h *HeapLogParam) String() string {
	data, err := json.Marshal(h)
	if err != nil {
		panic(err.Error())
	}

	return hpHeader + string(data)
}

func ParseHeapLogParam(text string) (*HeapLogParam, error) {

	idx := strings.Index(text, hpHeader)
	if idx < 0 {
		return nil, errors.Errorf("no hlog param")
	}

	var ret HeapLogParam

	err := json.Unmarshal([]byte(text[len(hpHeader):]), &ret)
	if err != nil {
		return nil, errors.Trace(err)
	}

	return &ret, nil
}
