#!/usr/bin/env python
#
# This script takes a dump of a JS heap performed by V7_FREEZE and turns it
# into a C file that defines the same object graph.
#
# The generated C file is meant to be compiled with V7_THAW. It will contain a
# static definition of the whole object graph contained in the JS heap. The
# root of the object graph is exposed in the `fr_vals` global symbol. That
# structure can be used to fill in `v7->vals`.
#
# The main goal of freezing is to offload a large chunk of library code
# (functions and objects) on memory mapped flash and thus reducing
# the ram footprint at the cost of making the library object graph
# read only (and thus breaking ecma compliance given that it breaks the ability
# to monkeypatch the stdlib which ecma test writers are so fond of).
#
# This script is not specific to v7 stdlib; the same technique can be used to
# offload a snapshot of the post initialization state of any V7 application,
# e.g. Smart.js. See freeze.c for details.
#
# This script preserves all the object and property attributes so it's the
# freeze dump generator's responsibility to set the readonly and sealed
# attributes as appropriate.
#
# Usage:
#
# 1. build a host `v7` binary compiled with the same feature set as the one
#    on the target device.
#
# 2. dump the post-init object graph in a file:
#
#    $ v7 -freeze fr.dump
#
# 3. dump the symbols of the `v7` binary:
#
#    $ nm v7 >fr.sym
#
# 4. generate C with the frozen object graph:
#
#    $ scripts/freeze.py fr.dump fr.sym >fr.c
#
# 5. build the firmware with -DV7_THAW. Include fr.c among the sources
#
#
# Details:
#
# The input object graph is a sequence of JSON objects one per line.  Each JSON
# object represents a JS entity on the heap: objects, JS functions, properties,
# bcode structures, byte arrays with bcode vec contents (literals, opcodes,
# names).
#
# Each heap entity is identified by its address in the address space of the
# dumping process. It's thus meaningless for the target but it's used as
# transient ID for the purpose of reconstructing the object graph.
#
# The target object graph is defined as C global variables of V7 internal
# datastructures.
#
# Each structure references other structures statically and the linker will
# make assign address and put the actual addresses in the target binary. To
# restate: the addresses used as object IDs are irrelevant for the target
# binary.
#
# V7's values are defined as `uint64_t` and are encoded with NaN packing.
# Unfortunately it's not possible to generate tagged values in initializers
# since C has very restrictive rules on the kind of constant expressions
# allowed in static initializers.  For this reason the v7 values here are
# represented as `static_val` structures which split the 64-bit value in two
# 32-bit halves, one for the tag and one for the payload (here endianness
# matters).
#
# 20k foot code overview:
#
# 1. Emits C preamble with includes and definitions.
# 2. Emits forward declarations: C function prototypes and object/property/...
#    structures
# 3. Emits bcode data (ops, lits and names) and bcode structures. opcodes and
#    names are just plain byte arrays, while the lits array is defined as a
#    val_t array so that it can contain pointers to function symbols (used as
#    closure prototypes).
# 4. Emits objects (referencing symbols of property structures and prototype
#    objects)
# 5. Emits function objects, pointing to bcode and scope objects (for closures)
# 6. Emits properties, pointing to next properties and value objects.
#    Non object values and strings are just defined as uint64_t literal values.
# 7. Emits a table of val_t mirroring the `v7->vals` structure. This is the
#    root set of the object graph.
#
# Caveats:
#
# Some parts of this script duplicate definitions from V7 internals.
#
# It currently requires the unamalgamated headers to be accessible while
# building the FW but it can be trivially solved by either concatenating the
# output of this script to the amalgamated v7.c or by other means.
#
# It currently doesn't handle non-dictionary strings.
#
# It currently works only for 32-bit little endian architectures.
# It's trivial to make it work on 16-bit archs and to support big-endian mode.
#

import argparse
import base64
import json
import string
import sys

parser = argparse.ArgumentParser(
    description = 'Generate C code for a V7 freeze dump'
)
parser.add_argument("dump")
parser.add_argument("syms")

#
# Some defs to be kept in sync with vm.h :-(
#

# poor python's enum:
V7_TYPE_UNDEFINED,\
V7_TYPE_NULL,\
V7_TYPE_BOOLEAN,\
V7_TYPE_NUMBER,\
V7_TYPE_STRING,\
V7_TYPE_FOREIGN,\
V7_TYPE_CFUNCTION,\
V7_TYPE_GENERIC_OBJECT,\
V7_TYPE_BOOLEAN_OBJECT,\
V7_TYPE_STRING_OBJECT,\
V7_TYPE_NUMBER_OBJECT,\
V7_TYPE_FUNCTION_OBJECT,\
V7_TYPE_CFUNCTION_OBJECT,\
V7_TYPE_REGEXP_OBJECT,\
V7_TYPE_ARRAY_OBJECT,\
V7_TYPE_DATE_OBJECT,\
V7_TYPE_ERROR_OBJECT,\
V7_TYPE_MAX_OBJECT_TYPE,\
V7_NUM_TYPES = range(19)

V7_TAG_OBJECT    = "0xFFFF"
V7_TAG_FUNCTION  = "0xFFF5"
V7_TAG_CFUNCTION = "0xFFF4"
V7_TAG_STRING_O  = "0xFFF8"
V7_TAG_STRING_F  = "0xFFF7"

#
# load input data
#

args = parser.parse_args()

syms = {}
for l in open(args.syms):
    s = l.split()
    if s[1] in "tT":
        func = s[2]
        if func.startswith("_"):
            func = func[1:]
        syms[int(s[0], 16)] = func

glob = []
props = []
objs = []
funcs = []
bcodes = []

f = open(args.dump)
for l in f:
    entry = json.loads(l)
    if entry["type"]== "prop":
        props.append(entry)
    elif entry["type"] == "obj":
        objs.append(entry)
    elif entry["type"] == "func":
        funcs.append(entry)
    elif entry["type"] == "bcode":
        bcodes.append(entry)
    else:
        glob.append(entry)

#
# generate preamble
#

print '#define V7_EXPORT_INTERNAL_HEADERS'
print '#include "v7/v7.c"'
print '''
#define STATIC_V7_UNDEFINED {0, 0, 0xFFFD}

/* little endian */
struct static_val {
  const void *p;
  uint16_t hi;
  uint16_t tag;
};

V7_STATIC_ASSERT(sizeof(struct static_val) == sizeof(uint64_t), ONLY_32_BIT_ARCH);

union u_val {
  struct static_val svalue; /* Property value */
  val_t value;
};

struct v7_fr_property {
  struct v7_fr_property *next;
  v7_prop_attr_t attributes;
  entity_id_t entity_id;
  union u_val name;  /* Property name (a string) */
  union u_val v;
};

'''

#
# C code generation helpers
#

def is_null(v):
    return v == "0x0" or v == "(nil)"


def num(v):
    return int(v, 16)


def untag(v):
    return int(num(v) & 0xFFFFFFFFFFFF)


def get_tag(v):
    return int(num(v)) >> 48


def ref(prefix, r, opt=False):
    if opt and is_null(r):
        return "0x0"
    return "&%s_%s" % (prefix, r)


def name(prefix, n):
    return "%s_%s" % (prefix, n)


def base_obj(o):
    return "{(struct v7_property *) %(name)s, %(attrs)s, %(entity_id_base)s, %(entity_id_spec)s}" % dict(
        name = ref("fprop", o["props"], opt=True),
        entity_id_base = o["entity_id_base"],
        entity_id_spec = o["entity_id_spec"],
        attrs = o["attrs"],
    )


def check_value(v):
    if get_tag(v) in [num(V7_TAG_STRING_O), num(V7_TAG_STRING_F)]:
        print >>sys.stderr, "Unhandled owned string during freezing. Please add it to string dictionary in v7/src/string.c"
        print >>sys.stderr, "(Sorry I can't tell you which string it is, otherwise I would have handled it in the first place,"
        print >>sys.stderr, "but I have a hunch it might be a literal in a frozen JS lib)"
        sys.exit(1)


def gen_svalue(tag, exp):
    '''
    a static value is a workaround for C's inability to treat
    `0xFFFF000000000000 | (uint64_t)some_uint32` as a constant expression
    '''
    return "{.svalue = {%s, 0, %s}}" % (exp, tag)


def gen_fstr(s):
    '''
    a static value is a workaround for C's inability to construct
    constant expressions via bitwise ops
    '''
    assert len(s) <= 0xFFFF
    return "{.svalue = {%s, %s, %s}}" % ('"%s"' % (escape_c_str(s),), len(s), V7_TAG_STRING_F)


def gen_value(v):
    check_value(v)
    return "{.value = %s}" % (v,)


def gen_uvalue(typ, v):
    check_value(v)
    if typ == V7_TYPE_CFUNCTION:
        return gen_svalue(V7_TAG_CFUNCTION, syms[untag(v)])
    elif typ == V7_TYPE_FUNCTION_OBJECT:
        return gen_svalue(V7_TAG_FUNCTION, ref("ffunc", hex(untag(v))))
    elif typ >= V7_TYPE_GENERIC_OBJECT:
        return gen_svalue(V7_TAG_OBJECT, ref("fobj", hex(untag(v))))
    else:
        return gen_value(v)


# possibly escape a python character using the C string literal rules
def escape_c_char(ch):
    if ch not in string.printable:
        return "\\" + oct(ord(ch))[1:].zfill(3)
    return ch

# escape a python string using the C string string literal rules
def escape_c_str(s):
    return "".join(escape_c_char(i) for i in s)


#
# forward declarations
#

for p in props:
    if p["value_type"] == V7_TYPE_CFUNCTION:
        print "enum v7_err %(name)s(struct v7*v7, v7_val_t *res);" % dict(
            name = syms[untag(p["value"])],
        )

for p in props:
    print "static const struct v7_fr_property fprop_%s;" % (p["addr"],)

for o in objs:
    print "static const struct v7_generic_object fobj_%s;" % (o["addr"],)

for f in funcs:
    print "static const struct v7_js_function ffunc_%s;" % (f["addr"],)

#
# definitions
#

def vec_initializer(prefix, addr, data):
    return "{(char *) %(ref)s, %(len)s}" % dict(
        ref = ref(prefix, addr),
        len = len(data),
    )


def vec(prefix, addr, buf):
    '''
    Dump a vec as a char array containing hex numbers.
    '''
    data = base64.decodestring(buf);
    print "static const char %(prefix)s_%(addr)s[] = {%(values)s};" % dict(
        prefix = prefix,
        addr = addr,
        values = ','.join(hex(ord(i)) for i in data),
    )
    return vec_initializer(prefix, addr, data)


def lit_vec(prefix, addr, records):
    def val(r):
        if 'val' in r:
            return u_val(r['val'])
        elif 'str' in r:
            return gen_fstr(r['str'])
        else:
            raise Exception("invalid value")

    def u_val(v):
        if get_tag(v) == int(V7_TAG_FUNCTION, 16):
            return gen_svalue(V7_TAG_FUNCTION, ref("ffunc", hex(untag(v))))
        return gen_value(v)

    vals = [val(i) for i in records]

    print "static const union u_val %(prefix)s_%(addr)s[] = {%(values)s};" % dict(
        prefix = prefix,
        addr = addr,
        values = ','.join(vals),
    )
    return "{(char *) %(ref)s, %(len)s * sizeof(v7_val_t)}" % dict(
        ref = ref(prefix, addr),
        len = len(records),
    )


for b in bcodes:
    a = b["addr"]
    ops = vec("fops", a, b["ops"])
    lits = lit_vec("flits", a, b["lit"])
    print ("static const struct bcode fbcode_%(addr)s"
           " = {%(ops)s, %(lits)s\n"
           "#if !V7_DISABLE_FILENAMES\n"
           ", NULL\n"
           "#endif\n"
           ", 0xff, %(names_cnt)s, %(args_cnt)s, %(strict_mode)s, 1, 1, 0, %(func_name_present)s\n"
           "#if !V7_DISABLE_FILENAMES\n"
           ", 0\n"
           "#endif\n"
           "};") % dict(
        addr = b["addr"],
        ops = ops,
        lits = lits,
        args_cnt = b["args_cnt"],
        names_cnt = b["names_cnt"],
        strict_mode = b["strict_mode"],
        func_name_present = b["func_name_present"],
    )

for o in objs:
    print ("static const struct v7_generic_object fobj_%(addr)s"
           " = {%(base_obj)s, (struct v7_object *) %(ref)s};") % dict(
        addr = o["addr"],
        base_obj = base_obj(o),
        ref = ref("fobj", o["proto"], opt=True),
    )

for f in funcs:
    print ("static const struct v7_js_function ffunc_%(addr)s"
           " = {%(base_obj)s, (struct v7_generic_object *) %(obj_ref)s,"
           " (struct bcode *) %(bcode_ref)s};") % dict(
        addr = f["addr"],
        base_obj = base_obj(f),
        obj_ref = ref("fobj", f["scope"], opt=True),
        bcode_ref = ref("fbcode", f["bcode"]),
    )

for p in props:
    value = gen_uvalue(p["value_type"], p["value"])
    if get_tag(p["name"]) not in [num(V7_TAG_STRING_O), num(V7_TAG_STRING_F)]:
        name = gen_value(p["name"])
    else:
        name = gen_fstr(p["name_str"])

    print ("static const struct v7_fr_property fprop_%(addr)s"
           " = {(struct v7_fr_property *) %(next_ref)s, %(attrs)s, %(entity_id)s,"
           " %(name)s, %(value)s}; /* %(name_str)s */") % dict(
        addr = p["addr"],
        entity_id = p["entity_id"],
        next_ref = ref("fprop", p["next"], opt=True),
        attrs = p["attrs"],
        name = name,
        value = value,
        name_str = p["name_str"],
    )

#
# emit globals, the roots of the whole object graph
#
print
print "const struct static_val fr_v7_vals[] = {"
for g in glob:
    if is_null(g["value"]):
        print "  STATIC_V7_UNDEFINED,"
    else:
        print "  {%s, 0, %s}," % (ref("fobj", g["value"]), V7_TAG_OBJECT)
print "};"

print ("V7_STATIC_ASSERT(sizeof(fr_v7_vals)"
       " == sizeof(struct v7_vals), BAD_GLOBS_LEN);")
print "struct v7_vals *fr_vals = (struct v7_vals*)&fr_v7_vals;"
