package main

import (
	"bufio"
	"encoding/json"
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"strings"

	"golang.org/x/net/context"

	"cesanta.com/common/go/mgrpc2"
)

func handleStdin(framec chan mgrpc2.Frame) {
	r := bufio.NewReader(os.Stdin)
	for {
		if text, err := r.ReadString('\n'); err != nil {
			break
		} else if parts := strings.SplitN(text, " ", 2); len(parts) < 1 || len(parts) > 2 || parts[0] == "\n" {
			log.Println("Invalid input. Expected: 'method args'")
		} else {
			frame := mgrpc2.Frame{Method: parts[0]}
			if len(parts) > 1 {
				frame.Args = json.RawMessage(parts[1])
			}
			framec <- frame
		}
	}
}

func handleConfig(name string, d mgrpc2.Dispatcher, framec chan mgrpc2.Frame) {
	config := struct {
		Config string `json:"config"`
	}{}
	b, err := ioutil.ReadFile(name)
	if err != nil {
		log.Fatal(err)
	}
	config.Config = string(b)

	d.AddHandler("Config.Get", func(d mgrpc2.Dispatcher, c mgrpc2.Channel, f *mgrpc2.Frame) *mgrpc2.Frame {
		return &mgrpc2.Frame{Result: json.RawMessage(config.Config)}
	})
	d.AddHandler("Config.Set", func(d mgrpc2.Dispatcher, c mgrpc2.Channel, f *mgrpc2.Frame) *mgrpc2.Frame {
		if err := json.Unmarshal([]byte(f.Args), &config); err != nil {
			return &mgrpc2.Frame{Result: json.RawMessage(`false`)}
		} else if err := ioutil.WriteFile(name, []byte(f.Args), 0644); err != nil {
			return &mgrpc2.Frame{Result: json.RawMessage(`false`)}
		}
		config.Config = string([]byte(f.Args))
		return &mgrpc2.Frame{Result: json.RawMessage(`true`)}
	})
}

func handleShadow(name string, d mgrpc2.Dispatcher, framec chan mgrpc2.Frame) {
	shadow := "{}"
	d.AddHandler("Dash.Shadow.Get", func(d mgrpc2.Dispatcher, c mgrpc2.Channel, f *mgrpc2.Frame) *mgrpc2.Frame {
		return &mgrpc2.Frame{Result: json.RawMessage(shadow)}
	})
	d.AddHandler("Dash.Shadow.Update", func(d mgrpc2.Dispatcher, c mgrpc2.Channel, f *mgrpc2.Frame) *mgrpc2.Frame {
		shadow = string([]byte(f.Args))
		return &mgrpc2.Frame{Result: json.RawMessage(`true`)}
	})
}

func main() {
	dst := flag.String("dst", "", "RPC destination")
	key := flag.String("key", "", "RPC frame key (password)")
	config := flag.String("config", "", "Device config JSON")
	shadow := flag.String("shadow", "", "Device shadow JSON")

	flag.Parse()

	if *dst == "" {
		log.Fatalf("Please specify --dst")
	}

	d := mgrpc2.CreateDispatcher(log.Printf)
	d.AddHandler("*", func(d mgrpc2.Dispatcher, c mgrpc2.Channel, f *mgrpc2.Frame) *mgrpc2.Frame {
		switch f.Method {
		case "RPC.List":
			// TODO
		case "RPC.Describe":
			// TODO
		default:
			log.Println(f)
		}
		return &mgrpc2.Frame{Result: json.RawMessage(`true`)}
	})

	framec := make(chan mgrpc2.Frame, 1)
	framec <- mgrpc2.Frame{Method: "Dash.Heartbeat"}
	go handleStdin(framec)

	if *config != "" {
		go handleConfig(*config, d, framec)
	}

	if *shadow != "" {
		go handleShadow(*shadow, d, framec)
	}

	for frame := range framec {
		frame.Dst = *dst
		frame.Key = *key
		res, err := d.Call(context.Background(), &frame)
		if err != nil {
			log.Fatalf("Error calling: %v: %v", frame, err)
		} else {
			s, _ := json.Marshal(&res)
			fmt.Println(string(s))
		}
	}
}
