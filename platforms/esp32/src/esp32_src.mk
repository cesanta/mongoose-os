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

include $(MGOS_PATH)/tools/mk/mgos_common.mk

# Get list of dirs which contain sources (used for IPATH and VPATH)
APP_SOURCE_DIRS = $(sort $(dir $(APP_SOURCES)))

BUILD_INFO_C = $(GEN_DIR)/build_info.c
MG_BUILD_INFO_C = $(GEN_DIR)/mg_build_info.c
MGOS_CONFIG_C = $(GEN_DIR)/mgos_config.c
MGOS_RO_VARS_C = $(GEN_DIR)/mgos_ro_vars.c

FFI_EXPORTS_C = $(GEN_DIR)/ffi_exports.c
FFI_EXPORTS_O = $(BUILD_DIR)/ffi_exports.o

include $(MGOS_PATH)/tools/mk/mgos_build_info.mk
include $(MGOS_PATH)/tools/mk/mgos_config.mk
include $(MGOS_PATH)/tools/mk/mgos_ffi_exports.mk

MGOS_CONF_SCHEMA += $(MGOS_ESP_SRC_PATH)/esp32_sys_config.yaml

MGOS_SRCS += mgos_config_util.c mgos_core_dump.c mgos_dlsym.c mgos_event.c \
             mgos_gpio.c mgos_init.c mgos_mmap_esp.c \
             mgos_sys_config.c $(notdir $(MGOS_CONFIG_C)) $(notdir $(MGOS_RO_VARS_C)) \
             mgos_file_utils.c mgos_hw_timers.c mgos_system.c mgos_system.cpp \
             mgos_time.c mgos_timers.c mgos_timers.cpp mgos_uart.c mgos_utils.c mgos_utils.cpp \
             esp32_debug.c esp32_exc.c esp32_fs_crypt.c \
             esp32_gpio.c esp32_hal.c esp32_hw_timers.c \
             esp32_main.c esp32_uart.c \
             error_codes.cpp status.cpp

VPATH += $(MGOS_ESP_SRC_PATH) $(MGOS_PATH)/common \
         $(MGOS_PATH)/common/platforms/esp/src

MGOS_SRCS += cs_crc32.c cs_file.c cs_hex.c cs_rbuf.c cs_varint.c json_utils.c mgos_json_utils.cpp

VPATH += $(MGOS_VPATH)

MGOS_SRCS += frozen.c

VPATH += $(GEN_DIR)

VPATH += $(APP_SOURCE_DIRS)

APP_SRCS := $(notdir $(foreach m,$(APP_SOURCES),$(wildcard $(m)))) $(APP_EXTRA_SRCS)
APP_BIN_LIB_FILES := $(foreach m,$(APP_BIN_LIBS),$(wildcard $(m)))

MGOS_OBJS = $(addsuffix .o,$(MGOS_SRCS)) esp32_nsleep100.S.o
APP_OBJS = $(addsuffix .o,$(APP_SRCS))
BUILD_INFO_OBJS = $(addsuffix .o,$(notdir $(BUILD_INFO_C)) $(notdir $(MG_BUILD_INFO_C)))

C_CXX_CFLAGS += -pipe -DMGOS_APP=\"$(APP)\" -DFW_ARCHITECTURE=$(APP_PLATFORM) \
                -DMGOS_ESP32 -include mgos_iram.h \
                $(MG_FEATURES_TINY) -DMG_NET_IF=MG_NET_IF_LWIP_LOW_LEVEL \
                $(MGOS_FEATURES) -DMGOS_MAX_NUM_UARTS=3 \
                -DMGOS_DEBUG_UART=$(MGOS_DEBUG_UART) \
                -DMG_ENABLE_FILESYSTEM \
                -DMG_ENABLE_DIRECTORY_LISTING \
                -DMGOS_NUM_HW_TIMERS=4 \
                -fno-jump-tables -fno-tree-switch-conversion

CFLAGS += $(C_CXX_CFLAGS)
CXXFLAGS += $(C_CXX_CFLAGS) -fno-exceptions -fno-rtti

$(BUILD_INFO_C): $(MGOS_OBJS) $(APP_OBJS)
	$(call gen_build_info,$@,$(APP_PATH),$(APP_BUILD_ID),$(APP_VERSION),,$(BUILD_INFO_C),$(BUILD_INFO_JSON))

$(MG_BUILD_INFO_C): $(MGOS_OBJS)
	$(call gen_build_info,$@,$(MGOS_PATH)/fw,,,mg_,$(MG_BUILD_INFO_C),)

# In ffi exports file we use fake signatures: void func(void), and it conflicts
# with the builtin functions like fopen, etc.
$(FFI_EXPORTS_O): CFLAGS += -fno-builtin

$(FFI_EXPORTS_O): $(FFI_EXPORTS_C)
	$(summary) "CC $@"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(FFI_EXPORTS_C): $(APP_FS_FILES)
	$(call gen_ffi_exports,$@,$(FFI_SYMBOLS),$(filter %.js,$(FS_FILES)))

./%.S.o: %.S
	$(summary) "AS $@"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

./%.c.o: %.c $(MGOS_CONFIG_C) $(MGOS_RO_VARS_C)
	$(summary) "CC $@"
	$(CC) $(CFLAGS) $(CPPFLAGS) \
	  $(addprefix -I ,$(COMPONENT_INCLUDES)) \
	  $(addprefix -I ,$(COMPONENT_EXTRA_INCLUDES)) \
	  -c $< -o $@

./%.cpp.o: %.cpp $(MGOS_CONFIG_C) $(MGOS_RO_VARS_C)
	$(summary) "CXX $@"
	$(CC) $(CXXFLAGS) $(CPPFLAGS) \
	  $(addprefix -I ,$(COMPONENT_INCLUDES)) \
	  $(addprefix -I ,$(COMPONENT_EXTRA_INCLUDES)) \
	  -c $< -o $@

COMPONENT_EXTRA_INCLUDES = $(MGOS_ESP_SRC_PATH) $(MGOS_ESP_PATH)/include $(MGOS_ESP_PATH)/include/spiffs \
                           $(GEN_DIR) $(sort $(APP_SOURCE_DIRS)) $(MGOS_IPATH) \
                           $(IDF_PATH)/components/freertos/include/freertos
