package ourio

import (
	zip_impl "archive/zip"
	"io"
	"os"
	"path"
	"path/filepath"
)

// Archive compresses a file/directory to a writer
//
// If the path ends with a separator, then the contents of the folder at that
// path are at the root level of the archive, otherwise, the root of the
// archive contains the folder as its only item (with contents inside).
//
// If progress is not nil, it is called for each file added to the archive.
func Archive(inFilePath string, writer io.Writer, progress ProgressFunc) error {
	zipWriter := zip_impl.NewWriter(writer)

	basePath := filepath.Dir(inFilePath)

	err := filepath.Walk(inFilePath, func(filePath string, fileInfo os.FileInfo, err error) error {
		if err != nil || fileInfo.IsDir() {
			return err
		}

		relativeFilePath, err := filepath.Rel(basePath, filePath)
		if err != nil {
			return err
		}

		archivePath := path.Join(filepath.SplitList(relativeFilePath)...)

		if progress != nil {
			if add := progress(archivePath); !add {
				// Should skip the current file
				return nil
			}
		}

		file, err := os.Open(filePath)
		if err != nil {
			return err
		}
		defer func() {
			_ = file.Close()
		}()

		zipFileWriter, err := zipWriter.Create(archivePath)
		if err != nil {
			return err
		}

		_, err = io.Copy(zipFileWriter, file)
		return err
	})
	if err != nil {
		return err
	}

	return zipWriter.Close()
}

// ProgressFunc is the type of the function called for each archive file.
type ProgressFunc func(archivePath string) bool
