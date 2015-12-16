#ifndef _WIN32
#include <stddef.h>
#ifndef MG_CC3200
#include <sys/time.h>
#endif
#else
#include <windows.h>
#endif

double cs_time() {
  double now;
#ifndef _WIN32
  struct timeval tv;
  if (gettimeofday(&tv, NULL /* tz */) != 0) return 0;
  now = (double) tv.tv_sec + (((double) tv.tv_usec) / 1000000.0);
#else
  now = GetTickCount() / 1000.0;
#endif
  return now;
}
