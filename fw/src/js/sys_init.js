"use strict";

print('\nMongoose IoT Firmware - see documentation at',
      'https://docs.cesanta.com/mongoose-iot/\n');

if (Sys.conf.device.id) {
    print('Device credentials: ', {
      device_psk: Sys.conf.device.password,
      device_id: Sys.conf.device.id,
    }, '\n');
}

if (File.exists('app.js')) {
  File.eval('app.js');
} else if (File.exists('demo.js')) {
  print('No app.js, running demo...');
  File.eval('demo.js');
}
