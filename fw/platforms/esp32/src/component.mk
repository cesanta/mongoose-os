#
# Component makefile.
#

MIOT_ENABLE_ATCA ?= 0
MIOT_ENABLE_ATCA_SERVICE ?= 0
MIOT_ENABLE_CONFIG_SERVICE ?= 0
MIOT_ENABLE_CONSOLE ?= 0
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
MIOT_ENABLE_WIFI ?= 1

MIOT_SRC_PATH = $(MIOT_PATH)/fw/src

SYS_CONFIG_C = $(GEN_DIR)/sys_config.c
SYS_CONFIG_DEFAULTS_JSON = $(GEN_DIR)/sys_config_defaults.json
SYS_CONFIG_SCHEMA_JSON = $(GEN_DIR)/sys_config_schema.json
SYS_RO_VARS_C = $(GEN_DIR)/sys_ro_vars.c
SYS_RO_VARS_SCHEMA_JSON = $(GEN_DIR)/sys_ro_vars_schema.json
SYS_CONF_SCHEMA =

COMPONENT_EXTRA_INCLUDES = $(MIOT_PATH) $(MIOT_ESP_PATH)/include $(SPIFFS_PATH) $(GEN_DIR)

MIOT_SRCS = miot_config.c miot_init.c miot_mongoose.c \
            miot_sys_config.c $(notdir $(SYS_CONFIG_C)) $(notdir $(SYS_RO_VARS_C)) \
            esp32_fs.c esp32_hal.c esp32_main.c

include $(MIOT_PATH)/fw/common.mk
include $(MIOT_PATH)/fw/src/features.mk

ifeq "$(MIOT_ENABLE_WIFI)" "1"
  MIOT_SRCS += esp32_wifi.c
  SYS_CONF_SCHEMA += $(MIOT_ESP_PATH)/src/esp32_wifi_config.yaml
endif

include $(MIOT_PATH)/fw/src/sys_config.mk

VPATH += $(MIOT_PATH)/common
MIOT_SRCS += cs_dbg.c cs_file.c

VPATH += $(MIOT_PATH)/fw/src

VPATH += $(MIOT_PATH)/frozen
MIOT_SRCS += frozen.c

VPATH += $(MIOT_PATH)/mongoose
MIOT_SRCS += mongoose.c

VPATH += $(GEN_DIR)

COMPONENT_OBJS = $(addsuffix .o,$(basename $(MIOT_SRCS)))
CFLAGS += $(MIOT_FEATURES)

libsrc.a: $(GEN_DIR)/conf_defaults.json

%.o: %.c $(SYS_CONFIG_C) $(SYS_RO_VARS_C)
	$(summary) CC $@
	$(CC) $(CFLAGS) $(CPPFLAGS) \
	  $(addprefix -I ,$(COMPONENT_INCLUDES)) \
	  $(addprefix -I ,$(COMPONENT_EXTRA_INCLUDES)) \
	  -c $< -o $@
