MIOT_PATH ?= ../..
FS_PATH ?= $(MIOT_PATH)/fw/skeleton/fs
SYS_CONF_SCHEMA ?=
APP_CONF_SCHEMA ?=
SYS_RO_VARS_SCHEMA ?= $(MIOT_PATH)/fw/src/sys_ro_vars_schema.yaml
GSC_TOOL ?= $(MIOT_PATH)/fw/tools/gen_sys_config.py
PYTHON ?= python

SYS_CONF_SCHEMA += $(MIOT_PATH)/fw/src/miot_sys_config.yaml

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
