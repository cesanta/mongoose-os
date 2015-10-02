// This file does not provide a new object, but modifies the prototype of
// objects constructed with `new I2C(...)`, so `this` refers to that particular
// object.

I2C.prototype.read = function(addr, nbytes) {
  var r = this.start(addr, I2C.READ);
  if (r != I2C.ACK) {
    this.stop();
    return I2C.ERR;
  }
  r = this.readString(nbytes);
  this.stop();
  return r;
};

I2C.prototype.write = function(addr, data) {
  var r = this.start(addr, I2C.WRITE);
  if (r != I2C.ACK) {
    this.stop();
    return I2C.ERR;
  }
  r = this.send(data);
  this.stop();
  return r;
};

I2C.prototype.do = function(addr) {
  var r = [];
  for (var i = 1; i < arguments.length; i++) {
    var op = arguments[i];
    if (op[0] == I2C.READ) {
      var nb = op[1];
      var t = this.start(addr, I2C.READ);
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
      var t = this.start(addr, I2C.WRITE);
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
