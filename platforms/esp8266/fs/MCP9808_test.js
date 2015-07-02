function testMCP9808() {
  var t = new MCP9808(14, 12, 1, 1, 1);
  print(t.getTemp());
}
