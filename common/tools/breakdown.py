#!/usr/bin/python
# Prints code and data breakdown based on given map file
# Usage:
#   ./breakdown.py map_file code_section1,code_section2,... ram_section1,ram_section2...sdk_path_prefix
# Ex:
#   ./breakdown.py ../../fw/examples/mjs_base/build/objs/mjs_base.elf.map .text,.irom0.text .bss,.data,.rodata /opt

import re
import sys

def get_total(content, section_name, sdk_path):
    i, mos_size, sdk_size = 0, 0, 0

    # Skipping all lines before `END GROUP`: in gcc map file
    # non-discarded data goes after `Linker script and memory map` section
    while i != len(content):
        if content[i].startswith("END GROUP"):
            break;
        i += 1

    if i == len(content):
        print "Invalid map file"
        return None

    # Looking for required section
    while i != len(content):
        if content[i].startswith(section_name):
            break
        i += 1

    if i == len(content):
        print "No section", section_name
        # Technically this is not an error
        return 0, 0

    i += 1

    # All entries till next section goes to section_name
    while i != len(content):
        # Entries information starts with " ", next section - with "."
        if not content[i].startswith(" "):
            break;

        data = content[i].strip().split()

        if not data[0].startswith("."):
            i += 1
            continue

        # Some lines are broken into 2 (if symbol name is too long)
        # In this case first line contains only name, trying to add next line
        if len(data) == 1:
            i += 1
            next_data = content[i].split()
            if len(next_data) == 3:
                data.extend(next_data)
            else:
                continue

        # If we got 4 elements, this is probably our data
        # otherwise that is sub-section names
        if data[3].startswith(sdk_path):
            sdk_size += int(data[2], 16)
        else:
            mos_size += int(data[2], 16)
        i += 1

    return mos_size, sdk_size


if __name__ == '__main__':
    map_file = open(sys.argv[1])
    code_sections = sys.argv[2].split(",")
    ram_sections = sys.argv[3].split(",")
    sdk_path = sys.argv[4]

    with map_file as f:
        content = f.readlines()

    mOS_code, SDK_code, mOS_ram, SDK_ram = 0, 0, 0, 0
    mos_size, sdk_size  = 0, 0

    for section_name in code_sections:
        mos_size, sdk_size = get_total(content, section_name, sdk_path)
        mOS_code += mos_size
        SDK_code += sdk_size

    for section_name in ram_sections:
        mos_size, sdk_size = get_total(content, section_name, sdk_path)
        mOS_ram += mos_size
        SDK_ram += sdk_size

    print "mOS code:", mOS_code
    print "SDK code:", SDK_code
    print "Total code:", mOS_code + SDK_code
    print "mOS static ram:", mOS_ram
    print "SDK static ram:", SDK_ram
    print "Total static ram:", mOS_ram + SDK_ram
    print "Total:", mOS_code + SDK_code + mOS_ram + SDK_ram
