/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#if CS_PLATFORM == CS_P_MSP432

int gettimeofday(struct timeval *tp, void *tzp) {
  /* FIXME */
  tp->tv_sec = 42;
  tp->tv_usec = 123;
  return 0;
}

long int random(void) {
  return 42; /* FIXME */
}

#endif /* CS_PLATFORM == CS_P_MSP432 */
