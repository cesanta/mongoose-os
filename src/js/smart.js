// initialization.
// Don't store functions declared here in the object graph
// this way the AST for this module is thrown away after init
// put the rest in sys_rts.js

File.eval("sys_rts.js");

conf = File.loadJSON('sys_config.json');
if (typeof(conf) == 'undefined') {
  conf = {
    dev: {
      // TODO(lsm): refactor to initialize ID and PSK lazily on first
      // clubby request, get those from the cloud
      id: '//api.cesanta.com/d/dev_' + Math.random().toString(36).substring(7),
      key: 'psk_' + Math.random().toString(36).substring(7)
    },
    cloud: '//api.cesanta.com'
  };
}
SJS.init();
conf.save();

print('')
print('Device id: ' + conf.dev.id);
print('Device psk: ' + conf.dev.key);
print('Cloud: ' + conf.cloud);

File.eval("user.js")
