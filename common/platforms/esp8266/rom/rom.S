.text
.org 0
.globl _start

// xtensa-esp108-elf-gcc -Wl,-N,-Ttext,0x40000000 -nostdlib rom.S -o rom.elf

here = .
#define PROVIDE(name, addr) name = here + addr - 0x40000000

#include "rom_functions.S"

PROVIDE(SPI_chip_erase, 0x40004080)
PROVIDE(SPIFlashModeConfig, 0x40004568)

PROVIDE(_c_0x80000000, 0x400003a4)
PROVIDE(_c_0x60000200, 0x40000fc4)
PROVIDE(_c_0x00001000, 0x40001994)
PROVIDE(_c_0x00001800, 0x40003534)
PROVIDE(_c_0x60000a00, 0x40003f54)
PROVIDE(_c_0x20000000, 0x4000416c)
PROVIDE(_c_0x08000000, 0x400043c4)
PROVIDE(_c_0x04000000, 0x400043fc)
PROVIDE(_c_0x40000000, 0x40004438)
PROVIDE(_c_0x00010000, 0x40005de0)

.text

_start:
.incbin "rom.bin"
_end:
