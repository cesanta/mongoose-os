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

CFLAGS = -W -Wall -Wextra -Werror -g -O0 -Wno-multichar -ffunction-sections -Wl,--gc-sections -I$(BUILD_DIR) $(INCS)

all: $(BUILD_DIR) test diff

test: $(PROG)
	./$(PROG)
	{ \
	  x=0; \
	  diff -u data/golden/mgos_config_pretty.json $(BUILD_DIR)/mgos_config_pretty.json || x=1; \
	  diff -u data/golden/mgos_config.json $(BUILD_DIR)/mgos_config.json || x=1; \
	  diff -u data/golden/mgos_config_boo_pretty.json $(BUILD_DIR)/mgos_config_boo_pretty.json || x=1; \
	  diff -u data/golden/mgos_config_test_bar1_pretty.json $(BUILD_DIR)/mgos_config_test_bar1_pretty.json || x=1; \
	  echo $$x; exit $$x; \
	}
	@echo DIFF PASS

diff: $(SYS_CONF_C)
	{ \
	  x=0; \
	  diff -u data/golden/mgos_config.h $(BUILD_DIR)/mgos_config.h || x=1; \
	  diff -u data/golden/mgos_config.c $(BUILD_DIR)/mgos_config.c || x=1; \
	  diff -u data/golden/mgos_config_schema.json $(BUILD_DIR)/mgos_config_schema.json || x=1; \
	  exit $$x; \
	}

update_golden: $(PROG)
	./$(PROG)
	cp ./build/mgos_config.c data/golden/mgos_config.c
	cp ./build/mgos_config.h data/golden/mgos_config.h
	cp ./build/mgos_config_schema.json data/golden/mgos_config_schema.json
	cp ./build/mgos_config.json data/golden/mgos_config.json
	cp ./build/mgos_config_boo_pretty.json data/golden/mgos_config_boo_pretty.json
	cp ./build/mgos_config_test_bar1_pretty.json data/golden/mgos_config_test_bar1_pretty.json
	cp ./build/mgos_config_pretty.json data/golden/mgos_config_pretty.json

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
