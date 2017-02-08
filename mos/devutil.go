package main

import (
	"context"
	"crypto/tls"
	"crypto/x509"
	"flag"
	"io/ioutil"
	"strings"

	"cesanta.com/mos/dev"
	"github.com/cesanta/errors"
)

var (
	// NOTE(lsm): we're reusing cert-file and key-file flags from aws.go
	caFile = ""
)

func init() {
	flag.StringVar(&caFile, "ca-cert-file", "", "CA cert for TLS server verification")
	hiddenFlags = append(hiddenFlags, "ca-cert-file")
}

func createDevConn(ctx context.Context) (*dev.DevConn, error) {
	return createDevConnWithJunkHandler(ctx, func(junk []byte) {})
}

func createDevConnWithJunkHandler(
	ctx context.Context, junkHandler func(junk []byte),
) (*dev.DevConn, error) {
	port, err := getPort()
	if err != nil {
		return nil, errors.Trace(err)
	}
	c := dev.Client{Port: port, Timeout: *timeout, Reconnect: *reconnect}
	prefix := "serial://"
	if strings.Index(port, "://") > 0 {
		prefix = ""
	}
	addr := prefix + port

	// Init and pass TLS config if --cert-file and --key-file are specified
	var tlsConfig *tls.Config = nil
	if certFile != "" {
		if keyFile == "" {
			return nil, errors.Errorf("Please specify --key-file")
		}
		cert, err := tls.LoadX509KeyPair(certFile, keyFile)
		if err != nil {
			return nil, errors.Trace(err)
		}
		var caCerts *x509.CertPool = nil
		if caFile != "" {
			caCert, err := ioutil.ReadFile(caFile)
			if err != nil {
				return nil, errors.Trace(err)
			}
			caCerts = x509.NewCertPool()
			caCerts.AppendCertsFromPEM(caCert)
		}

		tlsConfig = &tls.Config{
			RootCAs:            caCerts,
			InsecureSkipVerify: caFile == "",
			Certificates:       []tls.Certificate{cert},
			Renegotiation:      tls.RenegotiateNever,
		}
	}

	devConn, err := c.CreateDevConnWithJunkHandler(ctx, addr, junkHandler, *reconnect, tlsConfig)
	return devConn, errors.Trace(err)
}
