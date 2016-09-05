/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_USER_ESP_GDB_H_
#define CS_FW_PLATFORMS_ESP8266_USER_ESP_GDB_H_

struct regfile;

void gdb_server(struct regfile *regs);

#endif /* CS_FW_PLATFORMS_ESP8266_USER_ESP_GDB_H_ */
