load('api_gpio.js');
load('api_adc.js');

let Grove = {
  Button: {
    attach: function(pin, handler) {
      GPIO.set_button_handler(pin, GPIO.PULL_UP, GPIO.INT_EDGE_NEG, 200,
                              handler, true);
    },
  },
  _motionHandler: undefined,
  MotionSensor: {
    attach: function(pin, handler) {
      GPIO.set_mode(pin, GPIO.MODE_INPUT);
      GPIO.set_int_handler(pin, GPIO.INT_EDGE_POS, handler, null);
      GPIO.enable_int(pin);
      Grove._motionHandler = handler;
    },
  },
  LightSensor: {
    get: function(pin) {
      return ADC.read(pin);
    },
  },
  MoistureSensor: {
    get: function(pin) {
      return ADC.read(pin);
    },
  },
  UVSensor: {
    get: function(pin) {
      return ADC.read(pin);
    },
  },
  _relayInited: undefined,
  _relayClosed: 0,
  Relay: {
    _init: function(pin) {
      if (Grove._relayInited !== 1) {
        GPIO.set_mode(pin, GPIO.MODE_OUTPUT);
        GPIO.set_pull(pin, GPIO.PULL_DOWN);
        Grove._relayInited = 1;
      }
    },
    close: function(pin) {
      this._init(pin);
      GPIO.write(pin, 1);
      Grove._relayClosed = 1;
    },
    open: function(pin) {
      this._init(pin);
      GPIO.write(pin, 0);
      Grove._relayClosed = 0;
    },
    isClosed: function(pin) {
      return Grove._relayClosed;
    },
  },
};
