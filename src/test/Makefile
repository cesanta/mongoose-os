PROG = unit_test
REPO_ROOT ?= ../..
PYTHON ?= python3
BUILD_DIR = .build
SYS_CONF_C = $(BUILD_DIR)/sys_conf.c

SOURCES = unit_test.c \
          $(SYS_CONF_C) \
          $(REPO_ROOT)/src/frozen/frozen.c \
          $(REPO_ROOT)/src/mgos_config_util.c \
          $(REPO_ROOT)/src/mgos_event.c \
          $(REPO_ROOT)/common/json_utils.c \
          $(REPO_ROOT)/common/cs_file.c \
          test_main.c \
          test_util.c

INCS = -I$(REPO_ROOT)/src \
       -I$(REPO_ROOT)/include \
       -I$(REPO_ROOT)/src/frozen \
       -I$(REPO_ROOT) \
       -I. \
       $(CFLAGS_EXTRA)

CFLAGS = -W -Wall -Werror -g -O0 -Wno-multichar -I$(BUILD_DIR) $(INCS)

$(BUILD_DIR):
	mkdir $@

all: $(BUILD_DIR) $(PROG)
	./$(PROG)

$(PROG): $(SOURCES)
	$(CC) -o $(PROG) $(SOURCES) $(CFLAGS)

#include $(REPO_ROOT)/common/scripts/test.mk
$(SYS_CONF_C): data/sys_conf_wifi.yaml data/sys_conf_http.yaml data/sys_conf_debug.yaml data/sys_conf_overrides.yaml
	$(REPO_ROOT)/tools/mgos_gen_config.py \
	  --c_name=sys_conf \
	  --c_global_name=sys_conf_global \
	  --dest_dir=$(BUILD_DIR) \
	  $^
	$(foreach f,sys_conf.c sys_conf.h sys_conf_defaults.json sys_conf_schema.json, \
	  diff -uBb data/golden/$f .build/$f && ) true

clean:
	rm -rf $(PROG) $(BUILD_DIR)
