package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"log"

	"golang.org/x/net/context"

	"cesanta.com/common/go/mgrpc2"
)

func main() {
	dst := flag.String("dst", "", "RPC destination")
	method := flag.String("method", "", "Method name to call")
	key := flag.String("key", "", "RPC frame key (password)")
	args := flag.String("args", "", "RPC command arguments")
	flag.Parse()

	if *dst == "" || *method == "" {
		log.Fatalf("Please specify --dst and --method")
	}
	d := mgrpc2.CreateDispatcher()
	req := mgrpc2.Frame{Dst: *dst, Method: *method, Key: *key}
	if *args != "" {
		req.Args = json.RawMessage(*args)
	}
	res, err := d.Call(context.Background(), &req)
	if err != nil {
		log.Fatalf("Error calling: %s@%s: %v", *method, *dst, err)
	}
	s, _ := json.Marshal(&res)
	fmt.Println(string(s))
}
