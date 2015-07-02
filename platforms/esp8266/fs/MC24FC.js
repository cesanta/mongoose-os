/* http://cesanta.com/docs/smartjs/drivers/mcf24fc */
function MC24FC(sda,scl,chn) {
  var c = new I2C(sda,scl);
  var chn = chn;

  function read_s(a,sz) {
    var a0_15 = a & 0xFFFF;
    var a16 = (a & 0x10000) >> 16;
    var i2c_addr = 0b1010000 | a16 << 2 | chn;

    if (c.start(i2c_addr, I2C.WRITE) != I2C.ACK ||
        c.send(a0_15 >> 8) != I2C.ACK ||
        c.send(a0_15 & 0xFF) != I2C.ACK) {
      return "";
    }

    if (c.start(i2c_addr, I2C.READ) != I2C.ACK) {
      return "";
    }

    var ret = c.readString(sz);

    c.stop();

    return ret;
  }

  function write_b(a0_15,a16,str,pos,sz) {
    var i2c_addr = 0b1010000 | a16 << 2 | chn;

    if (c.start(i2c_addr, I2C.WRITE) != I2C.ACK) {
      return 0;
    }

    if(c.send(a0_15 >> 8) != I2C.ACK ||
       c.send(a0_15 & 0xFF) != I2C.ACK) {
      return 0;
    }

    if(c.send(str.slice(pos, pos + sz)) != I2C.ACK) {
      return 0;
    }

    c.stop();

    // Wait for write to complete.
    var i = 0;
    while (c.start(i2c_addr, I2C.WRITE) != I2C.ACK && i < 100) {
      i++;
    }

    c.stop();

    return i != 100;
  }

  function write_s(a,str,pos,sz) {
    var a0_15 = a & 0xFFFF;
    var a16 = (a & 0x10000) >> 16

    var s, n;
    s = n = sz;

    var page_a = a0_15 & -128;
    var page_space = page_a + 128 - a0_15;
    if (page_space < n) {
      n = page_space;
    }

    if (write_b(a0_15,a16,str,pos,n) == 0) {
      return 0;
    }

    s -= n;
    pos += n;
    a0_15 += n;

    while (s > 0) {
      n = s;
      if (n > 128) {
        n = 128;
      }

      if (write_b(a0_15,a16,str,pos,n) == 0) {
        return 0;
      }

      s -= n;
      pos += n;
      a0_15 += n;
    }

    return sz;
  }

  this.read = function(a,sz) {

    if (a + sz > 0x1FFFF) {
      return -1;
    }

    if (a <= 0xFFFF &&
      a + sz > 0x10000) {
      var r = 0xFFFF - a + 1;
      return read_s(a, r) +
             read_s(a + r, sz - r);
    } else {
      return read_s(a, sz);
    }
  }

  this.write = function (a,str) {
    var n, s = str.length;

    if (a + s > 0x1FFFF) {
      return -1;
    }

    if (a <= 0xFFFF &&
        a + s > 0x10000) {
      n = 0xFFFF - a + 1;
      return write_s(a, str, 0, n) + write_s(a + n, str, n, s - n);
    } else {
      return write_s(a, str, 0, s);
    }

    return 0;
  }
}
