package ourio

import (
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"path/filepath"

	"github.com/cesanta/errors"
)

// CopyFile copies the contents of the file named src to the file named
// by dst. The file will be created if it does not already exist. If the
// destination file exists, all it's contents will be replaced by the contents
// of the source file. The file mode will be copied from the source.
func CopyFile(src, dst string) (err error) {
	in, err := os.Open(src)
	if err != nil {
		err = errors.Trace(err)
		return
	}
	defer in.Close()

	out, err := os.Create(dst)
	if err != nil {
		err = errors.Trace(err)
		return
	}
	defer func() {
		if e := out.Close(); e != nil {
			err = e
		}
	}()

	si, err := os.Stat(src)
	if err != nil {
		err = errors.Trace(err)
		return
	}

	err = os.Chmod(dst, si.Mode())
	if err != nil {
		err = errors.Trace(err)
		return
	}

	_, err = io.Copy(out, in)
	if err != nil {
		err = errors.Trace(err)
		return
	}

	return
}

// CopyDir recursively copies a directory tree, attempting to preserve permissions.
// Source directory must exist, destination must either not exist or be a
// directory.
func CopyDir(src, dst string, blacklist []string) (err error) {
	src = filepath.Clean(src)
	dst = filepath.Clean(dst)

	si, err := os.Stat(src)
	if err != nil {
		err = errors.Trace(err)
		return
	}
	if !si.IsDir() {
		return fmt.Errorf("source is not a directory")
	}

	dstStat, err := os.Stat(dst)
	if err != nil && !os.IsNotExist(err) {
		err = errors.Trace(err)
		return
	}
	if err == nil {
		if !dstStat.IsDir() {
			return fmt.Errorf("destination already exists and is not a directory")
		}
	}

	err = os.MkdirAll(dst, si.Mode())
	if err != nil {
		err = errors.Trace(err)
		return
	}

	entries, err := ioutil.ReadDir(src)
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

		srcPath := filepath.Join(src, entry.Name())
		dstPath := filepath.Join(dst, entry.Name())

		if entry.IsDir() {
			err = CopyDir(srcPath, dstPath, blacklist)
			if err != nil {
				err = errors.Trace(err)
				return
			}
		} else {
			// Skip symlinks.
			if entry.Mode()&os.ModeSymlink != 0 {
				continue
			}

			err = CopyFile(srcPath, dstPath)
			if err != nil {
				err = errors.Trace(err)
				return
			}
		}
	}

	return
}
