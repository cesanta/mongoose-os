#ifndef _DEBUG_H_INCLUDED_
#define _DEBUG_H_INCLUDED_

//#include "/home/alex/Projects/dev/v7/platforms/esp8266/arduino/debug.h"
void DumpFunc(const char* str);
void DumpLine(int line);
#define DUMPLINE() DumpLine(__LINE__)
#define DUMPFUNC() DumpFunc(__func__);

#endif
