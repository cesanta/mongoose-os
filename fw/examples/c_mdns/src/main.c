#include <stdio.h>

#include "common/mg_str.h"
#include "fw/src/miot_app.h"
#include "fw/src/miot_dns_sd.h"
#include "fw/src/miot_mongoose.h"

enum miot_app_init_result miot_app_init(void) {
  miot_sd_register_service(
      mg_mk_str("_myservice._tcp"), mg_mk_str("foobar"),
      mg_mk_str("foobar.local"), 80,
      (struct mg_str[]){
          mg_mk_str("somelabel"), mg_mk_str("other"), mg_mk_str(NULL),
      },
      (struct mg_str[]){
          mg_mk_str("somevalue"), mg_mk_str("value"), mg_mk_str(NULL),
      });

  return MIOT_APP_INIT_SUCCESS;
}
