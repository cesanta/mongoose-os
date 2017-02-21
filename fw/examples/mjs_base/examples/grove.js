// This examples demonstrates how to use such Goove Kit components as
// Button, Motion Sensor and Temperature Sensor

// Load Mongoose OS API
load('api_grove.js');

// Use any free GPIO
let buttonGPIO = 27;
let motionSensorGPIO = 26;

Grove.Button.attach(buttonGPIO, function() {
  print("Clicked!");
});

print("Press button to see current temperature");

Grove.MotionSensor.attach(motionSensorGPIO, function() {
  print("Big brother is watching you!");
});
