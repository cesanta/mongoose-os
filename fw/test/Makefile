PROG = unit_test
REPO_ROOT ?= ../..
PYTHON ?= python

SOURCES = unit_test.c \
               sys_conf.c \
               $(REPO_ROOT)/frozen/frozen.c \
               $(REPO_ROOT)/fw/src/mg_config.c \
               $(REPO_ROOT)/mongoose/mongoose.c \
               $(REPO_ROOT)/common/json_utils.c \
               $(REPO_ROOT)/common/cs_file.c \
               $(REPO_ROOT)/common/test_util.c

INCS = -I$(REPO_ROOT)/fw/src \
       -I$(REPO_ROOT) \
       -I$(REPO_ROOT)/common \
       -I. \
       $(CFLAGS_EXTRA)

CFLAGS = -W -Wall -Werror -g -O0 -Wno-multichar $(INCS)

all: $(PROG) sys_conf.c
	./$(PROG)

$(PROG): $(SOURCES)
	$(CC) -o $(PROG) $(SOURCES) $(CFLAGS)

#include $(REPO_ROOT)/common/scripts/test.mk
sys_conf.c: data/defaults.json
	$(PYTHON) $(REPO_ROOT)/tools/json_to_c_config.py --c_name=sys_conf $<

clean:
	rm -rf $(PROG) sys_conf.*
