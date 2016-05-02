CC = arm-none-eabi-gcc

IPATH += $(SDK_PATH)/third_party/FreeRTOS/source/portable/GCC/ARM_CM4
VPATH += $(SDK_PATH)/third_party/FreeRTOS/source/portable/GCC/ARM_CM4

CFLAGS = -mthumb -mcpu=cortex-m4 -ffunction-sections -fdata-sections \
         -MD -std=c99 -Os -Wall -Werror -Dgcc

OBJS += $(BUILD_DIR)/startup_gcc.o

$(SDK_OBJS): CFLAGS += -include common/platform.h

AR = arm-none-eabi-ar
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy
LDFLAGS = --gc-sections
LIBGCC := ${shell ${CC} -mthumb ${CFLAGS} -print-libgcc-file-name}
LIBC := ${shell ${CC} ${CFLAGS} -print-file-name=libc.a}
LIBM := ${shell ${CC} ${CFLAGS} -print-file-name=libm.a}

# Disable certain warnings on SDK sources, we have no control over them anyway.
# We aloc force-include platform.h which resolves some symbol conflicts
# between system includes and simplelink.h
$(SDK_OBJS): CFLAGS += -Wno-missing-braces -Wno-strict-aliasing -Wno-parentheses -Wno-unused-variable -Wno-builtin-macro-redefined

$(BUILD_DIR)/%.o: %.c
	$(vecho) "GCC   $< -> $@"
	$(Q) $(CC) $(CFLAGS) -c -o $@ $<

$(APP_ELF):
	$(vecho) "LD    $@"
	$(Q) $(LD) --script=src/$(APP).ld --entry=ResetISR \
	           --gc-sections -o $@ $(filter %.o %.a, $^) \
	           $(LIBM) $(LIBC) $(LIBGCC)

