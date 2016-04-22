// Button handler helper
// Usage:
// Connect button to required GPIO and GND
// Execute code:
// var gpio = 2;
// function button_pressed(pin,level) {
//   if (level==1) print("Button pressed"); };
// GPIO.onclick(gpio, button_pressed);
GPIO.onclick = function(pin, func) {
  GPIO.setMode(pin, 3, 2);
  GPIO.setISR(pin, 6, func);
}
