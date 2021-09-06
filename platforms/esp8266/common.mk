CFLAGS_EXTRA ?=

XTENSA_TOOLS_ROOT ?= /opt/Espressif/xtensa-lx106-elf/bin
SDK_PATH ?= /opt/Espressif/ESP8266_SDK
ESPTOOL ?= esptool.py
ESPPORT ?= /dev/ttyACM0
ESPSPEED ?= 230400
# For flash = > 16Mbit
ESPFLASHARGS = --flash_mode dio --flash_size 32m

VERBOSE ?= 0

CC := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
CXX := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-g++
AR := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-ar
LD := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
OBJCOPY := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-objcopy
NM := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-nm
CC_WRAPPER ?=

define link
$(vecho) "LD    $@"
$(Q) $(CC_WRAPPER) $(LD) $(LIBDIRS) -T$(LD_SCRIPT) $(LDFLAGS) -o $@ \
-Wl,-Map=$@.map -Wl,--start-group $1 -Wl,--end-group
endef

define compile_params
$(vecho) "$5   $1"
$(Q) $(CC_WRAPPER) $3 -MD -MP $(INCDIRS) $4 -c $1 -o $2
endef

define compile
$(call compile_params,$<,$@, $(CC), $(CFLAGS),"CC ")
endef

define compile_cxx
$(call compile_params,$<,$@, $(CXX), $(CXXFLAGS),"CXX")
endef

C_CXX_FLAGS  = -Wall -Werror -Wundef -Wno-array-bounds \
               -Wno-error=address-of-packed-member \
               -Wno-error=unused-const-variable \
               -Wno-error=strict-aliasing \
               -Wno-error=format-truncation \
               -pipe -Os -g3 \
               -D_XOPEN_SOURCE=500 \
               -nostdlib -mlongcalls -mtext-section-literals -D__ets__ \
               -include mgos_iram.h \
               -DICACHE_RAM_ATTR=IRAM \
               -DNOINSTR='__attribute__((no_instrument_function))' \
               -I/opt/Espressif/xtensa-lx106-elf/xtensa-lx106-elf/sysroot/usr/include \
               -DCS_PLATFORM=CS_P_ESP8266 \
               -ffunction-sections -fdata-sections

CFLAGS = -std=gnu99 $(C_CXX_FLAGS)
CXXFLAGS = -std=gnu++11 -fno-exceptions -fno-rtti $(C_CXX_FLAGS)

# linker flags used to generate the main object file
LDFLAGS = -nostdlib -Wl,--no-check-sections -u call_user_start \
          -Wl,-static -Wl,--gc-sections
