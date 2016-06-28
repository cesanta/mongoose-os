PYTHON ?= python
BUILD_DIR ?= ./build
BUILD_INFO_C ?= $(BUILD_DIR)/build_info.c
BUILD_INFO_JSON ?= $(BUILD_DIR)/build_info.json
FW_META_CMD ?= $(REPO_PATH)/common/tools/fw_meta.py
GEN_BUILD_INFO_EXTRA ?=

$(BUILD_INFO_C) $(BUILD_INFO_JSON):
	$(vecho) "GEN   $@"
	$(Q) $(PYTHON) $(FW_META_CMD) gen_build_info \
	  --tag_as_version=true \
	  --c_output=$(BUILD_INFO_C) \
	  --json_output=$(BUILD_INFO_JSON) \
	  $(GEN_BUILD_INFO_EXTRA)
