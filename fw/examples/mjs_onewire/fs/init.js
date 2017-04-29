// Load Mongoose OS API
load('api_timer.js');
load('api_onewire.js');

// GPIO pin which has a 1-Wire bus connected
let pin = 13;

// Initialize 1-Wire bus
let ow = OneWire.init(pin);

// This function reads data from the DS18B20 temperature sensors
// Datasheet: http://datasheets.maximintegrated.com/en/ds/DS18B20.pdf
let getTemp = function() {
  let n = 0;
  let rom = ['01234567'];
  let data = [];
  let raw;
  let cfg;
  let t = [-1000];
  
  //Setup the search to find the device type 'DS18B20' (0x28) if it is present
  let req_family_code = 0x28;
  //Normal search mode
  let search_mode = 0;
  
  OneWire.targetSetup(ow, req_family_code);

  while (OneWire.next(ow, rom[n], search_mode)) {
    //If no devices of the desired family are currently on the bus, 
    //then another type will be found. We should check it.
    if (rom[n][0].charCodeAt(0) !== req_family_code) {
      print('Unrequested device');
      return;
    }
    //DS18B20 found
    rom[++n] = '01234567';
  }
 
  if (n === 0) {
    print('No device found');
    return;
  }
  
  for (let i = 0; i < n; i++) {
    OneWire.reset(ow);
    OneWire.select(ow, rom[i]);
    OneWire.write(ow, 0x44);

    OneWire.delay(750);

    OneWire.reset(ow);
    OneWire.select(ow, rom[i]);
    OneWire.write(ow, 0xBE);

    for (let j = 0; j < 9; j++) {
      data[j] = OneWire.read(ow);
    }

    raw = (data[1] << 8) | data[0];
    cfg = (data[4] & 0x60);

    if (cfg === 0x00) { 
      raw = raw & ~7;
    } else if (cfg === 0x20) {
      raw = raw & ~3;
    } else if (cfg === 0x40) {
      raw = raw & ~1;
    }

    t[i] = raw / 16.0;
    print('T:');
    print(t[i]);
  }
};

// Get temperature every second
Timer.set(1000 /* milliseconds */, true /* repeat */, function() {
  getTemp();
}, null);
