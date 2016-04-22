// This file does not provide a new object, but modifies the prototype of
// objects constructed with `new I2C(...)`, so `this` refers to that particular
// object.

I2C.prototype.read = function(address, nbytes) {
  var r = this.start(address, I2C.READ);
  if (r != I2C.ACK) return I2C.ERR;
  r = this.readString(nbytes);
  this.stop();
  return r;
};

I2C.prototype.write = function(address, data) {
  var r = this.start(address, I2C.WRITE);
  if (r != I2C.ACK) return I2C.ERR;
  r = this.send(data);
  this.stop();
  return r;
};

I2C.prototype.do = function(address) {
  var r = [];
  for (var i = 1; i < arguments.length; i++) {
    var op = arguments[i];
    if (op[0] == I2C.READ) {
      var nb = op[1];
      var t = this.start(address, I2C.READ);
      if (t == I2C.ACK) {
        if (op.length >= 3) {
          var v = op[2]; // XXX: inlining this makes V7 unhappy
          r.push(this.readString(nb, v));
        } else {
          r.push(this.readString(nb, I2C.NAK));
        }
      } else {
        r.push(I2C.ERR);
        break;
      }
    } else if (op[0] == I2C.WRITE) {
      var data = op[1];
      var t = this.start(address, I2C.WRITE);
      if (t == I2C.ACK) {
        var er = I2C.ACK;
        if (op.length >= 3) {
          er = op[2];
        }
        var v = this.send(data);
        if (v == er) {
          r.push(v);
        } else {
          r.push(I2C.ERR);
          break;
        }
      } else {
        r.push(I2C.ERR);
        break;
      }
    } else {
      r.push(I2C.ERR);
      break;
    }
  }
  this.stop();
  return r;
};

/* Register interface, a-la SMBus. */
I2C.prototype._writeRegAddr = function(address, reg, mode) {
  if (this.start(address, I2C.WRITE) != I2C.ACK) return -1;
  if (this.send(reg & 0xff) != I2C.ACK) return -2;
  if (this.start(address, mode) != I2C.ACK) return -3;
  return 0;
}

I2C.prototype.readRegB = function(address, reg) {
  var b = this._writeRegAddr(address, reg, I2C.READ);
  if (b < 0) return b;
  b = this.readByte();
  this.stop();
  return b;
}

I2C.prototype.writeRegB = function(address, reg, b) {
  var r = this._writeRegAddr(address, reg, I2C.WRITE);
  if (r < 0) {
    this.stop();
    return r;
  }
  r = this.send(b);
  this.stop();
  return r == I2C.ACK ? 0 : -4;
}

I2C.prototype.readRegW = function(address, reg) {
  var r = this._writeRegAddr(address, reg, I2C.READ);
  if (r < 0) return r;
  var bl = this.readByte();
  if (bl < 0) return -4;
  var bh = this.readByte();
  if (bh < 0) return -5;
  this.stop();
  return (bh << 8) | bl;
}
