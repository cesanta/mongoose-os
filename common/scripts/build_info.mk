PYTHON ?= python
BUILD_DIR ?= ./.build
BUILD_INFO_C ?= $(BUILD_DIR)/build_info.c
BUILD_INFO_JSON ?= $(BUILD_DIR)/build_info.json
MG_BUILD_INFO_C ?= $(BUILD_DIR)/mg_build_info.c
FW_META_CMD ?= $(MIOT_PATH)/common/tools/fw_meta.py
GEN_BUILD_INFO_EXTRA ?=
APP_VERSION ?=
APP_BUILD_ID ?=

define gen_build_info
	$(vecho) "GEN   $1"
	$(Q) $(PYTHON) $(FW_META_CMD) gen_build_info \
	  --repo_path=$2 \
	  --id=$3 \
	  --version=$4 \
	  --tag_as_version=true \
	  --var_prefix=$5 \
	  --c_output=$6 \
	  --json_output=$7 \
	  $(GEN_BUILD_INFO_EXTRA)
endef
