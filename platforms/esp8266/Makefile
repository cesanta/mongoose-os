XTENSA_TOOLS_ROOT ?= /opt/Espressif/crosstool-NG/builds/xtensa-lx106-elf/bin
SDK_PATH ?= /opt/Espressif/ESP8266_SDK
V7_PATH ?= ../../src
COMMON_PATH ?= ../../src
SMARTJS_PATH ?= ../..
KRYPTON_PATH ?= ../../../krypton
ESPTOOL	?= esptool.py
ESPPORT	?= /dev/ttyACM0
ESPSPEED	?= 230400
VERBOSE ?= 0

BUILD_DIR = build
TOOLS_DIR = tools
FW_DIR = firmware
FS_DIR = fs
APP = smartjs
# SSL can be SDK or Krypton
SSL ?= None

APP_MODULES := user spiffs
APP_SRCS := $(notdir $(foreach m,$(APP_MODULES),$(wildcard $(m)/*.c))) \
            sj_prompt.c v7.c sj_v7_ext.c sj_conf.c sj_simple_http.c sj_i2c_js.c

VPATH = $(APP_MODULES) $(V7_PATH) $(SMARTJS_PATH)/src
IPATH = $(VPATH) $(SDK_PATH)/include $(SDK_PATH)/include/json $(COMMON_PATH)
LPATH = $(SDK_PATH)/lib

# libraries used in this project, mainly provided by the SDK
APP_LIBS = m cirom gcc hal pp phy net80211 lwip wpa main

# linker script used for the above linkier step
LD_SCRIPT	= user/eagle.app.30000.v6.ld

# linker flags used to generate the main object file
LDFLAGS		= -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static

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
 
SHIMS = -Dprintf=c_printf -Dfprintf=c_ufprintf \
        -DINVALID_FILE=-1 -Dc_file_t=int \
        -Dc_fread=spiffs_fread -Dc_fwrite=spiffs_fwrite \
        -Dc_fclose=spiffs_fclose -Dc_fopen=spiffs_fopen \
	      -Dc_rename=spiffs_rename -Dc_remove=spiffs_remove  \
        -Dc_fseek=spiffs_fseek -Dc_ftell=spiffs_ftell \
        -Dc_rewind=spiffs_rewind -Dc_ferror=spiffs_ferror \
        -Dhmac_sha1=v7_hmac_sha1 \
	      -DNDEBUG

# NDEBUG is required because we don't have an assert shim
# TODO(mkm): make one

FEATURES = -DV7_BUILD_PROFILE=1 -DV7_ENABLE_FILE -DV7_ESP_GDB_SERVER \
           -DV7_DISABLE_AST_TAG_NAMES -DV7_TEMP_OFF -DV7_ENABLE_GC \
	         -DV7_ENABLE_COMPACTING_GC -DDISABLE_MD5 -DNO_LIBC \
					 -DV7_ENABLE__Math -DV7_ENABLE__Math__random \
					 -DV7_ENABLE__Memory__stats -DV7_ENABLE_SPIFFS \
					 -DV7_ESP_FLASH_ACCESS_EMUL -DV7_FORCE_STRICT_MODE

MEMORY_FLAGS = -DMBUF_SIZE_MULTIPLIER=1 -DV7_STACK_SIZE=8192-64 -DFS_MAX_OPEN_FILES=5

# compiler flags using during compilation of source files
CFLAGS = -Wall -Os $(NO_Os_FLAGS) -g3 -Wpointer-arith -Wl,-EL -fno-inline-functions \
         -nostdlib -mlongcalls -mtext-section-literals  -D__ets__ -DSTATIC=static \
         -DICACHE_FLASH \
         -DFAST='__attribute__((section(".fast.text")))' \
         -DFS_ADDR=$(FW_FS_ADDR) -DFS_SIZE=$(FW_FS_SIZE) \
         $(MEMORY_FLAGS) $(SHIMS) $(FEATURES) $(CFLAGS_EXTRA)

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
FEATURES += -DESP_SSL_KRYPTON -DKR_EXT_IO -DKR_EXT_RANDOM -DKR_EXT_MD5 -DKR_EXT_SHA1
#-DKRYPTON_DEBUG -DESP_SSL_KRYPTON_DEBUG
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

CC := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
AR := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-ar
LD := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
OBJCOPY := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-objcopy

V ?= $(VERBOSE)
ifeq ("$(V)","1")
Q :=
else
Q := @
endif
vecho := @echo " "

all: $(FW_DIR) $(BUILD_DIR) $(FW_FILE_1_OUT) $(FW_FILE_2_OUT) $(FW_FS_OUT)
	$(Q) rm -f `ls $(FW_DIR)/* | \
		grep -vE '^($(FW_FILE_1_OUT)|$(FW_FILE_2_OUT)|$(FW_FS_OUT))$$'`

$(BUILD_DIR) $(FW_DIR):
	$(Q) mkdir -p $@


# Main output product: firmware files.
$(FW_FILE_1_OUT) $(FW_FILE_2_OUT): $(APP_OUT)
	$(vecho) "FW $(APP_OUT) -> $(FW_DIR)"
	$(Q) $(ESPTOOL) elf2image -o $(FW_DIR)/ $(APP_OUT)


# -- Linking.

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
	$(Q) echo "void user_init(void) {}" > $@


# Main app lib.
$(APP_AR): $(OBJS)
	$(vecho) "AR $@"
	$(Q) $(AR) cru $@ $^
	$(Q) $(OBJCOPY) --rename-section .text=.irom0.text $(APP_AR)
	$(Q) $(OBJCOPY) --rename-section .fast.text=.text $(APP_AR)
	$(Q) $(OBJCOPY) --rename-section .rodata=.irom0.text $(APP_AR)
	$(Q) $(OBJCOPY) --rename-section .rodata.str1.1=.irom0.text $(APP_AR)
	
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

$(FW_FS_OUT): $(FS_FILES) $(BUILD_DIR)/mkspiffs
	$(vecho) "MKSPIFFS $(FS_DIR) -> $@"
	$(Q) $(BUILD_DIR)/mkspiffs $(FW_FS_SIZE) $(FS_DIR) > $@

$(BUILD_DIR)/mkspiffs: $(TOOLS_DIR)/mkspiffs.c $(wildcard spiffs/*.c)
	$(vecho) "GCC mkspiffs"
	$(Q) gcc -I. -o $@ $^ -DNO_ESP


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
