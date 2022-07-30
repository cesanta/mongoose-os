/*
 * Copyright (c) 2022 Deomid "rojer" Ryabkov
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

        .section .iram1.esp32c3_nsleep100_40, "x"
        .align 4
esp32c3_nsleep100_40:
        .global esp32c3_nsleep100_40
        .type   esp32c3_nsleep100_40, @function
        li a1, 2
.loop100_40:
        blt a0, a1, .out100_40
        nop
        addi a0, a0, -1
        j .loop100_40
.out100_40:
        ret

        .section .iram1.esp32c3_nsleep100_80, "x"
        .align 4
esp32c3_nsleep100_80:
        .global esp32c3_nsleep100_80
        .type   esp32c3_nsleep100_80, @function
        li a1, 2
.loop100_80:
        blt a0, a1, .out100_80
        nop
        nop
        nop
        addi a0, a0, -1
        j .loop100_80
.out100_80:
        ret

        .section .iram1.esp32c3_nsleep100_160, "x"
        .align 4
esp32c3_nsleep100_160:
        .global esp32c3_nsleep100_160
        .type   esp32c3_nsleep100_160, @function
        li a1, 4
.loop100_160:
        blt a0, a1, .out100_160
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
        addi a0, a0, -1
        j .loop100_160
.out100_160:
        ret
