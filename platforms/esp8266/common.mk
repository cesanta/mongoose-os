XTENSA_TOOLS_ROOT ?= /opt/Espressif/crosstool-NG/builds/xtensa-lx106-elf/bin
SDK_PATH ?= /opt/Espressif/ESP8266_SDK
ESPTOOL	?= esptool.py
ESPPORT	?= /dev/ttyACM0
ESPSPEED	?= 230400
# For flash = > 16Mbit
ESPFLASHARGS = --flash_mode dio --flash_size 32m

VERBOSE ?= 0

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

define link
$(vecho) "LD $@"
$(Q) $(LD) $(LIBDIRS) -T$(LD_SCRIPT) $(LDFLAGS) -Wl,--start-group $(LIBS) $1 \
  $< -Wl,--end-group -o $@
endef

define compile_params
$(vecho) "CC $1 -> $2"
$(Q) $(CC) -MD $(INCDIRS) $(CFLAGS) -c $1 -o $2
endef

define compile
$(call compile_params,$<,$@)
endef

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

CFLAGS := -Wall -Werror -Os $(NO_Os_FLAGS) -g3 -Wpointer-arith -Wl,-EL -fno-inline-functions \
          -nostdlib -mlongcalls -mtext-section-literals  -D__ets__ -DSTATIC=static \
          -Wno-parentheses -DFAST='__attribute__((section(".fast.text")))' \
          -ffunction-sections

# linker flags used to generate the main object file
LDFLAGS = -nostdlib -Wl,--no-check-sections -u call_user_start \
          -Wl,-static -Wl,--gc-sections
