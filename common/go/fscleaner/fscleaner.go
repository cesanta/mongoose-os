package fscleaner

import (
	"context"
	"os"
	"path/filepath"
	"time"

	"github.com/cesanta/errors"
	"github.com/golang/glog"
)

func clean(glob string, minAge time.Duration) error {
	now := time.Now()
	glog.Infof("cleaning %q", glob)

	top, err := filepath.Glob(glob)
	if err != nil {
		return errors.Trace(err)
	}

	for _, d := range top {
		s, err := os.Stat(d)
		// ignore files deleted by other agents running on the same shared volume
		if os.IsNotExist(err) {
			continue
		}
		if err != nil {
			return errors.Trace(err)
		}
		age := now.Sub(s.ModTime())
		glog.V(2).Infof("age of %q: %s", d, age)
		if age > minAge {
			glog.Infof("deleting %q", d)
			os.RemoveAll(d)
		}
	}

	return nil
}

// Run starts a filesystem cleaning goroutine
func Run(ctx context.Context, glob string, period, minAge time.Duration) {
	go func() {
		ticker := time.NewTicker(period)
		defer ticker.Stop()

		err := clean(glob, minAge)
		if err != nil {
			glog.Warning(err)
		}

		for {
			select {
			case <-ctx.Done():
				return
			case <-ticker.C:
				err := clean(glob, minAge)
				if err != nil {
					glog.Warning(err)
				}
			}
		}
	}()
}
