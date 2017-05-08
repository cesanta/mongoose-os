load('api_gpio.js');
load('api_blynk.js');
load('api_sys.js');

Blynk.setHandler(function(conn, cmd, pin, val) {
  let ram = Sys.free_ram() / 1024;
  if (cmd === 'vr') {
  	// When reading any virtual pin, report free RAM in KB
    Blynk.virtualWrite(conn, pin, ram);
  } else if (cmd === 'vw') {
  	// Writing to virtual pin translate to writing to physical pin
    GPIO.set_mode(pin, GPIO.MODE_OUTPUT);
    GPIO.write(pin, val);
  }
  print('BLYNK JS handler, ram', ram, cmd, pin, val);
}, null);
