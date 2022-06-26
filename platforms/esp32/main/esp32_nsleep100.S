/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

        .section .iram1, "x"

        .align 4
esp32_nsleep100_80:
        .global esp32_nsleep100_80
        .type   esp32_nsleep100_80, @function
        entry a1, 16
.loop100_80:
        blti a2, 4, .out100_80
        nop
        addi a2, a2, -1
        j .loop100_80
.out100_80:
        retw

        .align 4
esp32_nsleep100_160:
        .global esp32_nsleep100_160
        .type   esp32_nsleep100_160, @function
        entry a1, 16
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
        addi a2, a2, -1
        j .loop100_160
.out100_160:
        retw

        .align 4
esp32_nsleep100_240:
        .global esp32_nsleep100_240
        .type   esp32_nsleep100_240, @function
        entry a1, 16
.loop100_240:
        blti a2, 2, .out100_240
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
        nop
        nop
        nop
        nop
        nop
        nop
        addi a2, a2, -1
        j .loop100_240
.out100_240:
        nop
        nop
        retw
