#ifndef _CC3200_FS_H_
#define _CC3200_FS_H_

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include "spiffs.h"

struct v7;

int init_fs(struct v7 *v7);
int set_errno(int e);

#ifdef CC3200_FS_DEBUG
#define dprintf(x) printf x
#else
#define dprintf(x)
#endif

#endif /* _CC3200_FS_H_ */
