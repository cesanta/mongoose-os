package main

import (
	"sort"
)

type Deps struct {
	m map[string][]string
	// We need to use a separate map for existence, because m might contain
	// optional dependencies
	exists map[string]struct{}
}

func NewDeps() *Deps {
	return &Deps{
		m:      make(map[string][]string),
		exists: make(map[string]struct{}),
	}
}

func (d *Deps) AddNode(node string) {
	if _, ok := d.m[node]; !ok {
		d.m[node] = []string{}
	}
	d.exists[node] = struct{}{}
}

func (d *Deps) AddDep(node string, dep string) {
	d.m[node] = append(d.m[node], dep)
}

func (d *Deps) AddNodeWithDeps(node string, deps []string) {
	d.AddNode(node)
	for _, dep := range deps {
		d.AddDep(node, dep)
	}
}

func (d *Deps) GetDeps(node string) []string {
	return d.m[node]
}

func (d *Deps) NodeExists(node string) bool {
	_, ok := d.exists[node]
	return ok
}

// Topological returns a slice of node names in the topological order. If
// skipOptDeps is true, optional dependencies are not present in the resulting
// slice (dependency is optional if it's mentioned as a dependency for some
// node, but not present as a node itself).
func (d *Deps) Topological(skipOptDeps bool) (topo []string, cycle []string) {
	dfo := &depthFirstOrder{
		marked:  map[string]bool{},
		onStack: map[string]bool{},
		names:   []string{},
		cycle:   nil,
	}

	// It's not actually necessary to sort nodes, the resulting topological
	// order will be correct without it as well, but the result will not be
	// always the same.
	//
	// We'd rather want the same output given the same depencency graph, so
	// let's sort nodes.
	keys := make([]string, len(d.m))
	i := 0
	for k, _ := range d.m {
		sort.Strings(d.m[k])
		keys[i] = k
		i++
	}
	sort.Strings(keys)

	for _, k := range keys {
		if !dfo.marked[k] {
			dfo.depthFirstSearch(d, k, skipOptDeps)
			if dfo.cycle != nil {
				reverse(dfo.cycle)
				return nil, dfo.cycle
			}
		}
	}

	ret := dfo.names

	// Remove optional deps from the resulting slice
	if skipOptDeps {
		ret = []string{}
		for _, n := range dfo.names {
			if d.NodeExists(n) {
				ret = append(ret, n)
			}
		}
	}

	return ret, nil
}

type depthFirstOrder struct {
	marked  map[string]bool
	onStack map[string]bool
	names   []string
	cycle   []string
}

func (dfo *depthFirstOrder) depthFirstSearch(d *Deps, name string, skipOptDeps bool) {
	dfo.onStack[name] = true
	defer func() { dfo.onStack[name] = false }()

	dfo.marked[name] = true

	for _, k := range d.m[name] {

		if !dfo.marked[k] {
			dfo.depthFirstSearch(d, k, skipOptDeps)
			if dfo.cycle != nil {
				// Dependency cycle was already detected, so we're not going to
				// continue. But before returning, check if first and last items of a
				// cycle are equal. If not, the cycle is not yet completed, so add a
				// current item there
				if dfo.cycle[0] != dfo.cycle[len(dfo.cycle)-1] {
					dfo.cycle = append(dfo.cycle, name)
				}
				return
			}
		} else if dfo.onStack[k] {
			// Dependency graph is not a DAG (i.e. it has a directed cycle), so
			// it's impossible to sort nodes topologically. Start building a cycle
			// slice and return.
			dfo.cycle = []string{k, name}
			return
		}
	}

	dfo.names = append(dfo.names, name)
}

func reverse(s []string) {
	last := len(s) - 1
	for i := 0; i < len(s)/2; i++ {
		s[i], s[last-i] = s[last-i], s[i]
	}
}
