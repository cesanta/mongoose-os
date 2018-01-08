package manifest_parser

import (
	"fmt"
	"math/rand"
	"reflect"
	"sort"
	"testing"
	"time"

	"github.com/cesanta/errors"
)

func TestDeps(t *testing.T) {
	deps := NewDeps()

	deps.AddNodeWithDeps("subbar", nil)
	deps.AddNodeWithDeps("foo", []string{"bar", "baz"})
	deps.AddNodeWithDeps("bar", []string{"subbar"})
	deps.AddNodeWithDeps("app", []string{"bar", "foo"})

	{
		topo, cycle := deps.Topological(false)
		if err := compareStringSlices(
			[]string{"subbar", "bar", "baz", "foo", "app"}, topo,
		); err != nil {
			t.Fatal(err)
		}

		if cycle != nil {
			t.Fatal("cycle should be nil")
		}
	}

	{
		topo, cycle := deps.Topological(true)
		if err := compareStringSlices(
			[]string{"subbar", "bar", "foo", "app"}, topo,
		); err != nil {
			t.Fatal(err)
		}

		if cycle != nil {
			t.Fatal("cycle should be nil")
		}
	}

	// Add a dependency which would create a cycle and make sure Topological
	// returns nil
	deps.AddDep("subbar", "foo")
	{
		topo, cycle := deps.Topological(false)

		if topo != nil {
			t.Fatalf("got: %v, want: %v", topo, nil)
		}

		if err := checkCycle(cycle, []string{"foo", "subbar", "bar"}); err != nil {
			t.Fatal(err)
		}
	}
}

func TestDepsRandom(t *testing.T) {
	for i := 10; i < 100; i++ {
		d := generateDepsDAG(i)
		if err := tryDeps(d); err != nil {
			t.Fatal(err)
		}

		//fmt.Printf("nodes: %v\n", d.m)
		//for j := 0; j < 10; j++ {
		//topo, _ := d.Topological(true)
		//fmt.Printf("topo #%d: %v\n", j, topo)
		//}

		// Add a minimal cycle: from some item to itself
		n := rand.Intn(i)
		sn := fmt.Sprintf("%d", n)
		d.AddDep(sn, sn)

		topo, cycle := d.Topological(true)
		if topo != nil {
			t.Fatalf("nodes: %v, topo: %v, there is a cycle from %d to %d, so topo should be nil", d.m, topo, n, n)
		}

		if err := checkCycle(cycle, []string{sn}); err != nil {
			t.Fatal(err)
		}
	}
}

func compareStringSlices(want, got []string) error {
	if !reflect.DeepEqual(want, got) {
		return errors.Errorf("want: %q, got: %q", want, got)
	}
	return nil
}

func generateDepsDAG(size int) *Deps {
	rand.Seed(time.Now().UTC().UnixNano())

	d := NewDeps()
	for i := 0; i < size; i++ {
		d.AddNode(fmt.Sprintf("%d", i))
	}

	numDeps := rand.Intn(size * 5)

	for i := 0; i < numDeps; i++ {
		nodeNum := rand.Intn(size - 1)
		depNum := nodeNum + 1 + rand.Intn(size-nodeNum-1)
		d.AddDep(fmt.Sprintf("%d", nodeNum), fmt.Sprintf("%d", depNum))
	}

	return d
}

func randInt(min int, max int) int {
	return min + rand.Intn(max-min)
}

func tryDeps(d *Deps) error {
	nodes := d.m

	skipOptDeps := true

	topo, _ := d.Topological(skipOptDeps)

	// Check that topological order is correct
	for node, deps := range nodes {
		nodeIdx := indexOfString(node, topo)
		for _, dep := range deps {
			depIdx := indexOfString(dep, topo)
			if depIdx < 0 {
				if _, ok := nodes[dep]; ok {
					return errors.Errorf(
						"nodes: %v, topo: %v, %q is not found in topo, but it should be there",
						nodes, topo, dep,
					)
				}
			} else if depIdx >= nodeIdx {
				return errors.Errorf(
					"nodes: %v, topo: %v, %q (idx %d) is a dep of %q (idx %d) and it should come before, but it comes after",
					nodes, topo, dep, depIdx, node, nodeIdx,
				)
			}
		}
	}

	if skipOptDeps {
		// Check that topological slice does not contain extra items
		for _, cur := range topo {
			if _, ok := nodes[cur]; !ok {
				return errors.Errorf(
					"nodes: %v, topo: %v, topo contains extra item %q",
					nodes, topo, cur,
				)
			}
		}
	}

	return nil
}

func checkCycle(cycle []string, elements []string) error {
	if len(cycle) < 2 {
		return errors.Errorf("cycle len should be >= 2, but it's %d (%v)", len(cycle), cycle)
	}

	if cycle[0] != cycle[len(cycle)-1] {
		return errors.Errorf("%v: first and last elements of cycle should be the same", cycle)
	}

	// Check exact elements if provided
	if elements != nil {
		sortedCycle := cycle[0 : len(cycle)-1]
		sort.Strings(sortedCycle)
		sort.Strings(elements)

		if !reflect.DeepEqual(sortedCycle, elements) {
			return errors.Errorf("expect those two to be equal: sortedCycle: %v, elements: %v", sortedCycle, elements)
		}
	}

	return nil
}

func indexOfString(needle string, haystack []string) int {
	for k, v := range haystack {
		if v == needle {
			return k
		}
	}
	return -1
}
