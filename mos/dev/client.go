package dev

import (
	"context"
	"flag"
	"time"

	"github.com/cesanta/errors"
)

type Client struct {
	Port      string
	Reconnect bool
	Timeout   time.Duration
}

func (c *Client) RegisterFlags(fs *flag.FlagSet) {
	fs.StringVar(&c.Port, "port", "", "Serial port to use")
	fs.BoolVar(&c.Reconnect, "reconnect", false, "Enable serial port reconnection")
	fs.DurationVar(&c.Timeout, "timeout", 10*time.Second,
		"Timeout for the device connection")
}

func (c *Client) PostProcessFlags(fs *flag.FlagSet) error {
	if c.Port == "" {
		return errors.New("-port is required")
	}

	return nil
}

func UsageSummary() string {
	return "-port <port-name>"
}

// RunWithTimeout takes a parent context and a function, and calls the function
// with the newly created context with timeout (see the "timeout" flag)
func (c *Client) RunWithTimeout(ctx context.Context, f func(context.Context) error) error {
	cctx, cancel := context.WithTimeout(ctx, c.Timeout)
	defer cancel()
	return errors.Trace(f(cctx))
}
