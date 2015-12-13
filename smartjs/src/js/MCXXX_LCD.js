/*
 * Midas MCXXX LCD driver.
 * Datasheet: http://goo.gl/3C5rR2
 */

function MCXXX(i2c, numLines, numCols, resetPin) {
  var icon_on = false;
  var lines = [];

  this.init = function(display_on, cursor_on, cursor_blink, contrast) {
    if (resetPin !== undefined) {
      GPIO.setmode(resetPin, 2, 0);
      GPIO.write(resetPin, 0);
      GPIO.write(resetPin, 1);
    }
    return sendInsn([
      I_FCS(2, false, 1),
      I1_SET_OSC_FREQ(0, 4),
      I1_PICS(icon_on, true, contrast >> 4),
      I1_SET_CONTRAST(contrast & 0xf),
      I1_FOLLOWER_CTL(true, 7),
      I_DISPLAY_ON(display_on, cursor_on, cursor_blink),
      I_CLS()
    ]);
  };

  this.cls = function() {
    return sendInsn([I_CLS()]);
  }

  this.setContrast = function(contrast) {
    return sendInsn([
      I1_PICS(icon_on, true, contrast >> 4),
      I1_SET_CONTRAST(contrast & 0xf),
    ]);
  };

  this.setText = function(text) {
    lines = text.split('\n');
    while (lines.length > numLines) lines.pop();
    return this._sendLines();
  }

  this.addLine = function(line) {
    var ll = line.split('\n');
    lines.push(ll[0]);
    while (lines.length > numLines) lines.shift();
    return this._sendLines();
  }

  // Some constants.
  var ADDR = 0x3e;
  var CB_IR = 0;
  var CB_DR = 0x40;

  // Helpers for constructing instruction bytes.
  function I_CLS() { return 0x1; };
  function I_HOME() { return 0x2; };
  function I_MOVE(screen, right) { return 0x4 | (right ? 2 : 0) | (screen & 1); };
  function I_DISPLAY_ON(display_on, con, cb) {
    return 0x8 | ((display_on & 1) << 2) | ((con & 1) << 1) | (cb & 1);
  };
  function I_FCS(n_lines, double_height, insn_table) {
    return 0x20 | 0x10 /* DL */ | ((n_lines == 2) << 3) |
           ((double_height & 1) << 2) | (insn_table & 1);
  };
  function I_SET_DRAM_ADDR(addr) { return 0x80 | (addr & 0x7f); };
  function I0_CD_SHIFT(sc, rl) { return 0x10 | ((sc & 1) << 3) | ((rl & 1) << 2); };
  function I0_SET_CGRAM_ADDR(addr) { return 0x40 | (addr & 0x3f); };
  function I1_SET_OSC_FREQ(bias, freq) { return 0x10 | ((bias & 1) << 3) | (freq & 0x7); };
  function I1_SET_ICON_ADDR(addr) { return 0x40 | (addr & 0xf); };
  function I1_PICS(icon_on, booster_on, contrast54) {
    return 0x50 | ((icon_on & 1) << 3) | ((booster_on & 1) << 2) | (contrast54 & 3);
  };
  function I1_FOLLOWER_CTL(follower_on, ratio) {
    return 0x60 | ((follower_on & 1) << 3) | (ratio & 7);
  };
  function I1_SET_CONTRAST(contrast) { return 0x70 | (contrast & 0xf); };

  function sendInsn(insn) {
    if (i2c.start(ADDR, I2C.WRITE) != I2C.ACK ||
        i2c.send(CB_IR) != I2C.ACK) {
      return false;
    }
    for (var i = 0; i < insn.length; i++) {
      if (i2c.send(insn[i]) != I2C.ACK) return false;
      /* According to the table in the datasheet, different instructions take
       * different time to execute at different clock speeds. 25 us should be
       * enough for all of them. */
      usleep(25);
    }
    i2c.stop();
    return true;
  }

  function sendData(data) {
    if (i2c.start(ADDR, I2C.WRITE) != I2C.ACK ||
        i2c.send(CB_DR) != I2C.ACK) {
      return false;
    }
    for (var i = 0; i < data.length; i++) {
      if (i2c.send(data.charCodeAt(i)) != I2C.ACK) return false;
      usleep(25);
    }
    i2c.stop();
    return true;
  };

  this._sendLines = function() {
    var curX = 0, curY = 0;
    for (var i = 0; i < numLines; i++) {
      var line = (lines[i] || '').substr(0, numCols);
      if (line.length > 0) {
        curX = Math.min(15, line.length);
        curY = i;
      }
      var numPad = numCols - line.length;
      while (numPad-- > 0) line += ' ';
      if (!sendInsn([I_DISPLAY_ON(true, false, false),
                     I_SET_DRAM_ADDR(i * 0x40)]) ||
          !sendData(line)) {
        return false;
      }
    }
    return sendInsn([I_DISPLAY_ON(true, true, true),
                     I_SET_DRAM_ADDR(0x40 * curY + curX)]);
  }
}
