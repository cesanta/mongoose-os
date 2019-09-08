MGOS_PATH ?= ../..
MGOS_CONF_SCHEMA ?=
APP_CONF_SCHEMA ?=
MGOS_RO_VARS_SCHEMA ?= $(MGOS_PATH)/src/mgos_ro_vars_schema.yaml
GSC_TOOL ?= $(MGOS_PATH)/tools/mgos_gen_config.py
PYTHON3 ?= python3

MGOS_CONFIG_DEFAULTS_JSON ?= $(GEN_DIR)/conf0.json

$(MGOS_CONFIG_C): $(MGOS_CONF_SCHEMA) $(APP_CONF_SCHEMA) $(GSC_TOOL)
	$(vecho) "GEN   $@"
	$(Q) $(PYTHON3) $(GSC_TOOL) \
	  --c_name=mgos_config --c_global_name=mgos_sys_config \
	  --dest_dir=$(dir $@) $(MGOS_CONF_SCHEMA) $(APP_CONF_SCHEMA)

$(MGOS_RO_VARS_C): $(MGOS_RO_VARS_SCHEMA) $(GSC_TOOL)
	$(vecho) "GEN   $@"
	$(Q) $(PYTHON3) $(GSC_TOOL) \
	  --c_name=mgos_ro_vars --c_global_name=mgos_sys_ro_vars \
	  --dest_dir=$(dir $@) $(MGOS_RO_VARS_SCHEMA)

# Deprecated since 2019/05/14.
# This can be deleted eventually. For now we keep the file so during OTA previous one gets overwritten
# and some space is freed.
$(MGOS_CONFIG_DEFAULTS_JSON): $(MGOS_CONFIG_C)
	$(Q) echo '{}' > $@

FS_FILES += $(MGOS_CONFIG_DEFAULTS_JSON)
