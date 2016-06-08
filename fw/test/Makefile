PROG = unit_test
EXTRA_CLEAN_TARGETS = sys_conf.*
TEST_SOURCES = unit_test.c \
               sys_conf.c \
               ../src/sj_config.c \
               ../src/mongoose.c \
               ../../common/cs_file.c \
               ../../common/test_util.c
INCS = -I../src -I../../common $(CFLAGS_EXTRA)
CFLAGS = -W -Wall -Werror -g -O0 -Wno-multichar $(INCS)
PYTHON ?= PYTHON

include ../../mongoose/test/test.mk

sys_conf.c: data/defaults.json
	$(PYTHON) ../../tools/json_to_c_config.py $< sys_conf
