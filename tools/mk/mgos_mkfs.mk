MKSPIFFS ?= /usr/local/bin/mkspiffs
MGOS_ROOT_FS_OPTS_SPIFFS ?=
MGOS_ROOT_FS_OPTS_LFS ?=

FS_FILES ?=
ifneq "$(APP_FS_FILES)" ""
  FS_FILES += $(foreach m,$(APP_FS_FILES),$(wildcard $(m)))
endif

ifeq "$(MGOS_ROOT_FS_TYPE)" "SPIFFS"
  MKFS ?= /usr/local/bin/mkspiffs8
  MGOS_ROOT_FS_OPTS ?= $(MGOS_ROOT_FS_OPTS_SPIFFS)
else
ifeq "$(MGOS_ROOT_FS_TYPE)" "LFS"
  MKFS ?= /usr/local/bin/mklfs
  MGOS_ROOT_FS_OPTS ?= $(MGOS_ROOT_FS_OPTS_LFS)
endif
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

# Args: fs_size, json_opts
define mkfs
	$(Q) rm -rf $(FS_STAGING_DIR) && mkdir -p $(FS_STAGING_DIR) $(dir $@)
	$(Q) $(foreach f,$(FS_FILES), \
	  echo "  CP    $(f) -> $(FS_STAGING_DIR)"; \
	  cp $(f) $(FS_STAGING_DIR);)
	$(vecho) "MKFS  $(MKFS) $1 $2 $(FS_STAGING_DIR) -> $@"
	$(Q) $(MKFS) -s $1 -o '$2' -f $@ $(FS_STAGING_DIR)
endef
