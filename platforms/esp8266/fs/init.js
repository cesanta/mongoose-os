print("HELO! Type some JS. See https://github.com/cesanta/smart.js for more info.");

// NodeMCU has a button on GPIO0 ("Flash"), attach a handler to it.
// If there isn't one, it's harmless.
File.eval("gpio.js");
GPIO.onclick(0, function(_, down) {
  if (down) print("Ouch!");
});
