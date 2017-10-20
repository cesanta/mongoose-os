package manifest_parser

import "sync"

type stringFlagSet struct {
	m   map[string]struct{}
	mtx sync.Mutex
}

func newStringFlagSet() *stringFlagSet {
	return &stringFlagSet{
		m:   map[string]struct{}{},
		mtx: sync.Mutex{},
	}
}

// Add tries to add a new key to the set. If key was added, returns true;
// otherwise (key already exists) returns false.
func (fs *stringFlagSet) Add(key string) bool {
	fs.mtx.Lock()
	defer fs.mtx.Unlock()

	_, ok := fs.m[key]
	if !ok {
		fs.m[key] = struct{}{}
		return true
	}

	return false
}
