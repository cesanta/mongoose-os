PROG = unit_test
REPO_ROOT ?= ../..
PYTHON ?= python3
BUILD_DIR = ./build
SYS_CONF_C = $(BUILD_DIR)/mgos_config.c
GEN_CONFIG_TOOL = $(REPO_ROOT)/tools/mgos_gen_config.py
MONGOOSE_PATH ?=

ifeq "$(MONGOOSE_PATH)" ""
$(error "provide MONGOOSE_PATH")
endif

SOURCES = unit_test.c \
          $(SYS_CONF_C) \
          $(REPO_ROOT)/src/frozen/frozen.c \
          $(REPO_ROOT)/src/mgos_config_util.c \
          $(REPO_ROOT)/src/mgos_event.c \
          $(REPO_ROOT)/src/common/json_utils.c \
          $(REPO_ROOT)/src/common/cs_file.c \
          $(REPO_ROOT)/src/common/cs_hex.c \
          $(MONGOOSE_PATH)/mongoose.c \
          test_main.c \
          test_util.c

INCS = -I$(REPO_ROOT)/src \
       -I$(REPO_ROOT)/include \
       -I$(REPO_ROOT)/src/frozen \
       -I$(REPO_ROOT) \
       -I$(MONGOOSE_PATH) \
       -I. \
       $(CFLAGS_EXTRA)

CFLAGS = -W -Wall -Wextra -Werror -g -O0 -Wno-multichar -I$(BUILD_DIR) $(INCS)

all: $(BUILD_DIR) $(PROG)
	./$(PROG)
	$(foreach f,mgos_config.c mgos_config.h mgos_config_schema.json, \
	  diff -uBb data/golden/$f $(BUILD_DIR)/$f && ) echo Ok

$(BUILD_DIR):
	mkdir $@

$(PROG): $(SOURCES)
	clang -fsanitize=address -o $(PROG) $(SOURCES) $(CFLAGS)

#include $(REPO_ROOT)/common/scripts/test.mk
$(SYS_CONF_C): data/sys_conf_wifi.yaml data/sys_conf_http.yaml data/sys_conf_debug.yaml data/sys_conf_overrides.yaml $(GEN_CONFIG_TOOL)
	$(REPO_ROOT)/tools/mgos_gen_config.py \
	  --c_name=mgos_config \
	  --c_global_name=mgos_sys_config \
	  --dest_dir=$(BUILD_DIR) \
	  $(filter-out $(GEN_CONFIG_TOOL),$^)

clean:
	rm -rf $(PROG) $(BUILD_DIR)
