#
# Component makefile.
#

override APP_CONF_SCHEMA = $(_APP_CONF_SCHEMA)
override APP_EXTRA_SRCS = $(_APP_EXTRA_SRCS)
override APP_FS_FILES = $(_APP_FS_FILES)
override FS_FILES = $(_FS_FILES)
override APP_SOURCES = $(_APP_SOURCES)
override BUILD_DIR = $(_BUILD_DIR)
override FW_DIR := $(_FW_DIR)
override GEN_DIR := $(_GEN_DIR)
override MGOS_PATH = $(_MGOS_PATH)

# Get list of dirs which contain sources (used for IPATH and VPATH)
APP_SOURCE_DIRS = $(sort $(dir $(APP_SOURCES)))

MGOS_DEBUG_UART ?= 0

BUILD_INFO_C = $(GEN_DIR)/build_info.c
MG_BUILD_INFO_C = $(GEN_DIR)/mg_build_info.c
SYS_CONFIG_C = $(GEN_DIR)/sys_config.c
SYS_CONFIG_DEFAULTS_JSON = $(GEN_DIR)/conf0.json
SYS_CONFIG_SCHEMA_JSON = $(GEN_DIR)/sys_config_schema.json
SYS_RO_VARS_C = $(GEN_DIR)/sys_ro_vars.c
SYS_RO_VARS_SCHEMA_JSON = $(GEN_DIR)/sys_ro_vars_schema.json

SYMBOLS_DUMP = $(GEN_DIR)/symbols_dump.txt
FFI_EXPORTS_C = $(GEN_DIR)/ffi_exports.c
FFI_EXPORTS_O = $(BUILD_DIR)/ffi_exports.o

NM = xtensa-esp32-elf-nm

COMPONENT_EXTRA_INCLUDES = $(MGOS_PATH) $(MGOS_ESP_PATH)/include $(SPIFFS_PATH) \
                           $(GEN_DIR) $(APP_SOURCE_DIRS)

MGOS_SRCS += mgos_config.c mgos_gpio.c mgos_hooks.c mgos_init.c mgos_mongoose.c \
             mgos_sys_config.c $(notdir $(SYS_CONFIG_C)) $(notdir $(SYS_RO_VARS_C)) \
             mgos_timers_mongoose.c mgos_uart.c mgos_utils.c mgos_dlsym.c \
             esp_mmap.c \
             esp32_crypto.c esp32_debug.c esp32_exc.c esp32_fs.c esp32_fs_crypt.c \
             esp32_gpio.c esp32_hal.c \
             esp32_main.c esp32_mdns.c esp32_mmap.c esp32_uart.c

include $(MGOS_PATH)/fw/common.mk
include $(MGOS_PATH)/common/scripts/ffi_exports.mk

SYS_CONF_SCHEMA += $(MGOS_ESP_SRC_PATH)/esp32_config.yaml

ifeq "$(MGOS_ENABLE_ARDUINO_API)" "1"
  MGOS_FEATURES += -DESP32 -DF_CPU=160000000
endif
ifeq "$(MGOS_ENABLE_DNS_SD)" "1"
  SYS_CONF_SCHEMA += $(MGOS_ESP_SRC_PATH)/esp32_dns_sd_config.yaml
endif
ifeq "$(MGOS_ENABLE_I2C)" "1"
  ifneq "$(MGOS_ENABLE_I2C_GPIO)" "1"
    MGOS_SRCS += esp32_i2c_master.c
    SYS_CONF_SCHEMA += $(MGOS_ESP_SRC_PATH)/esp32_i2c_config.yaml
  else
    SYS_CONF_SCHEMA += $(MGOS_ESP_SRC_PATH)/esp32_i2c_gpio_config.yaml
  endif
endif
ifeq "$(MGOS_ENABLE_MQTT)" "1"
  SYS_CONF_SCHEMA += $(MGOS_ESP_SRC_PATH)/esp32_mqtt_config.yaml
endif
ifeq "$(MGOS_ENABLE_SPI)" "1"
  ifneq "$(MGOS_ENABLE_SPI_GPIO)" "1"
    MGOS_SRCS += esp32_spi_master.c
    SYS_CONF_SCHEMA += $(MGOS_ESP_SRC_PATH)/esp32_spi_config.yaml
  else
    SYS_CONF_SCHEMA += $(MGOS_ESP_SRC_PATH)/esp32_spi_gpio_config.yaml
  endif
endif
ifeq "$(MGOS_ENABLE_UPDATER)" "1"
  MGOS_SRCS += esp32_updater.c
endif
ifeq "$(MGOS_ENABLE_WIFI)" "1"
  MGOS_SRCS += esp32_wifi.c
  SYS_CONF_SCHEMA += $(MGOS_ESP_SRC_PATH)/esp32_wifi_config.yaml
endif

include $(MGOS_PATH)/common/scripts/build_info.mk

include $(MGOS_PATH)/fw/src/sys_config.mk

SYS_CONF_SCHEMA += $(MGOS_ESP_SRC_PATH)/esp32_sys_config.yaml

VPATH += $(MGOS_ESP_SRC_PATH) $(MGOS_PATH)/common \
         $(MGOS_PATH)/common/platforms/esp/src

MGOS_SRCS += cs_crc32.c cs_dbg.c cs_file.c cs_rbuf.c json_utils.c
ifeq "$(MGOS_ENABLE_RPC)" "1"
  VPATH += $(MGOS_PATH)/common/mg_rpc
endif

VPATH += $(MGOS_PATH)/fw/src

VPATH += $(MGOS_PATH)/frozen
MGOS_SRCS += frozen.c

VPATH += $(MGOS_PATH)/mongoose
MGOS_SRCS += mongoose.c

VPATH += $(GEN_DIR)

VPATH += $(APP_SOURCE_DIRS)

APP_SRCS := $(notdir $(foreach m,$(APP_SOURCES),$(wildcard $(m)))) $(APP_EXTRA_SRCS)

MGOS_OBJS = $(addsuffix .o,$(basename $(MGOS_SRCS))) esp32_nsleep100.o
APP_OBJS = $(addsuffix .o,$(basename $(APP_SRCS)))
BUILD_INFO_OBJS = $(addsuffix .o,$(basename $(notdir $(BUILD_INFO_C)) $(notdir $(MG_BUILD_INFO_C))))
COMPONENT_OBJS = $(MGOS_OBJS) $(APP_OBJS) $(FFI_EXPORTS_O) $(BUILD_INFO_OBJS)

C_CXX_CFLAGS += -DMGOS_APP=\"$(APP)\" -DFW_ARCHITECTURE=$(APP_PLATFORM) \
                -DIRAM='__attribute__((section(".iram1")))' \
                $(MG_FEATURES_TINY) -DMG_NET_IF=MG_NET_IF_LWIP_LOW_LEVEL \
                $(MGOS_FEATURES) -DMGOS_MAX_NUM_UARTS=3 \
                -DMGOS_DEBUG_UART=$(MGOS_DEBUG_UART) \
                -DMGOS_NUM_GPIO=40 \
                -DMG_ENABLE_FILESYSTEM \
                -DMG_ENABLE_SSL -DMG_SSL_IF=MG_SSL_IF_MBEDTLS \
                -DMG_SSL_IF_MBEDTLS_FREE_CERTS \
                -DMG_ENABLE_DIRECTORY_LISTING \
                -DCS_DISABLE_MD5 -DMG_EXT_MD5 \
                -DCS_DISABLE_SHA1 -DMG_EXT_SHA1 \
                -DCS_MMAP

CFLAGS += $(C_CXX_CFLAGS) $(APP_CFLAGS)
CXXFLAGS += -std=c++11 -fno-exceptions $(C_CXX_CFLAGS) $(APP_CXXFLAGS)
COMPONENT_EXTRA_INCLUDES += $(IPATH)

COMPONENT_ADD_LDFLAGS := -Wl,--whole-archive -lsrc -Wl,--no-whole-archive

$(BUILD_INFO_C): $(MGOS_OBJS) $(APP_OBJS)
	$(call gen_build_info,$@,,$(APP_BUILD_ID),$(APP_VERSION),,$(BUILD_INFO_C),$(BUILD_INFO_JSON))

$(MG_BUILD_INFO_C): $(MGOS_OBJS)
	$(call gen_build_info,$@,$(MGOS_PATH)/fw,,,mg_,$(MG_BUILD_INFO_C),)

libsrc.a: $(GEN_DIR)/sys_config.o

$(SYMBOLS_DUMP): $(MGOS_OBJS) $(APP_OBJS)
	$(vecho) "GEN $@"
	$(NM) --defined-only --print-file-name -g $^ > $@

# In ffi exports file we use fake signatures: void func(void), and it conflicts
# with the builtin functions like fopen, etc.
$(FFI_EXPORTS_O): CFLAGS += -fno-builtin

$(FFI_EXPORTS_O): $(FFI_EXPORTS_C)
	$(summary) "CC $@"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(FFI_EXPORTS_C): $(SYMBOLS_DUMP) $(FS_FILES)
	$(call gen_ffi_exports,$<,$@,$(FFI_SYMBOLS),$(filter %.js,$(FS_FILES)))

./%.o: %.c $(SYS_CONFIG_C) $(SYS_RO_VARS_C)
	$(summary) "CC $@"
	$(CC) $(CFLAGS) $(CPPFLAGS) \
	  $(addprefix -I ,$(COMPONENT_INCLUDES)) \
	  $(addprefix -I ,$(COMPONENT_EXTRA_INCLUDES)) \
	  -c $< -o $@

./%.o: %.cpp $(SYS_CONFIG_C) $(SYS_RO_VARS_C)
	$(summary) "CXX $@"
	$(CC) $(CXXFLAGS) $(CPPFLAGS) \
	  $(addprefix -I ,$(COMPONENT_INCLUDES)) \
	  $(addprefix -I ,$(COMPONENT_EXTRA_INCLUDES)) \
	  -c $< -o $@
