#!/usr/bin/python

import subprocess;

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

def process_objdump(symb_table):
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

def correct_sys_res(user_res, sys_res):
    results = {};
    for seg, size in sys_res.items():
        results[seg] = sys_res.get(seg, 0) - user_res.get(seg, 0);
        if results[seg] < 0:
            results[seg] = 0;
    return results;

symb_table = subprocess.check_output(['xtensa-lx106-elf-objdump', '-t', './build/app_app.a']).split('\n')
u_funcs, u_objects, u_others_size = process_objdump(symb_table)

symb_table = subprocess.check_output(['xtensa-lx106-elf-objdump', '-t', './build/app.out']).split('\n')
all_funcs, all_objects, all_others_size = process_objdump(symb_table)
all_objects['.irom.text'] = u_objects.get('.irom.text', 0);

s_funcs = correct_sys_res(u_funcs, all_funcs);
s_objects = correct_sys_res(u_objects, all_objects);
s_others_size = all_others_size - u_others_size;

headers = [".text", ".irom0.text", ".irom.text", ".rodata", ".data", ".bss"];
row_format = "{:<15}" * (len(headers)+1)
print row_format.format("", *headers);
print_row(row_format, "usr", u_funcs, u_objects);
print_row(row_format, "sys", s_funcs, s_objects);
print_row(row_format, "total", all_funcs, all_objects);

if all_others_size !=0:
    print "Others: ", all_others_size
