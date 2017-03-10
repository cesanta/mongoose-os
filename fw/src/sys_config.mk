MGOS_PATH ?= ../..
FS_PATH ?= $(MGOS_PATH)/fw/skeleton/fs
SYS_CONF_SCHEMA ?=
APP_CONF_SCHEMA ?=
SYS_RO_VARS_SCHEMA ?= $(MGOS_PATH)/fw/src/sys_ro_vars_schema.yaml
GSC_TOOL ?= $(MGOS_PATH)/fw/tools/gen_sys_config.py
PYTHON ?= python

SYS_CONF_SCHEMA += $(MGOS_PATH)/fw/src/mgos_sys_config.yaml

SYS_CONFIG_SCHEMA_JSON ?=
SYS_RO_VARS_SCHEMA_JSON ?=

$(SYS_CONFIG_C) $(SYS_CONFIG_SCHEMA_JSON): $(SYS_CONF_SCHEMA) $(APP_CONF_SCHEMA) $(GSC_TOOL)
	$(vecho) "GEN   $@"
	$(Q) $(PYTHON) $(GSC_TOOL) \
	  --c_name=sys_config \
	  --dest_dir=$(dir $@) $(SYS_CONF_SCHEMA) $(APP_CONF_SCHEMA)

$(SYS_RO_VARS_C) $(SYS_RO_VARS_SCHEMA_JSON): $(SYS_RO_VARS_SCHEMA) $(GSC_TOOL)
	$(vecho) "GEN   $@"
	$(Q) $(PYTHON) $(GSC_TOOL) \
	  --c_name=sys_ro_vars --c_const_char=true \
	  --dest_dir=$(dir $@) $(SYS_RO_VARS_SCHEMA)

$(SYS_CONFIG_DEFAULTS_JSON): $(SYS_CONFIG_C)
	$(Q) cp $(dir $(SYS_CONFIG_C))/sys_config_defaults.json $@

FS_FILES += $(SYS_CONFIG_DEFAULTS_JSON) $(SYS_CONFIG_SCHEMA_JSON) $(SYS_RO_VARS_SCHEMA_JSON)
