MIOT_PATH ?= ../..
FS_PATH ?= $(MIOT_PATH)/fw/skeleton/fs
SYS_CONF_SCHEMA ?=
APP_CONF_SCHEMA ?=
SYS_RO_VARS_SCHEMA ?= $(MIOT_PATH)/fw/src/sys_ro_vars_schema.yaml
GSC_TOOL ?= $(MIOT_PATH)/fw/tools/gen_sys_config.py
PYTHON ?= python

SYS_CONF_SCHEMA += $(MIOT_PATH)/fw/src/miot_sys_config.yaml

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

FS_FILES += $(SYS_CONFIG_SCHEMA_JSON) $(SYS_RO_VARS_SCHEMA_JSON)

# Put defaults on the filesystem under the old name, for compatibility.
FS_FILES += $(GEN_DIR)/conf_defaults.json

$(GEN_DIR)/conf_defaults.json: $(SYS_CONFIG_C)
	$(vecho) "CP    $(SYS_CONFIG_DEFAULTS_JSON) $@"
	$(Q) mkdir -p $(dir $@)
	$(Q) cp $(SYS_CONFIG_DEFAULTS_JSON) $@
