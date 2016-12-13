#
# Component makefile.
#

MIOT_ENABLE_ATCA ?= 0
MIOT_ENABLE_ATCA_SERVICE ?= 0
MIOT_ENABLE_CONFIG_SERVICE ?= 0
MIOT_ENABLE_DNS_SD ?= 0
MIOT_ENABLE_FILESYSTEM_SERVICE ?= 0
MIOT_ENABLE_I2C ?= 0
MIOT_ENABLE_JS ?= 0
MIOT_ENABLE_MQTT ?= 0
MIOT_ENABLE_RPC ?= 0
MIOT_ENABLE_RPC_CHANNEL_HTTP ?= 0
MIOT_ENABLE_RPC_CHANNEL_UART ?= 0
MIOT_ENABLE_UPDATER ?= 0
MIOT_ENABLE_UPDATER_POST ?= 0
MIOT_ENABLE_UPDATER_RPC ?= 0

MIOT_SRC_PATH = $(MIOT_PATH)/fw/src

SYS_CONFIG_C = $(GEN_DIR)/sys_config.c
SYS_CONFIG_DEFAULTS_JSON = $(GEN_DIR)/sys_config_defaults.json
SYS_CONFIG_SCHEMA_JSON = $(GEN_DIR)/sys_config_schema.json
SYS_RO_VARS_C = $(GEN_DIR)/sys_ro_vars.c
SYS_RO_VARS_SCHEMA_JSON = $(GEN_DIR)/sys_ro_vars_schema.json
SYS_CONF_SCHEMA =

include $(MIOT_PATH)/fw/common.mk
include $(MIOT_PATH)/fw/src/sys_config.mk
include $(MIOT_PATH)/fw/src/features.mk

COMPONENT_EXTRA_INCLUDES = $(MIOT_PATH) $(MIOT_ESP_PATH)/include $(SPIFFS_PATH) $(GEN_DIR)

COMPONENT_OBJS = esp32_fs.o esp32_main.o

VPATH += $(MIOT_PATH)/common $(MIOT_PATH)/fw/src $(GEN_DIR)

COMPONENT_OBJS += cs_dbg.o miot_init.o sys_config.o

%.o: %.c $(SYS_CONFIG_C) $(SYS_RO_VARS_C)
	$(summary) CC $@
	$(CC) $(CFLAGS) $(CPPFLAGS) \
	  $(addprefix -I ,$(COMPONENT_INCLUDES)) \
	  $(addprefix -I ,$(COMPONENT_EXTRA_INCLUDES)) \
	  -c $< -o $@
