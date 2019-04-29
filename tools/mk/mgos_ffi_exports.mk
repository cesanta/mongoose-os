PYTHON ?= python3
FW_META_CMD ?= $(MGOS_PATH)/tools/mgos_fw_meta.py

define gen_ffi_exports
	$(vecho) "GEN   $1"
	$(Q) $(PYTHON) $(FW_META_CMD) gen_ffi_exports \
	  --c_output='$1' \
	  --patterns='$2' \
	  $3
endef

