IPATH += $(SDK_PATH)/third_party/FreeRTOS/source/portable/CCS/ARM_CM3
VPATH += $(SDK_PATH)/third_party/FreeRTOS/source/portable/CCS/ARM_CM3

CC_WRAPPER ?=
CC = $(TOOLCHAIN)/bin/armcl

CFLAGS = --c99 -mv7M4 --little_endian --code_state=16 --float_support=vfplib --abi=eabi \
         -O4 --opt_for_speed=0 --unaligned_access=on --small_enum \
         --gen_func_subsections=on --diag_wrap=off --display_error_number \
         --emit_warnings_as_errors -Dccs
CFLAGS += -I$(TOOLCHAIN)/include

OBJS += $(BUILD_DIR)/startup_ccs.o
OBJS += $(BUILD_DIR)/portasm.o

$(BUILD_DIR)/%.o: %.c
	$(vecho) "TICC  $< -> $@"
	$(Q) $(CC_WRAPPER) $(CC) \
	  -c --output_file=$@ --preproc_with_compile -ppd=$@.d $(CFLAGS) $<

$(BUILD_DIR)/%.o: %.asm
	$(vecho) "TIASM $< -> $@"
	$(Q) $(CC_WRAPPER) $(CC) $(CFLAGS) -c --output_file=$@ $<

$(APP_ELF):
	$(vecho) "TILD  $@"
	$(Q) $(CC_WRAPPER) $(CC) \
	  -mv7M4 --code_state=16 --float_support=vfplib --abi=eabi --little_endian \
	  --run_linker --map_file=$(BUILD_DIR)/$(APP).map \
	  -i $(TOOLCHAIN)/lib \
	  --reread_libs --warn_sections --display_error_number \
	  --ram_model --cinit_compression=off --copy_compression=off \
	  --unused_section_elimination=on \
	  -o $@ --map_file=$@.map --xml_link_info=$@.map.xml \
	  $(filter %.o %.a, $^) $(APP_LDFLAGS)
