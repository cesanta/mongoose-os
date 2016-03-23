REPO_PATH ?= ../..
SYS_CONF_DEFAULTS ?= $(REPO_PATH)/smartjs/src/fs/conf_sys_defaults.json
APP_CONF_DEFAULTS ?= $(REPO_PATH)/smartjs/src/fs/conf_app_defaults.json

$(SYS_CONFIG_C): $(SYS_CONF_DEFAULTS) $(APP_CONF_DEFAULTS)
	$(vecho) "GEN   $@"
	$(Q) python $(REPO_PATH)/tools/json_to_c_config.py \
	  --c_name=sys_config \
	  --dest_dir=$(dir $@) $^
