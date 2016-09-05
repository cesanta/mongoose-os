/*
* Copyright (c) 2016 Cesanta Software Limited
* All rights reserved
*/

#ifndef CS_FW_SRC_SJ_UPDATER_CLUBBY_H_
#define CS_FW_SRC_SJ_UPDATER_CLUBBY_H_

#ifdef SJ_ENABLE_CLUBBY
void sj_updater_clubby_init(void);
void clubby_updater_finish(int error_code);
#endif

#endif /* CS_FW_SRC_SJ_UPDATER_CLUBBY_H_ */
