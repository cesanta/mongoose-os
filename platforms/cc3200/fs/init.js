File.eval("I2C.js");
File.eval("TMP006.js");

print("HELO! Type some JS. See https://github.com/cesanta/smart.js for more info.");

{
  // There is a temp sensor on the LAUNCHXL, try to read it.
  var i2c = new I2C();
  var t = (new TMP006(i2c, 0x41)).getTemp();
  if (t !== undefined) print("It's", t, "degrees outside.");
  i2c.close();
}
