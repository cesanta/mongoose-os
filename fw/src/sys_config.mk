REPO_PATH ?= ../..
FS_PATH ?= $(REPO_PATH)/fw/skeleton/filesystem
SYS_CONF_SCHEMA ?= $(REPO_PATH)/fw/src/sys_config_schema.yaml
APP_CONF_SCHEMA ?=
SYS_RO_VARS_SCHEMA ?= $(REPO_PATH)/fw/src/sys_ro_vars_schema.yaml
GSC_TOOL ?= $(REPO_PATH)/fw/tools/gen_sys_config.py
PYTHON ?= python

$(SYS_CONFIG_C): $(SYS_CONF_SCHEMA) $(APP_CONF_SCHEMA) $(GSC_TOOL)
	$(vecho) "GEN   $@"
	$(Q) $(PYTHON) $(GSC_TOOL) \
	  --c_name=sys_config \
	  --dest_dir=$(dir $@) $(SYS_CONF_SCHEMA) $(APP_CONF_SCHEMA)

$(SYS_RO_VARS_C): $(SYS_RO_VARS_SCHEMA) $(GSC_TOOL)
	$(vecho) "GEN   $@"
	$(Q) $(PYTHON) $(GSC_TOOL) \
	  --c_name=sys_ro_vars --c_const_char=true \
	  --dest_dir=$(dir $@) $(SYS_RO_VARS_SCHEMA)
