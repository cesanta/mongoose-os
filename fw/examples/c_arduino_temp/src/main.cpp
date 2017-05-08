/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <OneWire.h>
#include "DS18B20.h"

#define ONEWIRE_PIN 13  // 1-Wire bus is plugged into GPIO13
#define NUM_SENS 2      // Number of sensors on the 1-Wire bus

OneWire *ow = NULL;
DS18B20 *ds = NULL;

// ROM-number is the sensors 64-bit unique registration number 
// in read-only memory (ROM). You can find it with search() function as shown below.
static const unsigned char sens[NUM_SENS][8] = { {0x28,0xe6,0x17,0xa1,0x08,0x00,0x00,0xab},
                                                 {0x28,0x3d,0xd8,0xa1,0x08,0x00,0x00,0xda} };

void setup(void) {
  printf("Arduino DS18B20 very simple driver example\n");

  //Initialize 1-Wire bus
  ow = new OneWire(ONEWIRE_PIN);

  // Pass a OneWire handle to DS18B20
  ds = new DS18B20(ow);

  /*
  int n = 0;
  unsigned char addr[NUM_SENS][8];

  // Search DS18B20 sensors on a 1-Wire bus
  if ((n = ds->search(addr, NUM_SENS)) == 0) {
    printf("Can't find sensors\n");
    return;
  } else {
    printf("Sensors found: %d\n", n);
  }

  for (int i = 0; i < n; i++) {
    printf("Sens#%d address:\n", i+1);
    for (int j = 0; j < 8; j++) {
      printf("%x\n", addr[i][j]);
    }
  }
  */
}

void loop(void) {
  // This function reads temperature from the DS18B20 sensors
  for (int i = 0; i < NUM_SENS; i++) {
    if (ds->select(sens[i])) {
      printf("Sens#%d Temperature: %f *C\n", i+1, ds->getTemp());
    } else {
      printf("Can't find Sens#%d\n", i+1);
    }
  }
}
