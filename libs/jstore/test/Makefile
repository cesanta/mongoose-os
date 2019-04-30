all: test

JSTORE_INCDIRS = -I. -I../src -I../include -I../../.. -I../../../fw/src -I../../../fw/include -I../../../frozen
JSTORE_SRC = jstore_test.c ../src/mgos_jstore.c \
             ../../../common/cs_file.c \
             ../../../common/cs_dbg.c \
             ../../../common/str_util.c \
             ../../../frozen/frozen.c \
             ../../../common/mg_str.c \

test:
	gcc --std=c99 -g3 -Wall -Werror -Wno-missing-braces $(CFLAGS) $(JSTORE_INCDIRS) -g3 -m32 $(JSTORE_SRC) -o jstore_test
	./jstore_test
