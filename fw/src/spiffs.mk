MKSPIFFS ?= /usr/local/bin/mkspiffs

FS_FILES ?=
ifneq "$(APP_FS_FILES)" ""
  FS_FILES += $(foreach m,$(APP_FS_FILES),$(wildcard $(m)))
endif

# Args: fs_size, block_size, page_size, erase_size.
define mkspiffs
	$(Q) rm -rf $(FS_STAGING_DIR) && mkdir -p $(FS_STAGING_DIR) $(dir $@)
	$(Q) $(foreach f,$(FS_FILES), \
	  echo "  CP    $(f) -> $(FS_STAGING_DIR)"; \
	  cp $(f) $(FS_STAGING_DIR);)
	$(vecho) "MKFS  $(MKSPIFFS) $1 $2 $3 $4 $(FS_STAGING_DIR) -> $@"
	$(Q) $(MKSPIFFS) -s $1 -b $2 -p $3 -e $4 -f $@ $(FS_STAGING_DIR)
endef
