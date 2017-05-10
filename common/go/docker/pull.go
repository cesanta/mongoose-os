package docker

import (
	"context"
	"strings"

	"github.com/cesanta/errors"
	docker "github.com/fsouza/go-dockerclient"
	"github.com/golang/glog"
)

// Pull pulls an image
func Pull(ctx context.Context, image string) error {
	cli, err := docker.NewClientFromEnv()
	if err != nil {
		return errors.Trace(err)
	}

	auths, err := docker.NewAuthConfigurationsFromDockerCfg()
	if err != nil {
		return errors.Trace(err)
	}

	comps := strings.Split(image, "/")
	auth := auths.Configs[comps[0]]

	glog.Infof("Pulling image: image=%q, cmd=%v, volumes=%v user=%q", image)
	err = cli.PullImage(docker.PullImageOptions{
		Repository: image,
	}, auth)
	if err != nil {
		return errors.Trace(err)
	}

	return nil
}
