APP_LDFLAGS ?=
CC_WRAPPER ?=
GENFILES_LIST ?=
CC = arm-none-eabi-gcc
AR = arm-none-eabi-ar

IPATH += $(SDK_PATH)/third_party/FreeRTOS/source/portable/GCC/ARM_CM4
VPATH += $(SDK_PATH)/third_party/FreeRTOS/source/portable/GCC/ARM_CM4

CFLAGS = -mthumb -mcpu=cortex-m4 -ffunction-sections -fdata-sections \
         -MD -std=c99 -Os -Wall -Werror -Dgcc

AR = arm-none-eabi-ar
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy
LIBGCC := ${shell ${CC} -mthumb ${CFLAGS} -print-libgcc-file-name}
LIBC := ${shell ${CC} ${CFLAGS} -print-file-name=libc.a}
LIBM := ${shell ${CC} ${CFLAGS} -print-file-name=libm.a}

# Disable certain warnings on SDK sources, we have no control over them anyway.
# We also force-include platform.h which resolves some symbol conflicts
# between system includes and simplelink.h
$(SDK_OBJS): CFLAGS += -Wno-missing-braces -Wno-strict-aliasing -Wno-parentheses -Wno-unused-variable -Wno-builtin-macro-redefined
$(SDK_OBJS): CFLAGS += -include common/platform.h

# cc flags,file
define cc
	$(vecho) "GCC   $2 -> $@"
	$(Q) $(CC_WRAPPER) $(CC) -c $1 -o $@ $2
endef

# ar files
define ar
	$(vecho) "AR    $@"
	$(Q) $(AR) cru $@ $1
endef

# link script,flags,objs
define link
	$(vecho) "LD    $@"
	$(Q) $(CC_WRAPPER) $(LD) \
	  --gc-sections -o $@ -T $1 $2 $3 \
	  $(LIBM) $(LIBC) $(LIBGCC)
endef
