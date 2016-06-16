// E.g.: testMC24FC(new MC24FC(new I2C(14, 12)));
function testMC24FC(c) {
  print(c.write(0, "Kittens"));
  print(c.write(7, " > "));
  print(c.write(10, "Puppies"));
  print(c.read(0, 17));
  print(c.write(0, "Puppies"));
  print(c.write(7, " < "));
  print(c.write(10, "Kittens"));
  print(c.read(0, 17));
}
