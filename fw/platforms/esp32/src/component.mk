#
# Component makefile.
#

override APP_MODULES = $(_APP_MODULES)
override APP_CONF_SCHEMA = $(_APP_CONF_SCHEMA)
override APP_EXTRA_SRCS = $(_APP_EXTRA_SRCS)
override APP_FS_PATH = $(_APP_FS_PATH)
override BUILD_DIR = $(_BUILD_DIR)
override FW_DIR := $(_FW_DIR)
override GEN_DIR := $(_GEN_DIR)
override MIOT_PATH = $(_MIOT_PATH)

MIOT_ENABLE_ATCA ?= 1
MIOT_ENABLE_ATCA_SERVICE ?= 1
MIOT_ENABLE_CONFIG_SERVICE ?= 1
MIOT_ENABLE_CONSOLE ?= 0
MIOT_ENABLE_DNS_SD ?= 0
MIOT_ENABLE_FILESYSTEM_SERVICE ?= 1
MIOT_ENABLE_I2C ?= 1
# Use bitbang I2C for now.
MIOT_ENABLE_I2C_GPIO ?= 1
MIOT_ENABLE_JS ?= 0
MIOT_ENABLE_MQTT ?= 1
MIOT_ENABLE_RPC ?= 1
MIOT_ENABLE_RPC_CHANNEL_HTTP ?= 1
MIOT_ENABLE_RPC_CHANNEL_UART ?= 1
MIOT_ENABLE_UPDATER ?= 0
MIOT_ENABLE_UPDATER_POST ?= 0
MIOT_ENABLE_UPDATER_RPC ?= 0
MIOT_ENABLE_WIFI ?= 1

MIOT_DEBUG_UART ?= 0

MIOT_SRC_PATH = $(MIOT_PATH)/fw/src

SYS_CONFIG_C = $(GEN_DIR)/sys_config.c
SYS_CONFIG_DEFAULTS_JSON = $(GEN_DIR)/sys_config_defaults.json
SYS_CONFIG_SCHEMA_JSON = $(GEN_DIR)/sys_config_schema.json
SYS_RO_VARS_C = $(GEN_DIR)/sys_ro_vars.c
SYS_RO_VARS_SCHEMA_JSON = $(GEN_DIR)/sys_ro_vars_schema.json
SYS_CONF_SCHEMA =

COMPONENT_EXTRA_INCLUDES = $(MIOT_PATH) $(MIOT_ESP_PATH)/include $(SPIFFS_PATH) $(GEN_DIR)

MIOT_SRCS = miot_config.c miot_gpio.c miot_init.c miot_mongoose.c \
            miot_sys_config.c $(notdir $(SYS_CONFIG_C)) $(notdir $(SYS_RO_VARS_C)) \
            miot_timers_mongoose.c miot_uart.c miot_utils.c \
            esp32_console.c esp32_crypto.c esp32_fs.c esp32_gpio.c esp32_hal.c \
            esp32_main.c esp32_uart.c

include $(MIOT_PATH)/fw/common.mk
include $(MIOT_PATH)/fw/src/features.mk

SYS_CONF_SCHEMA += $(MIOT_ESP_PATH)/src/esp32_config.yaml

ifeq "$(MIOT_ENABLE_I2C)" "1"
  SYS_CONF_SCHEMA += $(MIOT_ESP_PATH)/src/esp32_i2c_config.yaml
endif
ifeq "$(MIOT_ENABLE_WIFI)" "1"
  MIOT_SRCS += esp32_wifi.c
  SYS_CONF_SCHEMA += $(MIOT_ESP_PATH)/src/esp32_wifi_config.yaml
endif

include $(MIOT_PATH)/fw/src/sys_config.mk

VPATH += $(MIOT_ESP_PATH)/src $(MIOT_PATH)/common
MIOT_SRCS += cs_crc32.c cs_dbg.c cs_file.c cs_rbuf.c json_utils.c
ifeq "$(MIOT_ENABLE_RPC)" "1"
  VPATH += $(MIOT_PATH)/common/mg_rpc
endif

VPATH += $(MIOT_PATH)/fw/src

VPATH += $(MIOT_PATH)/frozen
MIOT_SRCS += frozen.c

VPATH += $(MIOT_PATH)/mongoose
MIOT_SRCS += mongoose.c

VPATH += $(GEN_DIR)

VPATH += $(APP_MODULES)

APP_SRCS := $(notdir $(foreach m,$(APP_MODULES),$(wildcard $(m)/*.c))) $(APP_EXTRA_SRCS)

COMPONENT_OBJS = $(addsuffix .o,$(basename $(APP_SRCS) $(MIOT_SRCS)))
CFLAGS += $(MIOT_FEATURES) -DMIOT_MAX_NUM_UARTS=3 \
          -DMIOT_DEBUG_UART=$(MIOT_DEBUG_UART) \
          -DMIOT_NUM_GPIO=40 \
          -DMG_ENABLE_FILESYSTEM \
          -DMG_ENABLE_SSL -DMG_SSL_IF=MG_SSL_IF_MBEDTLS \
          -DMG_ENABLE_STREAMING_MULTIPART \
          -DCS_ENABLE_SPIFFS -DMG_ENABLE_DIRECTORY_LISTING
COMPONENT_ADD_LDFLAGS := -Wl,--whole-archive -lsrc -Wl,--no-whole-archive

libsrc.a: $(GEN_DIR)/sys_config.o

./%.o: %.c $(SYS_CONFIG_C) $(SYS_RO_VARS_C)
	$(summary) CC $@
	$(CC) $(CFLAGS) $(CPPFLAGS) \
	  $(addprefix -I ,$(COMPONENT_INCLUDES)) \
	  $(addprefix -I ,$(COMPONENT_EXTRA_INCLUDES)) \
	  -c $< -o $@
