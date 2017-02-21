// This examples demonstrates how to use such Goove Kit components as
// Button, Motion Sensor and Temperature Sensor
// See https://www.seeedstudio.com/Grove-Indoor-Environment-Kit-for-Intel%C2%AE-Edison-p-2427.html

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
