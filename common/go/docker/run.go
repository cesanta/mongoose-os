package docker

import (
	"context"
	"fmt"
	"io"
	"path/filepath"

	"github.com/cesanta/errors"
	"github.com/fsouza/go-dockerclient"
	"github.com/golang/glog"
)

type ExitError struct {
	code int
}

func (e *ExitError) Error() string {
	return fmt.Sprintf("container exited with %d status code", e.code)
}

func (e *ExitError) Code() int {
	return e.code
}

// RunOption is an optional argument to Run
type RunOption func(*options) error

type options struct {
	cmd     []string
	binds   []string
	user    string
	workDir string
}

// Bind option adds a bind mount point to the docker container
func Bind(host, guest, mode string) RunOption {
	return func(opts *options) error {
		if !filepath.IsAbs(host) {
			var err error
			host, err = filepath.Abs(host)
			if err != nil {
				return errors.Trace(err)
			}
		}
		opts.binds = append(opts.binds, fmt.Sprintf("%s:%s:%s", host, guest, mode))
		return nil
	}
}

// Cmd option overrides the command present in the image.
func Cmd(cmd []string) RunOption {
	return func(opts *options) error {
		opts.cmd = cmd
		return nil
	}
}

// User option overrides the user/uid of the process spawned in the docker container
func User(user string) RunOption {
	return func(opts *options) error {
		opts.user = user
		return nil
	}
}

// WorkDir option overrides the working directory of the process spawned in the
// docker container
func WorkDir(workDir string) RunOption {
	return func(opts *options) error {
		opts.workDir = workDir
		return nil
	}
}

// Run spawns a new docker container whose duration is limited by the context
// and that writes its stdout/stderr to out.
func Run(ctx context.Context, image string, out io.Writer, opts ...RunOption) error {
	var options options
	for _, o := range opts {
		if err := o(&options); err != nil {
			return errors.Trace(err)
		}
	}

	cli, err := docker.NewClientFromEnv()
	if err != nil {
		return errors.Trace(err)
	}

	glog.Infof("Creating container: image=%q, cmd=%v, volumes=%v user=%q", image, options.cmd, options.binds, options.user)
	cont, err := cli.CreateContainer(docker.CreateContainerOptions{
		Config: &docker.Config{
			Cmd:        options.cmd,
			Image:      image,
			User:       options.user,
			WorkingDir: options.workDir,
		},
		HostConfig: &docker.HostConfig{
			Binds: options.binds,
		},
	})
	if err != nil {
		return errors.Trace(err)
	}

	defer func() {
		glog.Infof("Removing container %q", cont.ID)
		err = cli.RemoveContainer(docker.RemoveContainerOptions{
			ID:    cont.ID,
			Force: true,
		})
		if err != nil {
			glog.Errorf("%+v", err)
		}
	}()

	glog.Infof("Attaching %q", cont.ID)
	attached := make(chan struct{})
	cw, err := cli.AttachToContainerNonBlocking(docker.AttachToContainerOptions{
		Container:    cont.ID,
		OutputStream: out,
		ErrorStream:  out,
		Stdout:       true,
		Stderr:       true,
		Stream:       true,
		Success:      attached,
	})
	if err != nil {
		return errors.Trace(err)
	}
	defer cw.Close()

	<-attached
	attached <- struct{}{}

	glog.Infof("Starting container %q", cont.ID)
	err = cli.StartContainer(cont.ID, nil)
	if err != nil {
		return errors.Trace(err)
	}

	done := make(chan struct{})
	go func() {
		cw.Wait()
		done <- struct{}{}
	}()

	select {
	case <-done:
		glog.Infof("Container %q exited", cont.ID)
	case <-ctx.Done():
		glog.Infof("Execution for container %q has been interrupted because of %+v", cont.ID, ctx.Err())
		return errors.Trace(ctx.Err())
	}

	c, err := cli.InspectContainer(cont.ID)
	if err != nil {
		return errors.Trace(err)
	}

	if c.State.ExitCode != 0 {
		glog.Errorf("Container %q exited with non-zero exit code %d", cont.ID, c.State.ExitCode)
		return &ExitError{c.State.ExitCode}
	}
	return nil
}
