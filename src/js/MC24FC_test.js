function testMC24FC() {
  var c = new MC24FC(new I2C(14, 12));
  print(c.write(0, "Kittens"));
  print(c.write(7, " > "));
  print(c.write(10, "Puppies"));
  print(c.read(0, 17));
  print(c.write(0, "Puppies"));
  print(c.write(7, " < "));
  print(c.write(10, "Kittens"));
  print(c.read(0, 17));
}
