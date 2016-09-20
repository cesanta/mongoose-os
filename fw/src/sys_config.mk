REPO_PATH ?= ../..
FS_PATH ?= $(REPO_PATH)/fw/skeleton/filesystem
SYS_CONF_DEFAULTS ?= $(FS_PATH)/conf_sys_defaults.json
APP_CONF_DEFAULTS ?= $(FS_PATH)/conf_app_defaults.json
SYS_RO_VARS_JSON ?= $(REPO_PATH)/fw/src/sys_ro_vars.json
J2CC_TOOL ?= $(REPO_PATH)/tools/json_to_c_config.py
PYTHON ?= python

$(SYS_CONFIG_C): $(SYS_CONF_DEFAULTS) $(APP_CONF_DEFAULTS) $(J2CC_TOOL)
	$(vecho) "GEN   $@"
	$(Q) $(PYTHON) $(J2CC_TOOL) \
	  --c_name=sys_config \
	  --dest_dir=$(dir $@) $(SYS_CONF_DEFAULTS) $(APP_CONF_DEFAULTS)

$(SYS_RO_VARS_C): $(SYS_RO_VARS_JSON) $(J2CC_TOOL)
	$(vecho) "GEN   $@"
	$(Q) $(PYTHON) $(J2CC_TOOL) \
	  --c_name=sys_ro_vars --c_const_char=true \
	  --dest_dir=$(dir $@) $(SYS_RO_VARS_JSON)
