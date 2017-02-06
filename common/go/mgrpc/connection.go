package mgrpc

import (
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"io/ioutil"
	"net/url"

	"github.com/cesanta/errors"
)

type connectOptions struct {
	proto           transport
	connectAddress  string
	localID         string
	serverHost      string
	defaultRoute    bool
	useTLS          bool
	cert            *tls.Certificate
	caPool          *x509.CertPool
	tlsConfig       *tls.Config
	psk             string
	enableUBJSON    bool
	enableTracing   bool
	enableReconnect bool
	junkHandler     func(junk []byte)
}

// ConnectOption is an optional argument to Instance.Connect which affects the
// behaviour of the connection.
type ConnectOption func(*connectOptions) error

func TlsConfig(tlsConfig *tls.Config) ConnectOption {
	return func(c *connectOptions) error {
		c.tlsConfig = tlsConfig
		return nil
	}
}

// UseWebSocket instructs RPC to create a WebSocket connection.
func UseWebSocket() ConnectOption {
	return func(c *connectOptions) error {
		c.proto = tWebSocket
		return nil
	}
}

// UseHTTPPost instructs RPC to use HTTP POST requests to send commands.
func UseHTTPPost() ConnectOption {
	return func(c *connectOptions) error {
		c.proto = tHTTP_POST
		return nil
	}
}

// LocalID specifies mgrpc id of the local node
func LocalID(localID string) ConnectOption {
	return func(c *connectOptions) error {
		c.localID = localID
		return nil
	}
}

// connectTo specifies the address used by HTTP_POST and WebSocket transports.
// Use it if the address automatically inferred from destination address is not
// suitable (e.g. you're connecting to another network interface of the device).
//
// This function is unexported, because mgrpc.New() takes connectAddr as a
// separate argument.
func connectTo(connectURL string) ConnectOption {
	url, err := url.Parse(connectURL)
	if err != nil {
		return badConnectOption(errors.Errorf("invalid ConnectTo format: %s", err))
	}
	var t transport
	var a string
	var tls bool
	switch {
	case url.Scheme == "http" || url.Scheme == "https":
		url.RawQuery = ""
		url.Fragment = ""
		t, a = tHTTP_POST, url.String()
		tls = url.Scheme == "https"
	case url.Scheme == "mqtt" || url.Scheme == "mqtts":
		url.RawQuery = ""
		url.Fragment = ""
		t, a = tMQTT, url.String()
		tls = url.Scheme == "mqtts"
	case url.Scheme == "ws" || url.Scheme == "wss":
		url.RawQuery = ""
		url.Fragment = ""
		t, a = tWebSocket, url.String()
		tls = url.Scheme == "wss"
	case url.Scheme == "tcp":
		t, a = tPlainTCP, url.Host
	case url.Scheme == "serial":
		// it might look like "serial:///dev/ttyUSB0" or "serial://COM7", so the
		// actual payload will be either in url.Host or url.Path.
		t, a = tSerial, url.Host+url.Path
	default:
		return badConnectOption(errors.Errorf("invalid ConnectTo protocol %q", url.Scheme))
	}
	return func(c *connectOptions) error {
		c.proto = t
		c.connectAddress = a
		c.useTLS = tls
		return nil
	}
}

// ClientCert specifies the client certificate chain to supply to the server
// and enables TLS.
func ClientCert(cert *tls.Certificate) ConnectOption {
	return func(o *connectOptions) error {
		o.cert = cert
		o.useTLS = cert != nil
		return nil
	}
}

func maybeLoadCertAndKey(certFile, keyFile string) (*tls.Certificate, error) {
	if certFile == "" && keyFile == "" {
		return nil, nil
	}
	cert, err := tls.LoadX509KeyPair(certFile, keyFile)
	if err != nil {
		return nil, errors.Errorf("failed to load TLS cert: %s", err)
	}
	return &cert, nil
}

// ClientCertFiles will load certificate and key from the specified files
// to present to the server and enables TLS.
// If file names are empty, this option does nothing.
func ClientCertFiles(certFile, keyFile string) ConnectOption {
	cert, err := maybeLoadCertAndKey(certFile, keyFile)
	if err != nil {
		return badConnectOption(err)
	}
	return ClientCert(cert)
}

// VerifyServerWith specifies the pool of CA certificates to use for validating
// server's certificate.
func VerifyServerWith(ca *x509.CertPool) ConnectOption {
	return func(c *connectOptions) error {
		c.caPool = ca
		return nil
	}
}

// VerifyServerWithCAsFromFile is a wrapper for VerifyServerWith that reads
// CA certificates from file.
func VerifyServerWithCAsFromFile(file string) ConnectOption {
	if file == "" {
		return VerifyServerWith(nil)
	}
	pem, err := ioutil.ReadFile(file)
	if err != nil {
		return badConnectOption(err)
	}
	ca := x509.NewCertPool()
	if !ca.AppendCertsFromPEM(pem) {
		return badConnectOption(fmt.Errorf("call to AppendCertsFromPEM returned false"))
	}
	return VerifyServerWith(ca)
}

// SendPSK sets the preshared-key for the connection.
func SendPSK(psk string) ConnectOption {
	return func(c *connectOptions) error {
		c.psk = psk
		return nil
	}
}

// UBJSON enables the binary UBJSON protocol.
func UBJSON(enable bool) ConnectOption {
	return func(c *connectOptions) error {
		c.enableUBJSON = enable
		return nil
	}
}

// Tracing enables the RPC tracing functionality.
func Tracing(enable bool) ConnectOption {
	return func(c *connectOptions) error {
		c.enableTracing = enable
		return nil
	}
}

func JunkHandler(junkHandler func(junk []byte)) ConnectOption {
	return func(c *connectOptions) error {
		c.junkHandler = junkHandler
		return nil
	}
}

func Reconnect(enable bool) ConnectOption {
	return func(c *connectOptions) error {
		c.enableReconnect = enable
		return nil
	}
}

func badConnectOption(err error) ConnectOption {
	return func(_ *connectOptions) error {
		return err
	}
}

// ListenerConfig specifies a listener that receives RPC connections.
type ListenerConfig struct {
	// Address suitable for net.Listener (currently only TCP).
	Addr string `yaml:"addr"`
	// If present, connections will be wrapped in a TLS wrapper and this field
	// specifies the configuration.
	TLS *TLSConfig `yaml:"tls,omitempty"`

	// Which higher level protocol will be used to transport RPC.
	// Exactly one of these needs to be configured per listener.

	// TCP listener sends RPC frames over plain TCP.
	TCP *TCPListenerConfig `yaml:"tcp,omitempty"`
	// HTTP listener uses HTTP POST requests to send RPC frames.
	// HTTP listener also supports WebSocket connections.
	HTTP *HTTPListenerConfig `yaml:"http,omitempty"`

	EnableTracing bool `yaml:"enable_tracing"`
}

// TLSConfig configures the TLS listener wrapper.
type TLSConfig struct {
	// Certificate and key to be used by the server. Either a parsed Cert or file names for both
	// must be provided. It's ok to have certificate and key in the same file.
	Cert     *tls.Certificate `yaml:"-"`
	CertFile string           `yaml:"cert_file,omitempty"`
	KeyFile  string           `yaml:"key_file,omitempty"`
	// CA certificates to use to validate clients.
	// This enables requesting client certificate during handshake, which is off by default.
	// A pre-existing pool or a file to load from can be specified (but not both).
	ClientCAPool     *x509.CertPool `yaml:"-"`
	ClientCAPoolFile string         `yaml:"client_ca_pool_file,omitempty"`
}

// HTTPListenerConfig configures the HTTP and WebSocket listener.
type HTTPListenerConfig struct {
	// Accept RPC frames in POST requests.
	EnablePOST bool `yaml:"enable_post,omitempty"`
	// Allow upgrading to a WebSocket connection.
	// Note: Currently both POST and WebSocket cannot be used on the same listener.
	EnableWebSocket bool `yaml:"enable_websocket,omitempty"`
	// Cloud host base. If set, then for REST-like requests destination can be
	// specified in the Host header, and will be derived by stripping this suffix.
	CloudHost string `yaml:"cloud_host,omitempty"`
}

// TCPListenerConfig is a TCP listener configuration.
type TCPListenerConfig struct {
}

// ListenerConfigFromURL offers a quick way to create ListenerConfig from URL, e.g. http://:8081.
func ListenerConfigFromURL(urlStr string) (ListenerConfig, error) {
	lc := ListenerConfig{}
	url, err := url.Parse(urlStr)
	if err != nil {
		return lc, errors.Annotatef(err, "invalid listen address %s", urlStr)
	}
	lc.Addr = url.Host
	switch url.Scheme {
	case "https":
		lc.TLS = &TLSConfig{}
		fallthrough
	case "http":
		lc.HTTP = &HTTPListenerConfig{EnablePOST: true}
	case "wss":
		lc.TLS = &TLSConfig{}
		fallthrough
	case "ws":
		lc.HTTP = &HTTPListenerConfig{EnableWebSocket: true}
	case "tcps":
		lc.TLS = &TLSConfig{}
		fallthrough
	case "tcp":
		lc.TCP = &TCPListenerConfig{}
	default:
		err = errors.Errorf("unknown listen protocol %q", url.Scheme)
	}
	return lc, errors.Trace(err)
}

// ListenOption implements the option pattern for the Listen function
type ListenOption func(*ListenerConfig)

// ServerCert specifies the certificate to present to remote peers
// and enables TLS.
func ServerCert(cert *tls.Certificate) ListenOption {
	return func(c *ListenerConfig) {
		if c.TLS == nil {
			c.TLS = &TLSConfig{}
		}
		c.TLS.Cert = cert
	}
}

// ServerCertFiles will load certificate and key from the specified files
// to be used by the server and enables TLS.
// If file names are empty, this option does nothing.
func ServerCertFiles(certFile, keyFile string) ListenOption {
	return func(c *ListenerConfig) {
		if c.TLS == nil {
			c.TLS = &TLSConfig{}
		}
		c.TLS.CertFile = certFile
		c.TLS.KeyFile = keyFile
	}
}

// VerifyClientsWith specifies the pool of CA certificates to use for validating
// client certificates.
func VerifyClientsWith(pool *x509.CertPool) ListenOption {
	return func(c *ListenerConfig) {
		if c.TLS == nil {
			c.TLS = &TLSConfig{}
		}
		c.TLS.ClientCAPool = pool
	}
}

// VerifyClientsWithCAsFromFile is a wrapper for VerifyClientsWith that reads
// CA certificates from file.
func VerifyClientsWithCAsFromFile(file string) ListenOption {
	return func(c *ListenerConfig) {
		if c.TLS == nil {
			c.TLS = &TLSConfig{}
		}
		c.TLS.ClientCAPoolFile = file
	}
}

// ListenerTracing enables the RPC tracing functionality.
func ListenerTracing(enable bool) ListenOption {
	return func(c *ListenerConfig) {
		c.EnableTracing = enable
	}
}
