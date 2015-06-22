/* http://cesanta.com/docs/smartjs/drivers/mcf24fc */
function MC24FC(sda,scl,chn) {
  var c = i2c.init(sda,scl);
  var chn = chn;

  function read_s(a,sz) {
    var a0_15 = a & 65535;
    var a16 = (a & 65536) >> 16
    var cb = 10 << 4 | a16 << 3 | chn << 1;

    c.start();
    c.sendByte(cb);
    c.sendWord(a0_15);
    c.start();
    c.sendByte(cb | 1);

    var ret = c.readString(sz);

    c.stop();

    return ret;
  }

  function write_b(cb,a0_15,a16,str,pos,sz) {
    c.start();

    if(c.sendByte(cb) != 0) {
      return 0;
    }

    if(c.sendWord(a0_15) != 0) {
      return 0;
    }

    if(c.sendString(str.slice(pos, pos + sz)) != 0) {
      return 0;
    }

    c.stop();

    var i;
    for (i = 0; i <= 100; i++) {
      c.start();
      if(c.sendByte(cb) == 0) {
        break;
      }
    }

    return i != 100;
  }

  function write_s(a,str,pos,sz) {
    var a0_15 = a & 65535;
    var a16 = (a & 65536) >> 16
    var cb = 10 << 4 | a16 << 3 | chn << 1

    var s, n;
    s = n = sz;

    var page_a = a0_15 & -128;
    var page_space = page_a + 128 - a0_15;
    if (page_space < n) {
      n = page_space;
    }

    if (write_b(cb,a0_15,a16,str,pos,n) == 0) {
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

      if (write_b(cb,a0_15,a16,str,pos,n) == 0) {
        return 0;
      }

      s -= n;
      pos += n;
      a0_15 += n;
    }

    return sz;
  }

  this.read = function(a,sz) {
    var r;

    if (a + sz > 131071 ) {
      return -1;
    }

    if (a <= 65535 &&
      a + sz > 65536) {
      r = 65535 - a + 1;
      return read_s(a, r) +
             read_s(a + r, sz - r);
    } else {
      return read_s(a, sz);
    }
  }

  this.write = function (a,str) {
    var n, s = str.length;

    if (a + s > 131071) {
      return -1;
    }

    if (a <= 65535 &&
        a + s > 65536) {
      n = 65535 - a + 1;
      return write_s(a, str, 0, n) +
       write_s(a + n, str, n, s - n);
    } else {
      return write_s(a, str, 0, s);
    }

    return 0;
  }
}
