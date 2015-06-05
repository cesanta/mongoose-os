/*
 * MCP9898 temperature sensor JS i2c wrapper sample
 * Usage: var t = new MCP9808(14,12,1,1,1); print(t.getTemp())
 */

function MCP9808(sda, scl, a0, a1, a2) {
  var i2c_conn = i2c.init(sda, scl);
  var ctrlByte = 3 << 4 | a2 << 3 | a1 << 2 | a0 << 1;

  this.getTemp = function () {
    i2c_conn.start();

    if (i2c_conn.sendByte(ctrlByte) != 0) {
      return -255;
    }

    if (i2c_conn.sendByte(5) != 0 ) {
      return -254;
    }

    i2c_conn.start();

    if (i2c_conn.sendByte(ctrlByte | 1) != 0) {
      return -253;
    }

    var temp_u, temp_l;

    temp_u = i2c_conn.readByte();
    i2c_conn.sendAck();
    temp_l = i2c_conn.readByte();
    i2c_conn.sendNack();
    i2c_conn.stop();

    temp_u &= 31;

    if ((temp_u & 16) != 0) {
      temp_u &= 15;
      return -(256-(temp_u*16 + temp_l/16));
    } else {
      return temp_u*16 + temp_l/16;
    }
  }

  this.stop = function () {
    i2c_conn.close();
    i2c_conn = null;
  }
}