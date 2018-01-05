package ourio

import (
	"io/ioutil"
	"os"
	"path/filepath"

	"github.com/cesanta/errors"
)

// RemoveFromDir removes everything from the given dir, except items with
// blacklisted names
func RemoveFromDir(dir string, blacklist []string) (err error) {
	entries, err := ioutil.ReadDir(dir)
	if err != nil {
		err = errors.Trace(err)
		return
	}

entriesLoop:
	for _, entry := range entries {
		for _, v := range blacklist {
			if entry.Name() == v {
				// Current entry is blacklisted, skip
				continue entriesLoop
			}
		}

		if err := os.RemoveAll(filepath.Join(dir, entry.Name())); err != nil {
			return errors.Trace(err)
		}
	}

	return nil
}
