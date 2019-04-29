#
# Component makefile.
#

override MGOS_PATH = $(_MGOS_PATH)
include $(MGOS_PATH)/platforms/esp32/src/esp32_src.mk

COMPONENT_OBJS = $(APP_OBJS)
