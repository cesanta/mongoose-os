#!/usr/bin/env python

import subprocess
import glob
import os
import os.path
import sys

path_to_bin = '.'
build_path = os.path.join(path_to_bin, 'build')
app_bin_path = [f for f in glob.glob(os.path.join(build_path, '*.out')) if '_symcheck' not in f]
# in order to work with make clean verifying app.out existence and exiting without an error
if not app_bin_path:
    sys.exit(0)
app_bin_path = app_bin_path[0]
app_name = os.path.basename(app_bin_path).split('.')[0]
app_lib_path = os.path.join(path_to_bin, 'build', '%s.a' % app_name)
v7_path = os.path.join(path_to_bin, 'build', 'v7.o')
have_v7 = os.path.exists(v7_path)

def print_obj_map(title, results):
    print title
    for seg, size in results.items():
        print " ", seg, ":", size

def print_results(title, funcs, objects):
    print title
    print "========================"
    print_obj_map("Functions:", funcs)
    print_obj_map("Objects:", objects)
    print ""

def print_row(fmt, title, funcs, objects):
    print row_format.format(title, funcs.get('.text', 0),funcs.get('.irom0.text', 0),
                            objects.get('.rodata', 0), objects.get('.data', 0), objects.get('.bss', 0))

def process_objdump_res(symb_table):
    funcs = {}
    objects = {}
    others_size = 0

    for line in symb_table:
        line_tbl = line.split()
        if len(line_tbl) == 6:
            if line_tbl[2] in ('F', 'O'):
                size = int(line_tbl[4], 16)
                size += 4 - (size % 3)  # round up to 4 due to alignment reqs
                funcs[line_tbl[3]] = funcs.get(line_tbl[3], 0) + size
            elif line_tbl[2] == "O":
                objects[line_tbl[3]] = objects.get(line_tbl[3], 0) + int(line_tbl[4], 16)
            else:
                others_size += int(line_tbl[4],16)

    return funcs, objects, others_size

def sub_res(full_res, part_res):
    results = {}
    for seg, size in full_res.items():
        results[seg] = full_res.get(seg, 0) - part_res.get(seg, 0)
        if results[seg] < 0:
            results[seg] = 0
    return results

symb_table = subprocess.check_output(['xtensa-lx106-elf-objdump', '-t', app_lib_path]).split('\n')
u_funcs, u_objects, u_others_size = process_objdump_res(symb_table)

if have_v7:
    symb_table = subprocess.check_output(['xtensa-lx106-elf-objdump', '-t', v7_path]).split('\n')
    v7_funcs, v7_objects, v7_others_size = process_objdump_res(symb_table)

    # Text sections was copied to flash text sections as a part the build process.
    # Here we're remapping them manually because we use objcopy only for `.a` file
    # and doesn't use it for `.o`
    v7_funcs['.irom0.text'] = v7_funcs.get('.text', 0);
    v7_objects['.irom0.text'] = v7_objects.get('.text', 0);
    v7_objects['.itom0.text'] = v7_objects.get('.itom0.text',0) + v7_objects.get('.rodata', 0);
    v7_objects['.rodata'] = 0;
    v7_funcs['.text'] = v7_funcs.get('.fast.text', 0);
    v7_objects['.text'] = v7_objects.get('.fast.text', 0);
else:
    v7_funcs, v7_objects, v7_others_size = {}, {}, 0

symb_table = subprocess.check_output(['xtensa-lx106-elf-objdump', '-t', app_bin_path]).split('\n')
all_funcs, all_objects, all_others_size = process_objdump_res(symb_table)

# linker merges irom.text and irom0.text, but since it is different data
# it might be useful to show both
all_objects['.irom.text'] = u_objects.get('.irom.text', 0)

# system = all - all user
s_funcs = sub_res(all_funcs, u_funcs)
s_objects = sub_res(all_objects, u_objects)
s_others_size = all_others_size - u_others_size

# user = user - v7
u_funcs = sub_res(u_funcs, v7_funcs)
u_objects = sub_res(u_objects, v7_objects)

headers = [".text", ".irom0.text", ".rodata", ".data", ".bss"]
row_format = "{:<15}" * (len(headers)+1)
print ""
print "%s footprint breakdown" % app_name
print "---------------------------------"
print row_format.format("", *headers)
if have_v7:
    print_row(row_format, "v7", v7_funcs, v7_objects)
print_row(row_format, app_name, u_funcs, u_objects)
print_row(row_format, "sys", s_funcs, s_objects)
print_row(row_format, "total", all_funcs, all_objects)

if all_others_size !=0:
    print "Others: ", all_others_size

bin_files = glob.glob(path_to_bin + "/firmware/*.bin")
flash_size = 0

for f in bin_files:
    flash_size += os.path.getsize(f)

print ""
print "Firmware size is {}Kb (of 512Kb), {}% is available".format(flash_size/1024 , 100 - flash_size*100/(512*1024))
print ""
