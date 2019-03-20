MGOS_PATH ?= ../..
MGOS_CONF_SCHEMA ?=
APP_CONF_SCHEMA ?=
MGOS_RO_VARS_SCHEMA ?= $(MGOS_PATH)/fw/src/mgos_ro_vars_schema.yaml
GSC_TOOL ?= $(MGOS_PATH)/fw/tools/gen_sys_config.py
PYTHON ?= python3

MGOS_CONFIG_SCHEMA_JSON ?=
MGOS_RO_VARS_SCHEMA_JSON ?=

$(MGOS_CONFIG_C) $(MGOS_CONFIG_SCHEMA_JSON): $(MGOS_CONF_SCHEMA) $(APP_CONF_SCHEMA) $(GSC_TOOL)
	$(vecho) "GEN   $@"
	$(Q) $(PYTHON) $(GSC_TOOL) \
	  --c_name=mgos_config --c_global_name=mgos_sys_config \
	  --dest_dir=$(dir $@) $(MGOS_CONF_SCHEMA) $(APP_CONF_SCHEMA)

$(MGOS_RO_VARS_C) $(MGOS_RO_VARS_SCHEMA_JSON): $(MGOS_RO_VARS_SCHEMA) $(GSC_TOOL)
	$(vecho) "GEN   $@"
	$(Q) $(PYTHON) $(GSC_TOOL) \
	  --c_name=mgos_ro_vars --c_global_name=mgos_sys_ro_vars \
	  --dest_dir=$(dir $@) $(MGOS_RO_VARS_SCHEMA)

$(MGOS_CONFIG_DEFAULTS_JSON): $(MGOS_CONFIG_C)
	$(Q) cp $(dir $(MGOS_CONFIG_C))/mgos_config_defaults.json $@

FS_FILES += $(MGOS_CONFIG_DEFAULTS_JSON)
