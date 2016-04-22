/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_INCLUDE_SYS_MMAN_H_
#define CS_FW_PLATFORMS_ESP8266_INCLUDE_SYS_MMAN_H_

#define MAP_PRIVATE 1
#define PROT_READ 1
#define MAP_FAILED ((void *) (-1))

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);

#endif /* CS_FW_PLATFORMS_ESP8266_INCLUDE_SYS_MMAN_H_ */
