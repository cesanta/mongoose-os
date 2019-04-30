# PPPoS / cellullar modem support

This library provides IP over serial port. Encapsulation is PPP.

## Settings

```
  "pppos": {
    "enable": false,                # Enable PPPoS
    "uart_no": 1,                   # Which UART to use.
    "baud_rate": 115200,            # Baud rate, data mode is 8-N-1.
    "fc_enable": false,             # Enable hardware CTS/RTS flow control
    "apn": "",                      # APN name
    "user": "",                     # User name
    "pass": "",                     # Password
    "connect_cmd": "ATDT*99***1#",  # AT command to send to initiate PPP data mode
    "echo_interval": 10,            # LCP Echo interval, seconds
    "echo_fails": 3,                # Number of failed echos before connection is declared dead are retried
    "hexdump_enable": false         # Dump all the data sent over UART to stderr
  }
```

Default UART pin assignments are used and they can be found [here](https://github.com/cesanta/mongoose-os/blob/aa89bc237e4ba492e791a069617a5c6f74ee63f4/fw/platforms/esp32/src/esp32_uart.c#L228).

## Example configuration

Access Point Name, PPP username and password depend on the operator. They are usually public and can be found [here](http://wiki.apnchanger.org/Main_Page).

Here's an example for Vodafone Ireland:

```
"pppos": {
  "enable": true,
  "uart_no": 1,
  "apn": "live.vodafone.ie",
  "user": "dublin",
  "pass": "dublin",
}
```
