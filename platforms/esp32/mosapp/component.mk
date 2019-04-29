#
# Component makefile.
#

override MGOS_PATH = $(_MGOS_PATH)
include $(MGOS_PATH)/platforms/esp32/src/esp32_src.mk

COMPONENT_OBJS = $(MGOS_OBJS) $(APP_OBJS) $(FFI_EXPORTS_O) $(BUILD_INFO_OBJS)

COMPONENT_ADD_LDFLAGS := $(APP_BIN_LIB_FILES) -Wl,--whole-archive -lmosapp -Wl,--no-whole-archive
