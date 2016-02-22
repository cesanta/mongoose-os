BUILD_DIR ?= .
REPO_PATH ?= ../..
SYS_CONF_DEFAULTS ?= $(REPO_PATH)/smartjs/src/fs/conf_sys_defaults.json
APP_CONF_DEFAULTS ?= $(REPO_PATH)/smartjs/src/fs/conf_app_defaults.json

$(BUILD_DIR)/sys_config.c $(BUILD_DIR)/sys_config.h: $(SYS_CONF_DEFAULTS) $(APP_CONF_DEFAULTS)
	$(vecho) GEN $@
	$(Q) mkdir -p $(BUILD_DIR)
	$(Q) python $(REPO_PATH)/tools/json_to_c_config.py --c_name=sys_config --dest_dir=$(BUILD_DIR) $^

APP_OBJS += $(BUILD_DIR)/sys_config.o $(BUILD_DIR)/device_config.o $(BUILD_DIR)/sj_config.o
