XTENSA_TOOLS_ROOT ?= /opt/Espressif/crosstool-NG/builds/xtensa-lx106-elf/bin
SDK_PATH ?= /opt/Espressif/ESP8266_SDK
SMARTJS_PATH ?= ../..
KRYPTON_PATH ?= ../../../krypton
SMARTJS_SRC ?= $(SMARTJS_PATH)/src
SPIFFS_PATH ?= $(SMARTJS_SRC)/spiffs
SJSJS_PATH ?= $(SMARTJS_SRC)/js
ESPTOOL	?= esptool.py
ESPPORT	?= /dev/ttyACM0
ESPSPEED	?= 230400
VERBOSE ?= 0
# Should be RTOS or OSS
SDK := $(shell echo $$SDK_FLAVOUR)
ifndef SDK
$(error Use make.sh to build project within docker container. To build against locally \
        installed ESP SDK define SDK_FLAVOUR (with RTOS or OSS value) enviroment variable)
endif

BUILD_DIR = build
TOOLS_DIR = tools
COMMON_TOOLS_DIR ?= ../../tools
FW_DIR = firmware
FS_DIR = fs
FS_BUILD_DIR = $(BUILD_DIR)/fs
APP = smartjs
# SSL can be SDK or Krypton
SSL ?= None

APP_MODULES := user $(SPIFFS_PATH)
APP_SRCS := $(notdir $(foreach m,$(APP_MODULES),$(wildcard $(m)/*.c))) \
            sj_prompt.c v7.c sj_v7_ext.c sj_simple_http.c \
            sj_i2c_js.c sj_spi_js.c sj_wifi.c sj_gpio_js.c sj_timers.c
JSBIN_SRCS := $(SJSJS_PATH)/clubby.js $(SJSJS_PATH)/sys_rts.js $(SJSJS_PATH)/cloud.js $(SJSJS_PATH)/swupdate.js
JS_SRCS    := $(SJSJS_PATH)/smart.js $(SJSJS_PATH)/demo.js $(SJSJS_PATH)/user.js
# In order to keep compatibility
# with shared JS-files, which can call "File.eval(....)"
# using JS as extension
JSBIN_EXT := js
JS_EXT := js
VPATH = $(APP_MODULES) $(SMARTJS_SRC)
IPATH = $(VPATH) $(SDK_PATH)/include $(SDK_PATH)/include/json
LPATH = $(SDK_PATH)/lib

# libraries used in this project, mainly provided by the SDK
APP_LIBS = cirom gcc hal pp phy net80211 lwip wpa main

# linker script used for the above linkier step
LD_SCRIPT = user/eagle.app.30000.v6.ld

# linker flags used to generate the main object file
LDFLAGS = -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static

FW_FILE_1_ADDR = 0x00000
FW_FILE_2_ADDR = 0x11000

# spiffs parameters
FW_FS_ADDR = 0x6d000
FW_FS_SIZE = 65536

# some of these flags works around for gdb 7.5.x stacktrace issue
# while still allowing -Os to remove padding between data in .rodata
# section, allowing us to gain about 1k of ram.
# text section is 4k bigger, but we care more about ram at the moment.
# TODO(mkm): figure out which flag(s).
NO_Os_FLAGS= -fno-expensive-optimizations -fno-thread-jumps \
             -fno-align-functions -fno-align-jumps \
             -fno-align-loops -fno-align-labels -fno-caller-saves \
             -fno-crossjumping -fno-cse-follow-jumps -fno-cse-skip-blocks \
             -fno-delete-null-pointer-checks -fno-devirtualize  \
             -fno-gcse -fno-gcse-lm -fno-hoist-adjacent-loads \
             -fno-inline-small-functions -fno-indirect-inlining -fno-partial-inlining \
             -fno-ipa-cp -fno-ipa-sra -fno-peephole2 -fno-optimize-sibling-calls -fno-optimize-strlen \
             -fno-reorder-blocks -fno-reorder-blocks-and-partition -fno-reorder-functions \
             -fno-sched-interblock -fno-sched-spec -fno-rerun-cse-after-loop \
             -fno-schedule-insns -fno-schedule-insns2 -fno-strict-aliasing -fno-strict-overflow \
             -fno-tree-builtin-call-dce -fno-tree-switch-conversion -fno-tree-tail-merge \
             -fno-tree-pre -fno-tree-vrp

SHIMS = -DNDEBUG

# NDEBUG is required because we don't have an assert shim
# TODO(mkm): make one

FEATURES = -DV7_BUILD_PROFILE=1 -DV7_ENABLE_FILE -DESP_GDB_SERVER \
           -DESP_COREDUMP -DV7_DISABLE_AST_TAG_NAMES -DV7_TEMP_OFF \
           -DV7_ENABLE_COMPACTING_GC -DDISABLE_MD5 -DNO_BSD_SOCKETS \
           -DV7_ENABLE__Math -DV7_ENABLE__Math__random \
           -DV7_ENABLE__Memory__stats -DV7_ENABLE_SPIFFS \
           -DESP_FLASH_BYTES_EMUL -DV7_FORCE_STRICT_MODE \
           -DV7_ENABLE__File__list=1 -DV7_ENABLE__StackTrace=1 \
           $(FEATURES_EXTRA)

MONGOOSE_FEATURES = -DMG_DISABLE_MQTT -DMG_DISABLE_MD5 \
                 -DMG_DISABLE_JSON_RPC -DMG_DISABLE_SOCKETPAIR -DMG_DISABLE_DAV \
                 -DMG_DISABLE_DIRECTORY_LISTING -DMG_DISABLE_CGI \
                 -DMG_DISABLE_FILESYSTEM

MEMORY_FLAGS = -DMBUF_SIZE_MULTIPLIER=1 -DV7_STACK_SIZE=8192-64 -DFS_MAX_OPEN_FILES=5

# compiler flags using during compilation of source files
CFLAGS = -Wall -Werror -Os $(NO_Os_FLAGS) -g3 -Wpointer-arith -Wl,-EL -fno-inline-functions \
         -nostdlib -mlongcalls -mtext-section-literals  -D__ets__ -DSTATIC=static \
         -Wno-parentheses -DICACHE_FLASH -DMG_ESP8266 \
         -DFAST='__attribute__((section(".fast.text")))' \
         -DFS_ADDR=$(FW_FS_ADDR) -DFS_SIZE=$(FW_FS_SIZE) \
         $(MEMORY_FLAGS) $(SHIMS) $(FEATURES) $(CFLAGS_EXTRA) \
         $(MONGOOSE_FEATURES)

##
## == No config vars beyond this point.
##

.PHONY: all clean

ifeq "$(SSL)" "SDK"
FEATURES += -DESP_SSL_SDK
APP_LIBS += ssl
else ifeq "$(SSL)" "Krypton"
IPATH += $(KRYPTON_PATH)
VPATH += $(KRYPTON_PATH)
APP_SRCS += krypton.c
FEATURES += -DESP_SSL_KRYPTON -DKR_EXT_IO -DKR_EXT_RANDOM -DKR_EXT_MD5 -DKR_EXT_SHA1 -DKRYPTON_DEBUG -DESP_SSL_KRYPTON_DEBUG
endif

CC := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
AR := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-ar
LD := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
OBJCOPY := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-objcopy
# Temp solution for RTOS SDK
OBJCOPY2 = $(OBJCOPY)

ifeq "$(SDK)" "RTOS"
IPATH += $(SDK_PATH)/include/espressif/esp8266 $(SDK_PATH)/include/espressif \
         $(SDK_PATH)/include/lwip $(SDK_PATH)/include/lwip/ipv4 \
         $(SDK_PATH)/include/lwip/ipv6 $(SDK_PATH)/extra_include \
         $(SDK_PATH)/lwip/include/

APP_SRCS += mongoose.c sj_mongoose_http_client.c \
            sj_simple_http.c sj_mongoose.c sj_mongoose_ws_client.c

APP_LIBS += freertos mirom
# -fno-builtin-printf avoid that the compiler "optimizes"
# some printf calls: e.g.
#   printf("foo\n") -> puts("foo")
#   printf("%c", ch) -> <something_that_calls_esp_stuff_not_libc>
CFLAGS += -DRTOS_SDK -DRTOS_TODO -fno-builtin-printf
ifneq "$(NETTEST)" ""
CFLAGS += -DRTOS_NETWORK_TEST
endif
OBJCOPY2 = /bin/true
#TODO(alashkin): try to use the same LD script
LD_SCRIPT = user/eagle.app.v6.RTOS.ld
else
APP_LIBS += m
endif

APP_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(APP_SRCS))
SRCS = $(APP_SRCS) sj_version.c
OBJS = $(APP_OBJS) $(BUILD_DIR)/sj_version.o
LIBS = $(addprefix -l,$(APP_LIBS))
APP_AR = $(BUILD_DIR)/$(APP).a
APP_OUT = $(BUILD_DIR)/$(APP).out
SYMCHECK_OUT = $(BUILD_DIR)/$(APP)_symcheck.out
SYMCHECK_FLAG = $(BUILD_DIR)/$(APP)_symcheck.ok
FW_FILE_1_OUT = $(FW_DIR)/$(FW_FILE_1_ADDR).bin
FW_FILE_2_OUT = $(FW_DIR)/$(FW_FILE_2_ADDR).bin
FW_FS_OUT = $(FW_DIR)/$(FW_FS_ADDR).bin
FS_FILES = $(wildcard $(FS_DIR)/*)

INCDIRS = $(addprefix -I,$(IPATH))
LIBDIRS = $(addprefix -L,$(LPATH))

V ?= $(VERBOSE)
ifeq ("$(V)","1")
Q :=
else
Q := @
endif
vecho := @echo " "

all: $(FW_DIR) $(BUILD_DIR) $(FW_FILE_1_OUT) $(FW_FILE_2_OUT) $(FW_FS_OUT)
	$(Q) rm -f `ls $(FW_DIR)/* \
    | grep -vE '^($(FW_FILE_1_OUT)|$(FW_FILE_2_OUT)|$(FW_FS_OUT))$$'`

$(BUILD_DIR) $(FW_DIR):
	$(Q) mkdir -p $@

# Main output product: firmware files.
$(FW_FILE_1_OUT) $(FW_FILE_2_OUT): $(APP_OUT)
	$(vecho) "FW $(APP_OUT) -> $(FW_DIR)"
	$(Q) $(ESPTOOL) elf2image -o $(FW_DIR)/ $(APP_OUT)

# -- Linking.

ifeq "$(SDK)" "RTOS"
LDFLAGS += -Wl,--wrap,user_fatal_exception_handler
endif

define link
$(vecho) "LD $@"
$(Q) $(LD) $(LIBDIRS) -T$(LD_SCRIPT) $(LDFLAGS) -Wl,--start-group $(LIBS) \
  $< -Wl,--end-group -o $@
endef

# Link the main ELF output file. Performs symbol clash check beforehand.
$(APP_OUT): $(APP_AR) $(SYMCHECK_FLAG)
	$(link)

# Symbol check: make sure app doesn't override SDK lib symbols.
# A target is created that uses the same libs but no user code and symbols are
# checked against main app library to make sure there's no overlap.
$(SYMCHECK_FLAG): $(APP_AR) $(SYMCHECK_OUT)
	$(vecho) "SYMCHECK $(SYMCHECK_OUT) vs $(APP_AR)"
	$(Q) python $(TOOLS_DIR)/symcheck.py $(SYMCHECK_OUT) $(APP_AR)
	$(Q) touch $@

$(SYMCHECK_OUT): $(BUILD_DIR)/dummy_main.o
	$(link)

$(BUILD_DIR)/dummy_main.c: $(APP_OBJS)
	$(vecho) "GEN $@"
	$(Q) echo "void user_init(void) {}; void __wrap_user_fatal_exception_handler() {}" > $@


# Main app lib.
$(APP_AR): $(OBJS)
	$(vecho) "AR $@"
	$(Q) $(AR) cru $@ $^
# Move all code to flash by default.
	$(Q) $(OBJCOPY) --rename-section .text=.irom0.text $(APP_AR)
	$(Q) $(OBJCOPY) --rename-section .fast.text=.text $(APP_AR)
# Move strings to flash too, we have the handler.
	$(Q) $(OBJCOPY) --rename-section .rodata=.irom0.text $(APP_AR)
	$(Q) $(OBJCOPY2) --rename-section .rodata.str1.1=.irom0.text $(APP_AR)

# -- Compilation.

define compile
$(vecho) "CC $< -> $@"
$(Q) $(CC) -MD $(INCDIRS) $(CFLAGS) -c $< -o $@
endef

# This rule is for normal (pre-existing) sources from VPATH.
$(BUILD_DIR)/%.o: %.c
	$(compile)

# This one is for generated sources in build directory.
$(BUILD_DIR)/%.o: $(BUILD_DIR)/%.c
	$(compile)

$(BUILD_DIR)/mongoose.o: mongoose.c
	$(compile) -DEXCLUDE_COMMON

# Generate version. Only regenrates if there are changes in objects.
$(BUILD_DIR)/sj_version.c: $(APP_OBJS)
	$(vecho) "GEN $@"
	@REF=$$(git symbolic-ref HEAD | sed 's:refs/heads/::'); \
	SHA=$$(git rev-parse HEAD | head -c 8); \
	DATE=$$(TZ=GMT date +"%Y%m%d-%H%M%S"); \
	echo "const char *sj_version = \"ESP8266-$$REF/$$DATE/$$SHA\";" > $@

# Pull in auto-generated .c dependencies (if any).
-include $(wildcard $(BUILD_DIR)/*.d)

# -- Filesystem.

$(FS_BUILD_DIR):
	@mkdir -p $(FS_BUILD_DIR)

$(FW_FS_OUT): $(FS_BUILD_DIR) $(FS_FILES) $(JSBIN_SRCS) $(BUILD_DIR)/mkspiffs $(BUILD_DIR)/v7
	$(Q) $(foreach jsbin,$(JSBIN_SRCS), \
		echo "V7 $(jsbin) -> $(FS_DIR)/$(basename $(notdir $(jsbin))).$(JSBIN_EXT)";\
		$(BUILD_DIR)/v7 -b $(jsbin) > $(FS_BUILD_DIR)/$(basename $(notdir $(jsbin))).$(JSBIN_EXT);)
	/usr/bin/rsync -r --copy-links $(FS_DIR)/ $(FS_BUILD_DIR)/
# OSX virtualbox shared folder have issues with some symlinks. JS_SRCS contains their canonical location
	$(Q) $(foreach js,$(JS_SRCS), \
		echo "CP $(js) -> $(FS_DIR)/$(basename $(notdir $(js))).$(JS_EXT)";\
		cp $(js) $(FS_BUILD_DIR);)
	$(vecho) "MKSPIFFS $(FS_DIR) -> $@"
	$(Q) $(BUILD_DIR)/mkspiffs $(FW_FS_SIZE) $(FS_BUILD_DIR) > $@

$(BUILD_DIR)/mkspiffs: $(COMMON_TOOLS_DIR)/mkspiffs.c $(wildcard $(SPIFFS_PATH)/*.c)
	$(vecho) "GCC mkspiffs"
	$(Q) gcc -I. -I$(SPIFFS_PATH) -Iuser -o $@ $^ -DNO_ESP

$(BUILD_DIR)/v7: $(SMARTJS_SRC)/v7.c $(SMARTJS_SRC)/v7.h
	$(vecho) "GCC v7"
	$(Q) gcc -DV7_EXE -DV7_EXPOSE_PRIVATE $(SMARTJS_SRC)/v7.c -lm -o $(BUILD_DIR)/v7
# Target specific warning inhibitions

build/spiffs_hydrogen.o: CFLAGS += -Wno-unused-function

#
# Auxiliary targets.
#

clean:
	$(Q) rm -rf $(BUILD_DIR) $(FW_DIR)

flash: $(FW_OUT) $(FW_FS_OUT)
	$(Q) $(ESPTOOL) --port $(ESPPORT) --baud $(ESPSPEED) write_flash \
                  $(FW_FILE_1_ADDR) $(FW_FILE_1_OUT) \
                  $(FW_FILE_2_ADDR) $(FW_FILE_2_OUT) \
                  $(FW_FS_ADDR) $(FW_FS_OUT)

flash_no_fs: $(FW_OUT)
	$(Q) $(ESPTOOL) --port $(ESPPORT) --baud $(ESPSPEED) write_flash \
                  $(FW_FILE_1_ADDR) $(FW_FILE_1_OUT) \
                  $(FW_FILE_2_ADDR) $(FW_FILE_2_OUT)

flash_fs: $(FW_FS_OUT)
	$(Q) $(ESPTOOL) --port $(ESPPORT) --baud $(ESPSPEED) write_flash \
                  $(FW_FS_ADDR) $(FW_FS_OUT)
