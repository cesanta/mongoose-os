MKSPIFFS ?= /usr/local/bin/mkspiffs

FS_FILES ?=
ifneq "$(APP_FS_FILES)" ""
  FS_FILES += $(foreach m,$(APP_FS_FILES),$(wildcard $(m)))
endif

# Args: fs_size.
define mkspiffs
	$(Q) rm -rf $(FS_STAGING_DIR) && mkdir -p $(FS_STAGING_DIR) $(dir $@)
	$(Q) $(foreach f,$(FS_FILES), \
	  echo "  CP    $(f) -> $(FS_STAGING_DIR)"; \
	  cp $(f) $(FS_STAGING_DIR);)
	$(vecho) "MKFS  $(MKSPIFFS) $1 $(FS_STAGING_DIR) -> $@"
	$(Q) $(MKSPIFFS) $1 $(FS_STAGING_DIR) $@
endef
