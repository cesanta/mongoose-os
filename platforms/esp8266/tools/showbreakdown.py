#!/usr/bin/python

import subprocess;
import glob;
import os;

path_to_bin = "."

def print_obj_map(title, results):
    print title;
    for seg, size in results.items():
        print " ", seg, ":", size

def print_results(title, funcs, objects):
    print title;
    print "========================";
    print_obj_map("Functions:", funcs);
    print_obj_map("Objects:", objects);
    print ""

def print_row(fmt, title, funcs, objects):
    print row_format.format(title, funcs.get('.text', 0),funcs.get('.irom0.text', 0), objects.get('.irom.text', 0),
                            objects.get('.rodata', 0), objects.get('.data', 0), objects.get('.bss', 0));

def process_objdump_res(symb_table):
    funcs = {};
    objects = {};
    others_size = 0;

    for line in symb_table:
        line_tbl = line.split();
        if len(line_tbl) == 6:
            if line_tbl[2] == "F":
                funcs[line_tbl[3]] = funcs.get(line_tbl[3],0) + int(line_tbl[4],16);
            elif line_tbl[2] == "O":
                objects[line_tbl[3]] = objects.get(line_tbl[3],0) + int(line_tbl[4],16);
            else:
                others_size += int(line_tbl[4],16);

    return funcs, objects, others_size;

def sub_res(full_res, part_res):
    results = {};
    for seg, size in full_res.items():
        results[seg] = full_res.get(seg, 0) - part_res.get(seg, 0);
        if results[seg] < 0:
            results[seg] = 0;
    return results;


symb_table = subprocess.check_output(['xtensa-lx106-elf-objdump', '-t', path_to_bin + '/build/app_app.a']).split('\n')
u_funcs, u_objects, u_others_size = process_objdump_res(symb_table)

symb_table = subprocess.check_output(['xtensa-lx106-elf-objdump', '-t', path_to_bin + '/build/user/v7.o']).split('\n')
v7_funcs, v7_objects, v7_others_size = process_objdump_res(symb_table)

symb_table = subprocess.check_output(['xtensa-lx106-elf-objdump', '-t', path_to_bin + '/build/app.out']).split('\n')
all_funcs, all_objects, all_others_size = process_objdump_res(symb_table)

# linker merges irom.text and irom0.text, but since it is different data
# it might be useful to show both
all_objects['.irom.text'] = u_objects.get('.irom.text', 0);

# system = all - all user
s_funcs = sub_res(all_funcs, u_funcs);
s_objects = sub_res(all_objects, u_objects);
s_others_size = all_others_size - u_others_size;

# user = user - v7
u_funcs = sub_res(u_funcs, v7_funcs)
u_objects = sub_res(u_objects, v7_objects);

headers = [".text", ".irom0.text", ".irom.text", ".rodata", ".data", ".bss"];
row_format = "{:<15}" * (len(headers)+1)
print "smart.js footprint breakdown";
print "---------------------------------";
print row_format.format("", *headers);
print_row(row_format, "v7", v7_funcs, v7_objects);
print_row(row_format, "smartjs", u_funcs, u_objects);
print_row(row_format, "sys", s_funcs, s_objects);
print_row(row_format, "total", all_funcs, all_objects);

if all_others_size !=0:
    print "Others: ", all_others_size

bin_files = glob.glob(path_to_bin + "/firmware/*.bin")
flash_size = 0

for f in bin_files:
    flash_size += os.path.getsize(f);

print ""
print "Firmware size is {}Kb (of 512Kb), {}% is available".format(flash_size/1024 , 100 - flash_size*100/(512*1024))
