/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

        .section .text.IRAM.esp_nsleep100, "x"

        .align 4
esp_nsleep100_80:
        .global esp_nsleep100_80
        .type   esp_nsleep100_80, @function
.loop100_80:
        blti a2, 2, .out100_80
        nop
        nop
        nop
        addi a2, a2, -1
        j .loop100_80
.out100_80:
        ret

        .align 4
esp_nsleep100_160:
        .global esp_nsleep100_160
        .type   esp_nsleep100_160, @function
.loop100_160:
        blti a2, 2, .out100_160
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        addi a2, a2, -1
        j .loop100_160
.out100_160:
        ret
