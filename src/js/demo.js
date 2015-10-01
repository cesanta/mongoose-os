if (typeof(conf.user) == 'undefined') {
  print("Initializing demo data source")
  t = {};
  t.getTemp = function() { return 20+Math.random()*20 };
} else if (conf.user.demo == 'MCP9808') {
  File.eval('I2C.js');
  File.eval('MCP9808.js');
  t = new MCP9808(new I2C(14,12), MCP9808.addr(1,1,1));
} else if (conf.user.demo == 'MC24FC') {
  File.eval('I2C.js');
  File.eval('MC24FC.js');
  File.eval('MC24FC_test.js');
}

function push(n, cb) {
  Cloud.store("temperature", n, {labels: {"sensor": "1"}, cb: cb});
}

function demo() {
  Debug.print("Pushing metric")
  push(t.getTemp(), function(d, e) {
  if (e) {
    Debug.print("Error sending to the cloud:", e);
  } else {
    Debug.print("Cloud reply:", d);
  }
  setTimeout(demo, 2000)});
};

function startDemo() {
  print("Starting temperature push demo in background, type Debug.mode(1) to see activity");
  demo();
}
