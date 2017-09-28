#
# Component makefile.
#

override APP_CONF_SCHEMA = $(_APP_CONF_SCHEMA)
override APP_EXTRA_SRCS = $(_APP_EXTRA_SRCS)
override APP_FS_FILES = $(_APP_FS_FILES)
override APP_BIN_LIBS = $(_APP_BIN_LIBS)
override FS_FILES = $(_FS_FILES)
override APP_SOURCES = $(_APP_SOURCES)
override APP_INCLUDES = $(_APP_INCLUDES)
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

FFI_EXPORTS_C = $(GEN_DIR)/ffi_exports.c
FFI_EXPORTS_O = $(BUILD_DIR)/ffi_exports.o

NM = xtensa-esp32-elf-nm

MGOS_SRCS += mgos_config.c mgos_dlsym.c mgos_gpio.c mgos_hooks.c mgos_init.c \
             mgos_mmap_esp.c mgos_mongoose.c \
             mgos_sys_config.c $(notdir $(SYS_CONFIG_C)) $(notdir $(SYS_RO_VARS_C)) \
             mgos_timers_mongoose.c mgos_uart.c mgos_utils.c \
             mgos_vfs.c mgos_vfs_dev.c mgos_vfs_fs_spiffs.c \
             esp32_crypto.c esp32_debug.c esp32_exc.c esp32_fs.c esp32_fs_crypt.c \
             esp32_vfs_dev_partition.c \
             esp32_gpio.c esp32_hal.c esp32_hw_timer.c \
             esp32_main.c esp32_mdns.c esp32_uart.c

include $(MGOS_PATH)/fw/common.mk
include $(MGOS_PATH)/common/scripts/ffi_exports.mk

SYS_CONF_SCHEMA += $(MGOS_ESP_SRC_PATH)/esp32_config.yaml

ifeq "$(MGOS_ENABLE_UPDATER)" "1"
  MGOS_SRCS += esp32_updater.c
endif

include $(MGOS_PATH)/common/scripts/build_info.mk

include $(MGOS_PATH)/fw/src/sys_config.mk

SYS_CONF_SCHEMA += $(MGOS_ESP_SRC_PATH)/esp32_sys_config.yaml

VPATH += $(MGOS_ESP_SRC_PATH) $(MGOS_PATH)/common \
         $(MGOS_PATH)/common/platforms/esp/src

MGOS_SRCS += cs_crc32.c cs_dbg.c cs_file.c cs_rbuf.c json_utils.c

VPATH += $(MGOS_PATH)/fw/src

VPATH += $(MGOS_PATH)/frozen
MGOS_SRCS += frozen.c

VPATH += $(MGOS_PATH)/mongoose
MGOS_SRCS += mongoose.c

VPATH += $(GEN_DIR)

VPATH += $(APP_SOURCE_DIRS)

APP_SRCS := $(notdir $(foreach m,$(APP_SOURCES),$(wildcard $(m)))) $(APP_EXTRA_SRCS)
APP_BIN_LIB_FILES := $(foreach m,$(APP_BIN_LIBS),$(wildcard $(m)))

MGOS_OBJS = $(addsuffix .o,$(basename $(MGOS_SRCS))) esp32_nsleep100.o
APP_OBJS = $(addsuffix .o,$(basename $(APP_SRCS)))
BUILD_INFO_OBJS = $(addsuffix .o,$(basename $(notdir $(BUILD_INFO_C)) $(notdir $(MG_BUILD_INFO_C))))

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
                -DCS_DISABLE_SHA1 -DMG_EXT_SHA1

                # TODO(dfrank): add support for encryption in mmap, and uncomment
                #-DCS_MMAP -DSPIFFS_ON_PAGE_MOVE_HOOK=mgos_vfs_mmap_spiffs_on_page_move_hook

CFLAGS += $(C_CXX_CFLAGS)
CXXFLAGS += -std=c++11 -fno-exceptions $(C_CXX_CFLAGS)

$(BUILD_INFO_C): $(MGOS_OBJS) $(APP_OBJS)
	$(call gen_build_info,$@,,$(APP_BUILD_ID),$(APP_VERSION),,$(BUILD_INFO_C),$(BUILD_INFO_JSON))

$(MG_BUILD_INFO_C): $(MGOS_OBJS)
	$(call gen_build_info,$@,$(MGOS_PATH)/fw,,,mg_,$(MG_BUILD_INFO_C),)

libsrc.a: $(GEN_DIR)/sys_config.o

# In ffi exports file we use fake signatures: void func(void), and it conflicts
# with the builtin functions like fopen, etc.
$(FFI_EXPORTS_O): CFLAGS += -fno-builtin

$(FFI_EXPORTS_O): $(FFI_EXPORTS_C)
	$(summary) "CC $@"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(FFI_EXPORTS_C): $(APP_FS_FILES)
	$(call gen_ffi_exports,$@,$(FFI_SYMBOLS),$(filter %.js,$(FS_FILES)))

./%.o: %.S
	$(summary) "CC $@"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

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

COMPONENT_EXTRA_INCLUDES = $(MGOS_ESP_SRC_PATH) $(MGOS_PATH) $(MGOS_SRC_PATH) $(MGOS_ESP_PATH)/include \
                           $(SPIFFS_PATH) $(GEN_DIR) $(sort $(APP_SOURCE_DIRS) $(APP_INCLUDES)) $(IPATH)
