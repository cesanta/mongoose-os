PYTHON ?= python
FW_META_CMD ?= $(MGOS_PATH)/common/tools/fw_meta.py

define gen_ffi_exports
	$(vecho) "GEN   $1 > $2"
	$(Q) $(PYTHON) $(FW_META_CMD) gen_ffi_exports \
	  --input='$1' \
	  --c_output='$2' \
	  --patterns='$3'
endef

