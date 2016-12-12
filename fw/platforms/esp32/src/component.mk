#
# Component makefile.
#

COMPONENT_EXTRA_INCLUDES = $(MIOT_PATH) $(MIOT_ESP_PATH)/include $(SPIFFS_PATH)

COMPONENT_OBJS = esp32_fs.o esp32_main.o
