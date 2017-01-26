package mgrpc

//go:generate stringer -type=transport transport.go

// transport is an enum of supported transport protocols
type transport int

const (
	// tHTTP_POST delivers the commands by issuing HTTP POST request and waiting
	// for the response.
	tHTTP_POST transport = iota
	// tWebSocket creates a permanent connection to the destination and
	// encapsulates Clubby frames into WebSocket frames.
	tWebSocket
	// tPlainTCP creates a permanent connection to the destination over plain TCP.
	tPlainTCP
	// tSerial creates a permanent connection to the destination over serial port.
	tSerial
)
