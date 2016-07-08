/*
* Copyright (c) 2016 Cesanta Software Limited
* All rights reserved
*/

#ifndef CS_FW_SRC_SJ_UPDATER_CLUBBY_H_
#define CS_FW_SRC_SJ_UPDATER_CLUBBY_H_

struct v7;
void init_updater_clubby(struct v7 *v7);
void clubby_updater_finish(int error_code);

#endif /* CS_FW_SRC_SJ_UPDATER_CLUBBY_H_ */
