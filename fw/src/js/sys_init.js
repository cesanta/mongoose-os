"use strict";

print('\nMongoose IoT Firmware - see documentation at',
      'https://mongoose-iot.com/docs/\n');

if (Sys.conf.device.id) {
    print('Device credentials: ', {
      device_psk: Sys.conf.device.password,
      device_id: Sys.conf.device.id,
    }, '\n');
}

if (File.exists('app.js')) {
  File.eval('app.js');
}
