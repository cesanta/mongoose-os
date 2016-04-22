/* http://cesanta.com/docs/fw/drivers/mcf24fc */
function MC24FC(bus) {
  function caddr(addr) {
    var s = (addr & 0x70000) >> 16;
    return MC24FC.addr_template | (s&1) << 2 | (s&2) | (s&4) >> 2;
  }

  function of2be(offset) {
    return String.fromCharCode(offset >> 8, offset & 0xFF);
  }

  function read1(addr, len) {
    var r = [I2C.ERR];
    var count = 0;
    // Device disappears from the bus during the write cycle, so try
    // a few times.
    while (r[0] == I2C.ERR && count < 100) {
      r = bus.do(caddr(addr),
        [I2C.WRITE, of2be(addr & 0xFFFF)],
        [I2C.READ, len]);
      count++;
    }
    if (r[r.length - 1] == I2C.ERR) {
      return "";
    }
    return r[1];
  }

  function wrPg(addr, data) {
    var r = I2C.ERR;
    var count = 0;
    // Device disappears from the bus during the write cycle, so try
    // a few times.
    while (r == I2C.ERR && count < 100) {
      r = bus.write(caddr(addr), of2be(addr & 0xFFFF) + data);
      count++;
    }
    return r;
  }

  this.read = function(addr, len) {
    var r = "";
    var tail = 0;
    addr &= 0x7FFFF;
    while (len > 0) {
      tail = (addr & 0x70000) + 0x10000 - addr;
      if (tail > len) { tail = len; }
      r += read1(addr, tail);
      len -= tail;
      addr += tail;
    }
    return r;
  }

  this.write = function(addr, data) {
    var p = 0;
    var tail = 0;
    addr &= 0x7FFFF;
    while (p < data.length) {
      tail = (addr & 0x7FF80) + 128 - addr;
      if (tail > data.length - p) { tail = data.length - p; }
      if (wrPg(addr, data.slice(p, p + tail)) != I2C.ACK) {
        return p;
      }
      p += tail;
      addr += tail;
    }
    return p;
  }
}

MC24FC.addr_template = 0x50;
