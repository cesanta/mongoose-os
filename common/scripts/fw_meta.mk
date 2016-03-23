# Vars:
#   FW_DIR: firmware output directory
#   FW_PARTS: definition of firmware parts
#   APP: app name
#   APP_PLATFORM: app platform
#
# Add dependencies to $(FW_MANIFEST).

FW_MANIFEST ?= $(FW_DIR)/manifest.json
FW_ZIP ?= $(FW_DIR)/$(APP)-$(APP_PLATFORM)-last.zip
FW_META_CMD ?= $(REPO_PATH)/common/tools/fw_meta.py

$(FW_ZIP): $(FW_MANIFEST)
	$(vecho) ZIP $@
	$(Q) $(FW_META_CMD) create_fw \
	  --manifest=$(FW_MANIFEST) \
	  --src_dir=$(FW_DIR) \
	  --output=$@
	$(Q) cp $@ $(FW_DIR)/$(APP)-$(APP_PLATFORM)-$(shell $(FW_META_CMD) get $(FW_MANIFEST) version).zip

$(FW_MANIFEST):
	$(vecho) GEN $(FW_MANIFEST)
	$(Q) $(FW_META_CMD) create_manifest \
	  --name=$(APP) --platform=$(APP_PLATFORM) \
	  --build_info=$(BUILD_INFO_JSON) \
	  --output=$(FW_MANIFEST) \
	  --src_dir=$(FW_DIR) \
	  $(FW_PARTS)
