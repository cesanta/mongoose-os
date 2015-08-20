#include <malloc.h>
#include <string.h>

#include "cc3200_sj_hal.h"
#include "sj_hal.h"
#include "v7.h"

#include "oslib/osi.h"

extern OsiMsgQ_t s_prompt_input_q;

void sj_exec_with(struct v7 *v7, const char *code, v7_val_t this_obj) {
  struct prompt_event pe;
  struct v7_exec_event_data *ped = calloc(1, sizeof(*ped));
  ped->code = strdup(code);
  ped->this_obj = this_obj;
  v7_own(v7, &ped->this_obj);
  pe.type = V7_EXEC_EVENT;
  pe.data = ped;
  osi_MsgQWrite(&s_prompt_input_q, &pe, OSI_WAIT_FOREVER);
}
