MKSPIFFS ?= /usr/local/bin/mkspiffs

FS_FILES ?=
ifneq "$(COMMON_FS_PATH)" ""
  FS_FILES += $(wildcard $(COMMON_FS_PATH)/*)
endif
ifneq "$(APP_FS_PATH)" ""
  FS_FILES += $(wildcard $(APP_FS_PATH)/*)
endif

# Args: fs_size.
define mkspiffs
	$(Q) rm -rf $(FS_STAGING_DIR) && mkdir -p $(FS_STAGING_DIR) $(dir $@)
	$(Q) $(foreach f,$(FS_FILES), \
	  echo "  CP    $(f) -> $(FS_STAGING_DIR)"; \
	  cp $(f) $(FS_STAGING_DIR);)
	$(vecho) "MKFS  $(FS_STAGING_DIR) ($(FS_SIZE)) -> $@"
	$(Q) $(MKSPIFFS) $1 $(FS_STAGING_DIR) > $@
endef
