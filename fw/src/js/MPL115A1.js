function MPL115A1() {
  this.vars = {};
  var bus = new SPI();
  this.vars.a0 = convert(bus.tran(0x88,1) << 8 | bus.tran(0x8A,1), 1 << 3, 15, 0xFFFF);
  this.vars.b1 = convert(bus.tran(0x8C,1) << 8 | bus.tran(0x8E,1), 1 << 13, 15, 0xFFFF);
  this.vars.b2 = convert(bus.tran(0x90,1) << 8 | bus.tran(0x92,1), 1 << 14, 15, 0xFFFF);
  this.vars.c12 = convert((bus.tran(0x94,1) << 8 | bus.tran(0x96,1)) >> 2, 1 << 22, 13, 0x3FFF);

  function convert(n, fcoef, neg_pos, mask) {
    var res = 1;

    if ((n & (1 << neg_pos)) != 0) {
      n = ~n & mask;
      res = -1;
    }

    res *= (n / fcoef);
    return res;
  }

  this.vars.p_adc = 0;
  this.vars.t_adc = 0;

  this.getPressure = function () {
    bus.tran(0x24, 1);
    usleep(3000);

    this.vars.p_adc = (bus.tran(0x80, 1) << 8 | bus.tran(0x82,1)) >> 6;
    this.vars.t_adc = (bus.tran(0x84, 1) << 8 | bus.tran(0x86,1)) >> 6;

    var p_comp = this.vars.a0 +
                 (this.vars.b1 + this.vars.c12 * this.vars.t_adc) * this.vars.p_adc +
                 this.vars.b2 * this.vars.t_adc;
    var pressure = p_comp * (115 - 50) / 1023 + 50;

    return pressure;
  }
}
