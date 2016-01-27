// Put app-specific code here.
print('Executing app_init.js');

function demoSendRandomData() {
  function send() {
    var value  = 20 + Math.random() * 20;  // Simulate sensor data
    clubby.call('//api.cesanta.com',
                {cmd: '/v1/Metrics.Publish',
                    args: {vars: [[{__name__: 'value'}, value]]}},
                function(res) {print("demoSendRandomData:", res)})

    setTimeout(send_on_ready, 2000);  // Call us every 2 seconds
  }

  function send_on_ready() {
    clubby.ready(send);
  }

  send_on_ready();
}
