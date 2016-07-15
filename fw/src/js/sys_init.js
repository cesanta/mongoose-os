"use strict";

print('\nMongoose IoT Firmware - see documentation at',
      'https://docs.cesanta.com/mongoose-iot/\n');

if (Sys.conf.clubby.device_id) {
    print('Device credentials: ', {
      device_psk: Sys.conf.clubby.device_psk,
      device_id: Sys.conf.clubby.device_id,
    }, '\n');
}

global.clubby = new Clubby({connect:false});

if (Sys.conf.clubby.device_id && Sys.conf.clubby.connect_on_boot) {
  if (Wifi.status() !== undefined) {
    // Wifi has some well-defined status; therefore, Wifi is usable at the
    // current platform
    Wifi.ready(clubby.connect.bind(clubby))
  } else {
    // Wifi management is not supported at the current platform: assume it uses
    // external networking and if connect_on_boot=true we can connect
    // immediately
    clubby.connect()
  }
}

if (File.exists('app.js')) {
  File.eval('app.js');
} else if (File.exists('demo.js')) {
  print('No app.js, running demo...');
  File.eval('demo.js');
}
