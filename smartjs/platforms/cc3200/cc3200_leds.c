#include "cc3200_leds.h"

#include "hw_types.h"
#include "hw_memmap.h"
#include "gpio.h"
#include "pin.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"

void cc3200_leds_init() {
  MAP_PRCMPeripheralClkEnable(PRCM_GPIOA1, PRCM_RUN_MODE_CLK);
  MAP_PinTypeGPIO(PIN_64, PIN_MODE_0, false); /* Amber LED */
  MAP_PinTypeGPIO(PIN_01, PIN_MODE_0, false); /* Red LED */
  MAP_PinTypeGPIO(PIN_02, PIN_MODE_0, false); /* Green LED */
  MAP_GPIODirModeSet(GPIOA1_BASE, RED | GREEN | AMBER, GPIO_DIR_MODE_OUT);
  cc3200_leds(RED | GREEN | AMBER, OFF);
}

void cc3200_leds(enum cc3200_led leds, enum cc3200_led_state s) {
  long v;
  leds &= (RED | GREEN | AMBER);
  switch (s) {
    case ON:
      v = ~0;
      break;
    case OFF:
      v = 0;
      break;
    case TOGGLE:
      v = ~(MAP_GPIOPinRead(GPIOA1_BASE, leds));
      break;
    default:
      return;
  }
  MAP_GPIOPinWrite(GPIOA1_BASE, leds, (v & leds));
}
