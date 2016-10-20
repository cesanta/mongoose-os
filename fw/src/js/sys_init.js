"use strict";

print('\nMongoose IoT Firmware - see documentation at',
      'https://docs.cesanta.com/mongoose-iot/\n');

if (File.exists('app.js')) {
  File.eval('app.js');
} else if (File.exists('demo.js')) {
  print('No app.js, running demo...');
}
