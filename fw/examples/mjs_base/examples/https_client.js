// Load Mongoose OS API
load('api_gpio.js');
load('api_http.js');

// On nodemcu, press flash button to send HTTPS request
GPIO.set_button_handler(0, GPIO.PULL_UP, GPIO.INT_EDGE_NEG, 200, function() {
  print('sending request...');
  HTTP.query({
    url: 'https://httpbin.org/get',
    success: function(body, full_http_msg) { print(body); },
    error: function(err) { print(err); }  // Optional
  });
}, null);
