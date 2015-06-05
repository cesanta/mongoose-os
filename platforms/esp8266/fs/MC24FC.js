/*
 * MCF24FC EEPROM i2c JS wrapper sample
 * Usage: x = new MC24FC(12,14,1);
 *        x.write(0x100, "Hello, word!);
 *        print(x.read(0x100, 12))
 */
function MC24FC(sda,scl,chip_no) {
  var i2c_conn = i2c.init(sda, scl);
  var chip_no = chip_no;

  function read_s(addr, size) {
    var a0_15 = addr & 65535;
    var a16 = (addr & 65536) >> 16
    var cb = 10 << 4 | a16 << 3 | chip_no << 1;

    i2c_conn.start();
    i2c_conn.sendByte(cb);
    i2c_conn.sendWord(a0_15);
    i2c_conn.start();
    i2c_conn.sendByte(cb | 1);

    var ret = i2c_conn.readString(size);

    i2c_conn.stop();

    return ret;
  }

  function write_b(cb, a0_15, a16, str, pos, size) {
    i2c_conn.start();

    if(i2c_conn.sendByte(cb) != 0) {
      return 0;
    }

    if(i2c_conn.sendWord(a0_15) != 0) {
      return 0;
    }

    if(i2c_conn.sendString(str.slice(pos, pos + size)) != 0) {
      return 0;
    }

    i2c_conn.stop();

    var i;
    for (i = 0; i <= 100; i++) {
      i2c_conn.start();
      if(i2c_conn.sendByte(cb) == 0) {
        break;
      }
    }

    return i != 100;
  }

  function write_s(addr, str, pos, size) {
    var a0_15 = addr & 65535;
    var a16 = (addr & 65536) >> 16
    var cb = 10 << 4 | a16 << 3 | chip_no << 1

    var data_size, to_write;
    data_size = to_write = size;

    var page_addr = a0_15 & -128;
    var page_space = page_addr + 128 - a0_15;
    if (page_space < to_write) {
      to_write = page_space;
    }

    if (write_b(cb, a0_15, a16, str, pos, to_write) == 0) {
      return 0;
    }

    data_size -= to_write;
    pos += to_write;
    a0_15 += to_write;

    while (data_size > 0) {
      to_write = data_size;
      if (to_write > 128) {
        to_write = 128;
      }

      if (write_b(cb, a0_15, a16, str, pos, to_write) == 0) {
        return 0;
      }

      data_size -= to_write;
      pos += to_write;
      a0_15 += to_write;
    }

    return size;
  }

  this.read = function(addr,size) {
    var to_read;

    if (addr + size > 131071 ) {
      return -1;
    }

    if (addr <= 65535 &&
      addr + size > 65536) {
      to_read = 65535 - addr + 1;
      return read_s(addr, to_read) +
             read_s(addr + to_read, size - to_read);
    } else {
      return read_s(addr, size);
    }
  }

  this.write = function (addr,str) {
    var to_write, data_size = str.length;

    if (addr + data_size > 131071) {
      return -1;
    }

    if (addr <= 65535 &&
        addr + data_size > 65536) {
      to_write = 65535 - addr + 1;
      return write_s(addr, str, 0, to_write) +
       write_s(addr + to_write, str, to_write, data_size - to_write);
    } else {
      return write_s(addr, str, 0, data_size);
    }

    return 0;
  }
}