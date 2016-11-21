ifeq "$(MIOT_ENABLE_JS)" "1"
  JS_EXTRA ?=
  JSBIN_SRCS := $(MIOT_JS_PATH)/sys_init.js \
                $(MIOT_JS_PATH)/demo.js \
                $(MIOT_JS_PATH)/I2C.js \
                $(JS_EXTRA)
ifneq "$(APP_FS_PATH)" ""
  JSBIN_SRCS += $(wildcard $(APP_FS_PATH)/*.js)
endif
else
  JSBIN_SRCS :=
endif

FS_FILES ?=
ifneq "$(COMMON_FS_PATH)" ""
  FS_FILES += $(filter-out $(JSBIN_SRCS),$(wildcard $(COMMON_FS_PATH)/*))
endif
ifneq "$(APP_FS_PATH)" ""
  FS_FILES += $(filter-out $(JSBIN_SRCS),$(wildcard $(APP_FS_PATH)/*))
endif

# In order to keep compatibility
# with shared JS-files, which can call "File.eval(....)" using JS as extension
JSBIN_EXT := js

# Args: fs_size.
define mkspiffs
	$(Q) rm -rf $(FS_STAGING_DIR) && mkdir -p $(FS_STAGING_DIR)
	$(Q) $(foreach f,$(FS_FILES) $(SYS_CONFIG_SCHEMA_JSON) $(SYS_RO_VARS_SCHEMA_JSON), \
	  echo "  CP    $(f) -> $(FS_STAGING_DIR)"; \
	  cp $(f) $(FS_STAGING_DIR);)
	$(Q) $(foreach jsbin,$(JSBIN_SRCS), \
	  echo "  V7C   $(jsbin) -> $(FS_STAGING_DIR)/$(basename $(notdir $(jsbin))).$(JSBIN_EXT)" && \
	  $(BUILD_DIR)/v7 -c $(jsbin) > $(FS_STAGING_DIR)/$(basename $(notdir $(jsbin))).$(JSBIN_EXT) && ) true
	$(Q) cp $(SYS_CONFIG_DEFAULTS_JSON) $(FS_STAGING_DIR)/conf_defaults.json
	$(vecho) "MKFS  $(FS_STAGING_DIR) ($(FS_SIZE))-> $@"
	$(Q) /usr/local/bin/mkspiffs $1 $(FS_STAGING_DIR) > $@
endef
