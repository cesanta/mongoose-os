package main

import (
	"context"
	"encoding/base64"
	"fmt"
	"io"
	"os"
	"path"
	"time"

	"cesanta.com/common/go/lptr"
	fwfilesystem "cesanta.com/fw/defs/fs"
	"cesanta.com/mos/dev"
	"github.com/cesanta/errors"
	"github.com/golang/glog"
	flag "github.com/spf13/pflag"
)

const (
	chunkSize = 512
)

var (
	fsOpTimeout  = 3 * time.Second
	fsOpAttempts = 3
)

func listFiles(ctx context.Context, devConn *dev.DevConn) ([]string, error) {
	// Get file list from the attached device
	files, err := devConn.CFilesystem.List(ctx)
	if err != nil {
		return nil, errors.Trace(err)
	}
	return files, err
}

func fsLs(ctx context.Context, devConn *dev.DevConn) error {
	files, err := listFiles(ctx, devConn)
	if err != nil {
		return errors.Trace(err)
	}
	for _, file := range files {
		fmt.Println(file)
	}
	return nil
}

func getFile(ctx context.Context, devConn *dev.DevConn, name string) (string, error) {
	// Get file
	contents := []byte{}
	var offset int64

	attempts := fsOpAttempts
	for {
		// Get the next chunk of data
		ctx2, _ := context.WithTimeout(ctx, fsOpTimeout)
		glog.V(1).Infof("Getting %s %d @ %d (attempts %d)", name, chunkSize, offset, attempts)
		chunk, err := devConn.CFilesystem.Get(ctx2, &fwfilesystem.GetArgs{
			Filename: &name,
			Offset:   lptr.Int64(offset),
			Len:      lptr.Int64(chunkSize),
		})
		if err != nil {
			attempts -= 1
			if attempts > 0 {
				glog.Warningf("Error: %s", err)
				continue
			}
			// TODO(dfrank): probably handle out of memory error by retrying with a
			// smaller chunk size
			return "", errors.Trace(err)
		}
		attempts = fsOpAttempts

		decoded, err := base64.StdEncoding.DecodeString(*chunk.Data)
		if err != nil {
			return "", errors.Trace(err)
		}

		contents = append(contents, decoded...)
		offset += int64(len(decoded))

		// Check if there is some data left
		if *chunk.Left == 0 {
			break
		}
	}
	return string(contents), nil
}

func fsGet(ctx context.Context, devConn *dev.DevConn) error {
	args := flag.Args()
	if len(args) < 2 {
		return errors.Errorf("filename is required")
	}
	if len(args) > 2 {
		return errors.Errorf("extra arguments")
	}
	filename := args[1]
	text, err := getFile(ctx, devConn, filename)
	if err != nil {
		return errors.Trace(err)
	}
	fmt.Print(string(text))
	return nil
}

func fsPut(ctx context.Context, devConn *dev.DevConn) error {
	args := flag.Args()
	if len(args) < 2 {
		return errors.Errorf("filename is required")
	}
	if len(args) > 3 {
		return errors.Errorf("extra arguments")
	}
	hostFilename := args[1]
	devFilename := path.Base(hostFilename)

	// If device filename was given, use it.
	if len(args) >= 3 {
		devFilename = args[2]
	}

	return fsPutFile(ctx, devConn, hostFilename, devFilename)
}

func fsPutFile(ctx context.Context, devConn *dev.DevConn, hostFilename, devFilename string) error {
	file, err := os.Open(hostFilename)
	if err != nil {
		return errors.Trace(err)
	}
	defer file.Close()

	return fsPutData(ctx, devConn, file, devFilename)
}

func fsPutData(ctx context.Context, devConn *dev.DevConn, r io.Reader, devFilename string) error {
	data := make([]byte, chunkSize)
	appendFlag := false

	attempts := fsOpAttempts
	for {
		// Read the next chunk from the file.
		n, readErr := r.Read(data)
		if n > 0 {
			for attempts > 0 {
				ctx2, _ := context.WithTimeout(ctx, fsOpTimeout)
				glog.V(1).Infof("Sending %s %d (attempts %d)", devFilename, n, attempts)
				err := devConn.CFilesystem.Put(ctx2, &fwfilesystem.PutArgs{
					Filename: &devFilename,
					Data:     lptr.String(base64.StdEncoding.EncodeToString(data[:n])),
					Append:   lptr.Bool(appendFlag),
				})
				if err != nil {
					attempts -= 1
					if attempts > 0 {
						glog.Warningf("Error: %s", err)
						continue
					}
					return errors.Trace(err)
				} else {
					break
				}
			}
		}
		attempts = fsOpAttempts
		if readErr != nil {
			if errors.Cause(readErr) == io.EOF {
				// Reached EOF, quit the loop normally.
				break
			}
			// Some non-EOF error, return error.
			return errors.Trace(readErr)
		}

		// All subsequent writes to this file will append the chunk.
		appendFlag = true
	}

	return nil
}

func fsRemoveFile(ctx context.Context, devConn *dev.DevConn, devFilename string) error {
	return errors.Trace(devConn.CFilesystem.Remove(ctx, &fwfilesystem.RemoveArgs{
		Filename: &devFilename,
	}))
}

func fsRm(ctx context.Context, devConn *dev.DevConn) error {
	args := flag.Args()
	if len(args) < 2 {
		return errors.Errorf("filename is required")
	}
	if len(args) > 2 {
		return errors.Errorf("extra arguments")
	}
	filename := args[1]
	return errors.Trace(fsRemoveFile(ctx, devConn, filename))
}
