package heaplog

import (
	"fmt"
	"io"
	"sort"

	"github.com/cesanta/errors"
)

type Heap struct {
	allocs    map[int]*Allocation
	opts      *Opts
	StartAddr int
	Size      int
}

type Opts struct {
	// If true, then allocations which intersect others will not result in an
	// error: instead, previous allocations will be assumed freed before,
	// and warning will be printed to the MsgWriter
	ResolveConflicts bool

	// If true, allocations outside of the current heap address boundaries will
	// not result in an error: instead, boundaries will be expanded with
	// the warning printed to the MsgWriter
	ExpandBoundaries bool

	// If not nil, then warnings will be printed there
	MsgWriter io.Writer
}

type Allocation struct {
	Addr  int
	Size  int
	Shim  bool
	Descr string
}

type allocsByAddr []*Allocation

func (a allocsByAddr) Len() int {
	return len(a)
}
func (a allocsByAddr) Swap(i, j int) {
	a[i], a[j] = a[j], a[i]
}
func (a allocsByAddr) Less(i, j int) bool {
	return a[i].Addr < a[j].Addr
}

func (a *Allocation) String() string {
	return fmt.Sprintf("[addr 0x%x, size %d (%s)]", a.Addr, a.Size, a.Descr)
}

func MkHeap(startAddr, size int, opts *Opts) (*Heap, error) {
	return &Heap{
		StartAddr: startAddr,
		Size:      size,
		allocs:    make(map[int]*Allocation),
		opts:      opts,
	}, nil
}

func (h *Heap) Allocations() []*Allocation {
	allocs := []*Allocation{}
	for _, v := range h.allocs {
		allocs = append(allocs, v)
	}
	sort.Sort(allocsByAddr(allocs))
	return allocs
}

func (h *Heap) Malloc(addr, size int, shim bool, descr string) error {
	newAlloc := &Allocation{
		Addr:  addr,
		Size:  size,
		Descr: descr,
		Shim:  shim,
	}
	if err, existAlloc := h.intersectsErr(newAlloc); err != nil {
		err = errors.Trace(err)
		if h.opts.ResolveConflicts {
			// Assume the region was freed
			h.Free(existAlloc.Addr)
			// Write a warning
			h.warning(err.Error())
			// Call this function again recursively, because there might be other
			// intersecting regions
			return h.Malloc(addr, size, shim, descr)
		} else {
			return err
		}
	}

	// TODO(dfrank): check heap boundaries

	h.allocs[addr] = newAlloc

	return nil
}

func (h *Heap) Free(addr int) error {
	if _, ok := h.allocs[addr]; ok {
		delete(h.allocs, addr)
	} else {
		err := errors.Errorf("free: there's no allocation at addr 0x%x", addr)
		if h.opts.ResolveConflicts {
			h.warning(err.Error())
		} else {
			return err
		}
	}

	return nil
}

// Returns whether the given range intersects any of the already allocated
// regions
func (h *Heap) Intersects(addr, size int) *Allocation {
	for curAddr, al := range h.allocs {
		if curAddr < addr+size && curAddr+al.Size >= addr {
			return al
		}
	}
	return nil
}

func (h *Heap) intersectsErr(a *Allocation) (error, *Allocation) {
	existAlloc := h.Intersects(a.Addr, a.Size)
	if existAlloc != nil {
		return errors.Errorf(
			"new allocation %v intersects existing one %v", *a, *existAlloc,
		), existAlloc
	}
	return nil, nil
}

func (h *Heap) warning(msg string) {
	if h.opts.MsgWriter != nil {
		h.opts.MsgWriter.Write([]byte("warning: "))
		h.opts.MsgWriter.Write([]byte(msg))
		h.opts.MsgWriter.Write([]byte("\n"))
	}
}
