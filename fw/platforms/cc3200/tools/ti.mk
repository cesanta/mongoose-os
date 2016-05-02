IPATH += $(SDK_PATH)/third_party/FreeRTOS/source/portable/CCS/ARM_CM3
VPATH += $(SDK_PATH)/third_party/FreeRTOS/source/portable/CCS/ARM_CM3

CC = $(TOOLCHAIN)/bin/armcl
CFLAGS = -mv7M4 --code_state=16 --float_support=vfplib --abi=eabi -me -O4 --opt_for_speed=0 \
         --diag_wrap=off --display_error_number --c99 --gen_func_subsections=on -Dccs
#        --emit_warnings_as_errors
CFLAGS += -I$(TOOLCHAIN)/include

OBJS += $(BUILD_DIR)/startup_ccs.o
OBJS += $(BUILD_DIR)/portasm.o

$(BUILD_DIR)/%.o: %.c
	$(vecho) "TICC  $< -> $@"
	$(Q) $(CC) $(CFLAGS) -c --output_file $@ $<

$(BUILD_DIR)/%.o: %.asm
	$(vecho) "TIAS  $< -> $@"
	$(Q) $(CC) $(CFLAGS) -c --output_file $@ $<

$(APP_ELF):
	$(vecho) "TILD  $@"
	$(Q) $(CC) -mv7M4 --code_state=16 --float_support=vfplib --abi=eabi --little_endian \
	  --run_linker --map_file=$(BUILD_DIR)/$(APP).map --heap_size=0xB000 --stack_size=0x100 \
	  -i $(TOOLCHAIN)/lib \
	  --reread_libs --warn_sections --display_error_number --rom_model \
	  --unused_section_elimination=on \
	  -o $@ $(filter %.o %.a, $^) tools/cc3200v1p32.cmd -l libc.a

