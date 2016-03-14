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
PROVIDE(_p_user_start, 0x40000fb8)
PROVIDE(_c_0x60000200, 0x40000fc4)
PROVIDE(_s_ets_build, 0x40000fc8)   // Jan  8 2013
PROVIDE(_s_ets_banner, 0x40000fcc)  // ets %s,rst cause:%d, boot mode:(%d,%d)
PROVIDE(_s_ets_main_c, 0x40000fd4)  // ets_main.c
PROVIDE(_s_user_code_done, 0x40000fe0)   // user code done
PROVIDE(_s_wdt_reset, 0x40000fe4)   // wdt reset
PROVIDE(_s_unk_reset, 0x40000fe8)   // unknown reset
PROVIDE(_l_boot_mode_not_5, 0x4000104a)
PROVIDE(_l_load_from_flash, 0x400010a8)
PROVIDE(_l_run_user_code, 0x400010be)
PROVIDE(_l_wdt_reset, 0x40001118)
PROVIDE(_l_rr_1_or_2, 0x40001121)
PROVIDE(_c_0x60000600, 0x4000115c)
PROVIDE(_s_waiting_for_host, 0x4000119c)  // waiting for host
PROVIDE(_x_unk1160, 0x40001160)
PROVIDE(_x_unk11ac, 0x400011ac)
PROVIDE(_l_124d, 0x4000124d)
PROVIDE(_l_setup_uart, 0x4000127a)
PROVIDE(_x_load_from_flash, 0x40001308)
PROVIDE(_c_0x00001000, 0x40001994)
PROVIDE(_c_0xffdfffff, 0x400025dc)
PROVIDE(_l_rr_not_dsleep, 0x40002624)
PROVIDE(_c_100000, 0x40002664)
PROVIDE(_l_recv_req, 0x400033a1)
PROVIDE(_l_send_response, 0x400033be)
PROVIDE(_l_send_resp_pkt, 0x400033da)
PROVIDE(_l_process_req, 0x40003424)
PROVIDE(_l_34a3, 0x400034a3)
PROVIDE(_l_34b9, 0x400034b9)
PROVIDE(_c_0x00001800, 0x40003534)
PROVIDE(_c_0x60000a00, 0x40003f54)
PROVIDE(_c_0x20000000, 0x4000416c)
PROVIDE(_c_0x08000000, 0x400043c4)
PROVIDE(_c_0x04000000, 0x400043fc)
PROVIDE(_c_0x40000000, 0x40004438)
PROVIDE(_s_bootup, 0x400054cc)      // bootup , addr 0x%08x
PROVIDE(_p_sip_ctx, 0x40005130)
PROVIDE(_l_sip_cmd_out, 0x4000550a)
PROVIDE(_c_0x00010000, 0x40005de0)

.text

_start:
.incbin "rom.bin"
_end:
