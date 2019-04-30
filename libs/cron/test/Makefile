all: test test_ndebug

INCDIRS = -I../src -I../include -I../../.. -I../../../frozen -I../../../mongoose -I.
SOURCES = ../src/mgos_cron.c ../src/ccronexpr.c ../src/sunriset.c \
          ../../../common/cs_time.c \
          ../../../common/str_util.c ../../../common/mg_str.c \
          cs_dbg.c mgos_hal.c mgos_location.c mgos_timers.c \
          mgos_sys_config.c mgos_utils.c ../../../frozen/frozen.c cron_test.c
LIBS = -lm

test:
	gcc --std=c99 -D_POSIX_SOURCE $(CFLAGS) $(INCDIRS) -g3 -m32 $(SOURCES) $(LIBS) -o cron_test
	./cron_test

test_ndebug:
	gcc --std=c99 -D_POSIX_SOURCE $(CFLAGS) $(INCDIRS) -DNDEBUG -g3 -m32 $(SOURCES) $(LIBS) -o cron_test
	./cron_test
