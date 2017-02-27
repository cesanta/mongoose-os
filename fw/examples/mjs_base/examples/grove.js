// This examples demonstrates how to use such Goove Kit components as
// Button, Motion Sensor, Temperature Sensor and UV Sensor

// Load Mongoose OS API
load('api_grove.js');

// Use any free GPIO
let buttonGPIO = 27;
let motionSensorGPIO = 26;
let lightSensorGPIO = 36;
let moistureSensorGPIO = 35;
let uvSensorGPIO = 39;

Grove.Button.attach(buttonGPIO, function() {
  print("Light sensor data:", Grove.LightSensor.get(lightSensorGPIO));
  print("Moisture sensor data:", Grove.MoistureSensor.get(moistureSensorGPIO));
  print("UV sensor data:", Grove.UVSensor.get(uvSensorGPIO));
});

Grove.MotionSensor.attach(motionSensorGPIO, function() {
  print("Big brother is watching you!");
});
