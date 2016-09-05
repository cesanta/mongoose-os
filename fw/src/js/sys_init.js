"use strict";

print('\nMongoose IoT Firmware - see documentation at',
      'https://docs.cesanta.com/mongoose-iot/\n');

if (Sys.conf.clubby.device_id) {
    print('Device credentials: ', {
      device_psk: Sys.conf.clubby.device_psk,
      device_id: Sys.conf.clubby.device_id,
    }, '\n');
}

//TODO(dfrank): implement these handlers in C. For now, they're just demo stubs
// {{{
clubby.oncmd("/v1/Vars.Get", function(data) {
  return {
    mac: "131313131313",
  };
});

clubby.oncmd("/v1/Config.Get", function(data) {
  print("Getting config")
  return Sys.conf;
});

clubby.oncmd("/v1/Config.Set", function(data) {
  print("TODO: set config: " + JSON.stringify(data))
});

clubby.oncmd("/v1/Config.Save", function(data) {
  print("TODO: save config")
});
// }}}

if (File.exists('app.js')) {
  File.eval('app.js');
} else if (File.exists('demo.js')) {
  print('No app.js, running demo...');
  File.eval('demo.js');
}
