package codec

import (
	"context"
	"crypto/tls"
	"encoding/json"
	"fmt"
	"net/url"
	"time"

	"cesanta.com/common/go/mgrpc/frame"

	"github.com/cesanta/errors"
	"github.com/golang/glog"
	"github.com/yosssi/gmq/mqtt"
	"github.com/yosssi/gmq/mqtt/client"
)

type mqttCodec struct {
	dst         string
	closeNotify chan struct{}
	rchan       chan frame.Frame
	topic       string
	cli         *client.Client
}

func MQTT(dst string, tlsConfig *tls.Config) (Codec, error) {
	u, err := url.Parse(dst)
	if err != nil {
		return nil, errors.Trace(err)
	}
	topic := u.Path[1:]
	rchan := make(chan frame.Frame)

	cli := client.New(&client.Options{
		ErrorHandler: func(err error) {
			glog.Errorf("MQTT error: %v", err)
		},
	})

	clientID := fmt.Sprintf("mos-%v", time.Now().Unix())
	co := client.ConnectOptions{
		Network:      "tcp",
		Address:      u.Host,
		ClientID:     []byte(clientID),
		TLSConfig:    tlsConfig,
		UserName:     []byte(clientID),
		CleanSession: true,
	}

	glog.Infof("Connecting %s to %s", clientID, u.Host)
	err = cli.Connect(&co)
	if err != nil {
		return nil, errors.Trace(err)
	}

	glog.Infof("Subscribing to [%s]", topic)
	err = cli.Subscribe(&client.SubscribeOptions{
		SubReqs: []*client.SubReq{
			&client.SubReq{
				TopicFilter: []byte(topic + "/rpc"),
				QoS:         mqtt.QoS0,
				Handler: func(topicName, message []byte) {
					glog.Infof("Got MQTT message, topic [%s], message [%s]", string(topicName), string(message))
					f := &frame.Frame{}
					if err := json.Unmarshal(message, &f); err != nil {
						glog.Errorf("Invalid json (%s): %+v", err, string(message))
						return
					}
					rchan <- *f
				},
			},
		},
	})

	if err != nil {
		return nil, errors.Trace(err)
	}

	return &mqttCodec{
		dst:         dst,
		closeNotify: make(chan struct{}),
		rchan:       rchan,
		topic:       topic,
		cli:         cli,
	}, nil
}

func (c *mqttCodec) Close() {
}

func (c *mqttCodec) CloseNotify() <-chan struct{} {
	return c.closeNotify
}

func (c *mqttCodec) String() string {
	return fmt.Sprintf("[mqttCodec to %s]", c.dst)
}

func (c *mqttCodec) Info() ConnectionInfo {
	return ConnectionInfo{}
}

func (c *mqttCodec) MaxNumFrames() int {
	return -1
}

func (c *mqttCodec) Recv(ctx context.Context) (*frame.Frame, error) {
	f := <-c.rchan
	return &f, nil
}

func (c *mqttCodec) Send(ctx context.Context, f12 *frame.Frame) error {
	f12.Src = c.topic
	msg, err := json.Marshal(f12)
	if err != nil {
		return errors.Trace(err)
	}
	glog.Infof("Sending [%s] to [%s]", string(msg), c.topic)
	err = c.cli.Publish(&client.PublishOptions{
		QoS:       mqtt.QoS1,
		Retain:    false,
		TopicName: []byte(c.topic),
		Message:   []byte(msg),
	})
	if err != nil {
		return errors.Trace(err)
	}
	return nil
}
