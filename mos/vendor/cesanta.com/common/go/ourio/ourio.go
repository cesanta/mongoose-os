package ourio

import (
	"bytes"
	"io/ioutil"
	"os"

	"github.com/cesanta/errors"
)

func WriteFileIfDiffers(filename string, data []byte, perm os.FileMode) error {
	exData, err := ioutil.ReadFile(filename)
	if err != nil || bytes.Compare(exData, data) != 0 {
		// File data is different, need to update it

		if err := ioutil.WriteFile(filename, data, perm); err != nil {
			return errors.Trace(err)
		}
	}

	return nil
}
