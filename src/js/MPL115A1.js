function MPL115A1() {
  var bus = new SPI();
  var a0 = convert(bus.tran(0x88,1) << 8 | bus.tran(0x8A,1), 1 << 3, 15, 0xFFFF);
  var b1 = convert(bus.tran(0x8C,1) << 8 | bus.tran(0x8E,1), 1 << 13, 15, 0xFFFF);
  var b2 = convert(bus.tran(0x90,1) << 8 | bus.tran(0x92,1), 1 << 14, 15, 0xFFFF);
  var c12 = convert((bus.tran(0x94,1) << 8 | bus.tran(0x96,1)) >> 2, 1 << 22, 13, 0x3FFF);

  function convert(n, fcoef, neg_pos, mask) {
    var res = 1;

    if ((n & (1 << neg_pos)) != 0) {
      n = ~n & mask;
      res = -1;
    }

    res *= (n / fcoef);
    return res;
  }

  this.getPressure = function () {
    bus.tran(0x24);
    usleep(3000);

    var p_adc = (bus.tran(0x80, 1) << 8 | bus.tran(0x82,1)) >> 6;
    var t_adc = (bus.tran(0x84, 1) << 8 | bus.tran(0x86,1)) >> 6;

    var p_comp = a0 + (b1 + c12*t_adc)*p_adc + b2 * t_adc;
    var pressure = p_comp * (115 - 50) / 1023 + 50;

    return pressure;
  }
}
