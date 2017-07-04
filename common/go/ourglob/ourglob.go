package ourglob

import (
	"path/filepath"
	"strings"

	"github.com/cesanta/errors"
)

type Matcher interface {
	Match(s string) (bool, error)
}

type Pat struct {
	// Patterns to check and corresponding match values. Patterns are checked
	// in order, corresponding Match value is returned at the first pattern
	// match. If no patterns matched, false will be returned.
	Items PatItems
}

type PatItems []Item

type Item struct {
	Pattern string
	Match   bool
}

func (m *Pat) Match(s string) (bool, error) {
	for _, item := range m.Items {
		patParts := strings.Split(item.Pattern, string(filepath.Separator))
		parts := strings.Split(s, string(filepath.Separator))

		// Unfortunately, filepath.Match can only match the whole string, not part
		// of it, and ** is not supported, so we have to just manually cut
		// string if it has more components than the pattern
		if len(parts) > len(patParts) {
			s = strings.Join(parts[:len(patParts)], string(filepath.Separator))
		}

		matched, err := filepath.Match(item.Pattern, s)
		if err != nil {
			return false, errors.Trace(err)
		}

		if matched {
			return item.Match, nil
		}
	}

	// No matching item; assuming no match
	return false, nil
}

// PatItems.Match is a shortcut for Pat.Match with the default options
func (items PatItems) Match(s string) (bool, error) {
	Pat := Pat{
		Items: items,
	}
	return Pat.Match(s)
}
