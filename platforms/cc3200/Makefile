SDK_PATH ?= /home/rojer/cesanta/cc3200/CC3200SDK_1.1.0/cc3200-sdk
V7_PATH ?= ../../../v7
OBJDIR = obj
BINDIR = exe

include ${SDK_PATH}/tools/gcc_scripts/makedefs

.PHONY: all clean flash

PROG = smartjs

IPATH = ${SDK_PATH} \
        ${SDK_PATH}/inc \
        ${SDK_PATH}/driverlib \
        ${V7_PATH}

VPATH = ${SDK_PATH}/example/common \
        ${SDK_PATH}/example/interrupt \
        ${V7_PATH}

V7_FEATURES = -DV7_BUILD_PROFILE=1 \
              -DV7_DISABLE_AST_TAG_NAMES -DV7_ENABLE_GC \
              -DV7_ENABLE_COMPACTING_GC -DDISABLE_MD5 -DNO_LIBC \
              -DV7_ENABLE__Math -DV7_ENABLE__Math__random \
              -DV7_ENABLE__Memory__stats -DV7_FORCE_STRICT_MODE \
              -DV7_NO_FS

# -DV7_TEMP_OFF
#-DV7_ENABLE_FILE -DV7_ESP_GDB_SERVER \
#-DV7_ENABLE_SPIFFS \
#-DV7_ESP_FLASH_ACCESS_EMUL

V7_MEMORY_FLAGS = -DMBUF_SIZE_MULTIPLIER=1 \
                  -DV7_STACK_SIZE=8192-64 \
                  -DFS_MAX_OPEN_FILES=5

CFLAGS += ${V7_FEATURES} ${V7_MEMORY_FLAGS} -Dc_file_t=int -Os

.PHONY: all clean flash

all: ${OBJDIR} ${BINDIR}
all: ${BINDIR}/${PROG}.axf

clean:
	@rm -rf ${OBJDIR} ${wildcard *~}
	@rm -rf ${BINDIR} ${wildcard *~}

${OBJDIR}:
	@mkdir -p ${OBJDIR}

${BINDIR}:
	@mkdir -p ${BINDIR}

${BINDIR}/${PROG}.axf: ${OBJDIR}/libc_replacements.o ${OBJDIR}/v7.o ${OBJDIR}/main.o
${BINDIR}/${PROG}.axf: ${OBJDIR}/startup_${COMPILER}.o
${BINDIR}/${PROG}.axf: ${OBJDIR}/systick_if.o
${BINDIR}/${PROG}.axf: ${SDK_PATH}/driverlib/${COMPILER}/exe/libdriver.a
SCATTERgcc_${PROG} = ${PROG}.ld
ENTRY_${PROG} = ResetISR

flash:
	cd /home/rojer/cesanta/energia-0101E0016/hardware/tools/lm4f/bin; \
    ./cc3200prog \
      /dev/ttyUSB3 \
      /home/rojer/go/src/cesanta.com/smartjs/platforms/cc3200/${BINDIR}/smartjs.bin
