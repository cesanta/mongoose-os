#!/usr/bin/python
#
# STM32<->mos GPIOs mapping generator
#
# usage: gen_gpio.py <general HAL file> <gpio HAL file> <output path>
#
# This script generates table for mapping STM32 pins (which are combination
# of GPIO-port_address and GPIO_address-within-port) to MOS pins (aka just
# numbers)
# Script outputs should be located in docker image

import sys
import re

def process_file(file_name, marker):
    ret = []
    with open(file_name) as f:
        for line in f:
            if re.search(marker, line):
                begin = line.find(" ")
                end = line.find(" ", begin + 1)
                name = line[begin + 1:end]
                ret.append(name)
    return ret

if __name__ == '__main__':
    gpio_port_marker = "#define GPIO. "
    gpio_marker = "#define GPIO_PIN_..? "
    stm32_gpio_h = "stm32_gpio_defs.h"
    stm32_gpio_c = "stm32_gpio_defs.c"
    port_file = sys.argv[1]
    gpio_file = sys.argv[2]
    output_path = sys.argv[3]
    ports = process_file(port_file, gpio_port_marker)
    gpios = process_file(gpio_file, gpio_marker)
    gpio_num = len(ports) * len(gpios)

# Writing header
    out = open(output_path + stm32_gpio_h, "w")
# Writing copyright notice
    out.write("/*\n");
    out.write(" * Copyright (c) 2014-2017 Cesanta Software Limited\n")
    out.write(" * All rights reserved\n")
    out.write(" * GENERATED FILE, DO NOT MODIFY\n")
    out.write(" */\n\n")
# Writing include guard
    out.write("#ifndef STM32_GPIO_DEFS_H_\n")
    out.write("#define STM32_GPIO_DEFS_H_\n\n")
# Writing required includes
    out.write("#include <stdint.h>\n\n")
    out.write("#include \"stm32_sdk_hal.h\"\n\n")
# Writing struct definition (we put it here to avoid complicated and unnecessary dependencies)
    out.write("struct stm32_gpio_def {\n  int index;\n  GPIO_TypeDef *port;\
              \n  uint16_t gpio;\n  const char *name;\
              \n  int inited;\n  GPIO_InitTypeDef init_info;\
              \n  IRQn_Type exti;\n  int intr_enabled;\n};\n\n")
# Writing total GPIOs number
    out.write("#define STM32_GPIO_NUM " + str(gpio_num)+"\n\n")
    out.write("#define STM32_PIN_NUM " + str(len(gpios))+"\n\n")
# Writint names
    const_suffix = "STM32_PIN_P"
    number = 0
    for port in ports:
# STM32 uses format GPIOn for ports
        port_index = port[4:]
        for gpio in gpios:
# STM32 uses format GPIO_PIN_n for gpios
            gpio_index = gpio[9:]
            out.write("#define " + const_suffix + port_index \
                       + gpio_index + " " + str(number) + "\n")
            number += 1
# Writing arrays declaration
    out.write("\nextern struct stm32_gpio_def stm32_gpio_defs[STM32_GPIO_NUM];\n")
    out.write("\nextern uint16_t stm32_pins[STM32_PIN_NUM];\n")
# Writing file footer
    out.write("\n#endif /* STM32_GPIO_DEFS_H_ */\n")
    out.write("\n")
    out.close()

# Writing C file
    out = open(output_path + stm32_gpio_c, "w")
    out.write("/* GENERATED FILE, DO NOT MODIFY */\n\n")
# Writint includes
    out.write("#include \"stm32_gpio_defs.h\"\n\n")
# Writing array definition
    out.write("struct stm32_gpio_def stm32_gpio_defs[STM32_GPIO_NUM] = {\n")
    const_suffix = "STM32_PIN_P"
    number = 0
    for port in ports:
# STM32 uses format GPIOn for ports
        port_index = port[4:]
        for gpio in gpios:
# STM32 uses format GPIO_PIN_n for gpios
            gpio_index = gpio[9:]
            exti_index = int(gpio_index)
            exti_name = "EXTI" + gpio_index + "_IRQn";
            if exti_index >=5 and exti_index <=9:
                exti_name = "EXTI9_5_IRQn"
            elif exti_index >= 10 and exti_index <= 15:
                exti_name = "EXTI15_10_IRQn"
            out.write("  {" + const_suffix + port_index + gpio_index \
                       + ", " + port + ", " + gpio + ", " \
                       "\"P" + port_index + gpio_index + "\", 0, {}, " \
                       + exti_name + ", 0},\n")
            number += 1
    out.write("};\n\n")

    out.write("uint16_t  stm32_pins[STM32_PIN_NUM] = {\n")
    for gpio in gpios:
        out.write("  " + gpio + ", \n")
    out.write("};\n\n")

    out.close()
