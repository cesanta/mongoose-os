// Load Mongoose OS API
load('api_timer.js');
load('api_uart.js');
load('api_sys.js');

// Uart number used for this example
let uartNo = 1;
// Accumulated Rx data, will be echoed back to Tx
let rxAcc = "";

let value = false;

// Configure UART at 115200 baud
UART.setConfig(uartNo, {
  baudRate: 115200,
  esp32: {
    gpio: {
      rx: 16,
      tx: 17,
    },
  },
});

// Set dispatcher callback, it will be called whenver new Rx data or space in
// the Tx buffer becomes available
UART.setDispatcher(uartNo, function(uartNo, ud) {
  let ra = UART.readAvail(uartNo);
  if (ra > 0) {
    // Received new data: print it immediately to the console, and also
    // accumulate in the "rxAcc" variable which will be echoed back to UART later
    let data = UART.read(uartNo);
    print("Received UART data:", data);
    rxAcc += data;
  }
}, null);

// Enable Rx
UART.setRxEnabled(uartNo, true);

// Send UART data every second
Timer.set(1000 /* milliseconds */, true /* repeat */, function() {
  value = !value;
  UART.write(
    uartNo,
    "Hello UART! "
      + (value ? 'Tick' : 'Tock')
      + " uptime: " + JSON.stringify(Sys.uptime())
      + " RAM: " + JSON.stringify(Sys.free_ram())
      + (rxAcc.length > 0 ? (" Rx: " + rxAcc) : "")
      + "\n"
  );
  rxAcc = "";
}, null);
