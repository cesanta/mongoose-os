function testMCP9808() {
  var t = new MCP9808(new I2C(14, 12), MCP9808.addr(1, 1, 1));
  print(t.getTemp());
}
