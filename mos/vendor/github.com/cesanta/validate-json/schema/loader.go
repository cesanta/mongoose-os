package schema

import (
	"fmt"
	"net/http"
	"strings"

	json "github.com/cesanta/ucl"
)

// Loader is an entity used for fetching schemas by reference.
type Loader struct {
	networkEnabled bool
	cache          map[string]json.Value
}

// NewLoader creates a new Loader instance.
func NewLoader() *Loader {
	return &Loader{cache: map[string]json.Value{}}
}

// Get returns a schema identified with id.
func (l *Loader) Get(id string) (json.Value, error) {
	s, found := l.cache[id]
	if found {
		return s, nil
	}
	if l.networkEnabled {
		resp, err := http.Get(id)
		if err != nil {
			return nil, err
		}
		s, err := json.Parse(resp.Body)
		resp.Body.Close()
		if err != nil {
			return nil, err
		}
		l.AddAs(s, id)
		return s, nil
	}
	return nil, fmt.Errorf("schema %q is not present in the cache and fetching is disabled", id)
}

// Add adds schema to the cache. Schema must have 'id' property.
func (l *Loader) Add(schema json.Value) error {
	s, ok := schema.(*json.Object)
	if !ok {
		return fmt.Errorf("schema must be an object")
	}
	id, ok := s.Find("id").(*json.String)
	if !ok {
		return fmt.Errorf("schema must have string property \"id\"")
	}
	return l.AddAs(schema, id.Value)
}

// AddAs adds schema to the cache as if its 'id' property was set to id.
func (l *Loader) AddAs(schema json.Value, id string) error {
	_, ok := schema.(*json.Object)
	if !ok {
		return fmt.Errorf("schema must be an object")
	}
	if i := strings.Index(id, "#"); i >= 0 {
		id = id[:i]
	}
	if id == "" {
		return fmt.Errorf("cannot add a schema with empty id")
	}
	l.cache[id] = schema
	return nil
}

// EnableNetworkAccess enables or disables fetching schemas not present in the
// cache from the Internet. Use with caution.
func (l *Loader) EnableNetworkAccess(enable bool) {
	l.networkEnabled = enable
}
