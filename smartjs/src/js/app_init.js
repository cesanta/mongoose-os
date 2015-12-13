// Put app-specific code here.
print('Executing app_init.js');

function demoSendRandomData() {
  function send() {
    var value  = 20 + Math.random() * 20;  // Simulate sensor data
    var vars = [{__name__: 'value'}, value];
    var frame = {
      v: 1, src: Sys.id, dst: '//api.cesanta.com', key: '', cmds: [
      {cmd: '/v1/Metrics.Publish', args: {vars: [[{__name__: 'value'}, value]]}}
    ]};
    Http.request({
      hostname: 'api.cesanta.com', method: 'POST'
    }, function() {}).end(JSON.stringify(frame));

    setTimeout(send, 2000);  // Call us every 2 seconds
  }
  send();
}
