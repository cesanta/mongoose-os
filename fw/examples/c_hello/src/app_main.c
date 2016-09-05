#include <stdio.h>

#include "common/platform.h"
#include "fw/src/sj_app.h"
#include "fw/src/sj_gpio.h"
#include "fw/src/sj_sys_config.h"

#if CS_PLATFORM == CS_P_ESP_LWIP
#define GPIO 12
#elif CS_PLATFORM == CS_P_CC3200
#define GPIO 64 /* The red LED on LAUNCHXL */
#else
#error Unknown platform
#endif

enum mg_app_init_result sj_app_init(void) {
  { /* Print a message using a value from config. */
    printf("Hello, %s!\n", get_cfg()->hello.who);
  }

  { /* Turn on LED. */
    sj_gpio_set_mode(GPIO, GPIO_MODE_OUTPUT, GPIO_PULL_FLOAT);
    sj_gpio_write(GPIO, GPIO_LEVEL_HIGH);
  }

  { /* Read a file. */
    FILE *fp = fopen("README.txt", "r");
    if (fp != NULL) {
      char buf[100];
      int n = fread(buf, 1, sizeof(buf), fp);
      if (n > 0) {
        fwrite(buf, 1, n, stdout);
      }
      fclose(fp);
    }
  }

  return MG_APP_INIT_SUCCESS;
}
