#!/usr/bin/env python3
# vim: tabstop=4 expandtab shiftwidth=4 ai cin smarttab
#
# Copyright (c) 2014-2016 Cesanta Software Limited
# All rights reserved
#
#
# This tool parses config schema definitions YAML and produces:
#  - C header and source file
#  - JSON file with defaults
#  - JSON file with schema (for consumption by the UI)
#
# Resulting configuration is an object which is represented as an opaque C
# struct, and accessors for it (so far struct is still public, but it will
# change soon). Each item is a [path, type, default, params] array,
# where path is the full path to the key, type is bool, int or string, default
# is the default value and params is a dict with various params (currently only
# used by web UI to assist rendering).
# Here's a simple example:
# [
#   ["foo", "o", {"title": "Foo settings"}],
#   ["foo.bar", "o", {"title": "The bar section"}],
#   ["foo.bar.name", "s", "Fred", {"title": "Name of the bar"}],
#   ["foo.bar.num_quux", "i", 100, {"title": "Number of quux in the bar"}],
#   ["foo.frombulate", "b", false, {"title": "Enable frombulation"}],
# ]
#
# $ gen_sys_config.py --c_name=myconfig --dest_dir /tmp/myconfig example.yaml
# $ cat foo_config.h
# ....
# const struct myconfig_foo *myconfig_get_foo(struct myconfig *cfg);
# const struct myconfig_foo_bar *myconfig_get_foo_bar(struct myconfig *cfg);
# const char *myconfig_get_foo_bar_name(struct myconfig *cfg);
# int         myconfig_get_foo_bar_num_quux(struct myconfig *cfg);
# int         myconfig_get_foo_frombulate(struct myconfig *cfg);
#
# void myconfig_set_foo_bar_name(struct myconfig *cfg, const char *val);
# void myconfig_set_foo_bar_num_quux(struct myconfig *cfg, int         val);
# void myconfig_set_foo_frombulate(struct myconfig *cfg, int         val);
# ....
#
# There is an optional flag --c_global_name=<name>; if it's given, then
# the resulting source file will contain the definition of a config instance
# with the given name, and the header will contain a bunch of accessors for
# that global instance:
#
# $ gen_sys_config.py --c_name=myconfig --c_global_name=myconfig_global --dest_dir /tmp/myconfig example.yaml
# $ cat foo_config.h
# ....
# extern struct myconfig myconfig_global;
#
# static inline const struct myconfig_foo *myconfig_global_get_foo(void) { return myconfig_get_foo(&myconfig_global); }
# static inline const struct myconfig_foo_bar *myconfig_global_get_foo_bar(void) { return myconfig_get_foo_bar(&myconfig_global); }
# static inline const char *myconfig_global_get_foo_bar_name(void) { return myconfig_get_foo_bar_name(&myconfig_global); }
# static inline int         myconfig_global_get_foo_bar_num_quux(void) { return myconfig_get_foo_bar_num_quux(&myconfig_global); }
# static inline int         myconfig_global_get_foo_frombulate(void) { return myconfig_get_foo_frombulate(&myconfig_global); }
#
# static inline void myconfig_global_set_foo_bar_name(const char *val) { myconfig_set_foo_bar_name(&myconfig_global, val); }
# static inline void myconfig_global_set_foo_bar_num_quux(int         val) { myconfig_set_foo_bar_num_quux(&myconfig_global, val); }
# static inline void myconfig_global_set_foo_frombulate(int         val) { myconfig_set_foo_frombulate(&myconfig_global, val); }
# ....
#
# If default value is not specified when an entry is defined, a zero-value
# is assumed: false for booleans, 0 for ints and an empty string for strings.
# These two definitions are equivalent:
#
# ["foo.frombulate", "b", false, {"title": "Enable frombulation"}],
# ["foo.frombulate", "b", {"title": "Enable frombulation"}],
#
# Multiple files can be specified on the command line, they will be merged
# into one schema. Entries can be overridden by subsequent files.
#
# A shorter, 2-element entry can be used to override just the default value:
# [
#   ["foo.frombulate", true],  # Enable frombulation by default.
# ]

import argparse
import collections
import contextlib
import json
import yaml
import os
import sys

parser = argparse.ArgumentParser(description="Create C config boilerplate from a YAML schema")
parser.add_argument("--c_name", required=True, help="name of the top-level C struct")
parser.add_argument("--c_global_name", required=False, help="name for the global instance, also will be used as a prefix for its accessors")
parser.add_argument("--dest_dir", default=".", help="base path of generated files")
parser.add_argument("schema_files", nargs="+", help="YAML schema files")


RESERVED_WORDS = set(
    ("alignas alignof and and_eq asm auto bitand bitor bool break case catch char "
     "char16_t char32_t class compl const constexpr const_cast continue decltype "
     "default delete do double dynamic_cast else enum explicit export extern false "
     "float for friend goto if inline int long mutable namespace new noexcept not "
     "not_eq nullptr operator or or_eq private protected public register "
     "reinterpret_cast restrict return short signed sizeof static static_assert "
     "static_cast struct switch template this thread_local throw true try typedef "
     "typeid typename union unsigned using virtual void volatile wchar_t while "
     "xor xor_eq").split())


class SchemaEntry(object):
    V_INT = "i"
    V_UNSIGNED_INT = "ui"
    V_BOOL = "b"
    V_DOUBLE = "d"
    V_STRING = "s"
    V_OBJECT = "o"

    def __init__(self, e):
        if isinstance(e, SchemaEntry):
            self.Assign(e)
            return
        if not isinstance(e, list):
            raise TypeError("Invalid entry type '%s' (%s)" % (type(e), e))
        if len(e) == 2:  # Default override.
            self.path, self.default = e
            self.vtype, self.params = None, None
        elif len(e) == 3:
            self.path, self.vtype, self.params = e
            if self.vtype == SchemaEntry.V_BOOL:
                self.default = False
            elif self.vtype == SchemaEntry.V_INT:
                self.default = 0
            elif self.vtype == SchemaEntry.V_UNSIGNED_INT:
                self.default = 0
            elif self.vtype == SchemaEntry.V_DOUBLE:
                self.default = 0.0
            elif self.vtype == SchemaEntry.V_STRING:
                self.default = ""
            else:
                self.default = None
        elif len(e) == 4:
            self.path, self.vtype, self.default, self.params = e
        else:
            raise ValueError("Invalid entry length: %s (%s)" % (len(e), e))

        if not isinstance(self.path, str):
            raise TypeError("Path is not a string (%s)" % e)

        for part in self.path.split("."):
            if part in RESERVED_WORDS:
                raise NameError("Cannot use '%s' as a config key, it's a C/C++ reserved keyword" % part)

        self.orig_path = None

        if self.vtype is not None:
            if self.vtype == SchemaEntry.V_DOUBLE and isinstance(self.default, int):
                self.default = float(self.default)
            if self.IsPrimitiveType():
                self.ValidateDefault()
            else:
                # Postpone validation, it may refer to an entry that doesn't exist yet.
                pass

        if self.params is not None and not isinstance(self.params, dict):
            raise TypeError("%s: Invalid params" % self.path)

    def IsPrimitiveType(self):
        return self.vtype in (
            self.V_OBJECT, self.V_BOOL, self.V_INT, self.V_UNSIGNED_INT,
            self.V_DOUBLE, self.V_STRING)


    def ValidateDefault(self):
        if self.vtype == SchemaEntry.V_DOUBLE and type(self.default) is int:
            self.default = float(self.default)
        if (self.vtype == SchemaEntry.V_BOOL and not isinstance(self.default, bool) or
            self.vtype == SchemaEntry.V_INT and not isinstance(self.default, int) or
            self.vtype == SchemaEntry.V_UNSIGNED_INT and not isinstance(self.default, int) or
            # In Python, boolvalue is an instance of int, but we don't want that.
            self.vtype == SchemaEntry.V_INT and isinstance(self.default, bool) or
            self.vtype == SchemaEntry.V_DOUBLE and not isinstance(self.default, float) or
            self.vtype == SchemaEntry.V_STRING and not isinstance(self.default, str)):
            raise TypeError("%s: Invalid default value type (%s)" % (self.path, type(self.default)))
        if self.vtype == SchemaEntry.V_UNSIGNED_INT and self.default < 0:
            raise TypeError("%s: Invalid default unsigned value (%d)" % (self.path, self.default))

    def GetIdentifierName(self):
        return self.path.replace(".", "_")

    def GetCType(self, struct_prefix):
        if self.vtype in (SchemaEntry.V_BOOL, SchemaEntry.V_INT):
            return "int"
        elif self.vtype == SchemaEntry.V_UNSIGNED_INT:
            return "unsigned int"
        elif self.vtype == SchemaEntry.V_DOUBLE:
            return "double"
        elif self.vtype == SchemaEntry.V_STRING:
            return "const char *"
        elif self.vtype == SchemaEntry.V_OBJECT:
            p = (self.orig_path if self.orig_path else self.path).replace(".", "_")
            return "struct %s_%s" % (struct_prefix, p)

    @property
    def key(self):
        return self.path.split(".")[-1]

    def Assign(self, other):
        self.path = other.path
        self.vtype = other.vtype
        self.default = other.default
        self.params = other.params
        self.orig_path = other.orig_path

    def __str__(self):
        return '["%s", "%s", "%s", %s]' % (self.path, self.vtype, self.default, self.params)


class Schema(object):
    def __init__(self):
        self._schema = []
        self._overrides = {}

    def _FindEntry(self, path):
        for e in self._schema:
            if e.path == path: return e
        return None

    def Merge(self, s):
        if type(s) is not list:
            raise TypeError("Schema must be a list, not %s" % (type(s)))
        for el in s:
            e = SchemaEntry(el)
            if e.vtype is None:
                self._overrides[e.path] = e.default
            else:
                oe = self._FindEntry(e.path)
                if oe is None:
                    # Construct containing objects as needed.
                    pc = e.path.split(".")
                    for l in range(1, len(pc)):
                        op = ".".join(pc[:l])
                        if not self._FindEntry(op):
                            self._schema.append(SchemaEntry([op, SchemaEntry.V_OBJECT, {}]))
                    self._schema.append(e)
                else:
                    oe.Assign(e)

    def _EmitObj(self, e, writer):
        writer.ObjectStart(e)
        prefix = e.path + "."
        for oe in self._schema:
            if not oe.path.startswith(prefix):
                continue
            suffix = oe.path[len(prefix):]
            if '.' in suffix:
                continue
            self._EmitEntry(oe, writer)
        writer.ObjectEnd(e)

    def _EmitEntry(self, e, writer):
        if e.vtype == SchemaEntry.V_OBJECT:
            self._EmitObj(e, writer)
        elif e.IsPrimitiveType():
            if e.path in self._overrides:
                e.default = self._overrides[e.path]
                e.ValidateDefault()
            writer.Value(e)
        else:
            # Reference to some other entry. Let's look it up.
            for e1 in self._schema:
                if e1.path == e.vtype:
                    break
            else:
                print("%s references a non-existent entry %s" % (e.path, e.vtype))
                sys.exit(1)

            # Now emit this entry but rewrite paths.
            prw = PathRewriter(self, e1.path, e.path, writer)
            self._EmitEntry(e1, prw)

    def Walk(self, writer):
        for e in self._schema:
            if '.' in e.path:
                continue
            self._EmitEntry(e, writer)


class PathRewriter(object):
    def __init__(self, schema, from_prefix, to_prefix, writer):
        self._schema = schema
        self._from_prefix = from_prefix
        self._to_prefix = to_prefix
        self._writer = writer

    def _RewritePath(self, e):
        ec = SchemaEntry(e)
        ec.orig_path = ec.path
        ec.path = ec.path.replace(self._from_prefix, self._to_prefix, 1)
        return ec

    def ObjectStart(self, e):
        self._writer.ObjectStart(self._RewritePath(e))

    def Value(self, e):
        e1 = self._RewritePath(e)
        if e1.path in self._schema._overrides:
            e1.default = self._schema._overrides[e1.path]
            e1.ValidateDefault()
        self._writer.Value(e1)

    def ObjectEnd(self, e):
        self._writer.ObjectEnd(self._RewritePath(e))


# Writes a JSON version of the schema.
class JSONSchemaWriter(object):
    def __init__(self):
        self._schema = []

    def ObjectStart(self, e):
        self._schema.append([e.path, SchemaEntry.V_OBJECT, e.params])

    def Value(self, e):
        self._schema.append([e.path, e.vtype, e.params])

    def ObjectEnd(self, _e):
        pass

    def __str__(self):
        # return a bit more compact representation, one entry per line.
        return "[\n%s\n]\n" % ",\n".join(
            "  %s" % json.dumps(e, sort_keys=True) for e in self._schema)

# Generates struct definitions.
# TODO(dfrank): add support for public/private structs, and then have two
# methods instead of a single GetLines() : we need some structs to be
# present in header, and some in .c file.
class StructDefGen(object):
    def __init__(self, struct_name):
        self._struct_name = struct_name
        self._obj_type = "struct %s" % struct_name
        self._objs = []
        self._fields = []
        self._stack = []

    def ObjectStart(self, e):
        new_obj_type = e.GetCType(self._struct_name)
        self._fields.append("%s %s" % (new_obj_type, e.key))
        self._stack.append((self._obj_type, self._fields))
        self._obj_type, self._fields = new_obj_type, []

    def Value(self, e):
        self._fields.append("%s %s" % (e.GetCType(self._struct_name), e.key))

    def ObjectEnd(self, e):
        if not e.orig_path:
            self._objs.append((len(self._stack), self._obj_type, self._fields))
        self._obj_type, self._fields = self._stack.pop()

    def GetLines(self):
        self._objs.append((len(self._stack), self._obj_type, self._fields))
        lines = []
        for _, obj_type, fields in self._objs:
            lines.append("%s {" % obj_type)
            for f in fields:
                lines.append("  %s;" % f)
            lines.append("};")
            lines.append("")

        return lines

# Generators of accessors, for both header and source files. If `c_global_name`
# is not None, then header will additionally contain static inline functions
# to access a global config instance, allocated globally in the source.
class AccessorsGen(object):
    def __init__(self, struct_name, c_global_name):
        self._struct_name = struct_name
        self._c_global_name = c_global_name
        self._entries = []
        self._getters = []
        self._setters = []

    def ObjectStart(self, e):
        self._entries.append(e)

    def Value(self, e):
        self._entries.append(e)

    def ObjectEnd(self, e):
        pass

    def _GetterSignature(self, e, ctype, const):
        return "%s%s %s_get_%s(struct %s *cfg)" % (
                const, ctype, self._struct_name, e.GetIdentifierName(), self._struct_name)

    def _SetterSignature(self, e, ctype, const):
        return "void %s_set_%s(struct %s *cfg, %s%s v)" % (
                self._struct_name, e.GetIdentifierName(), self._struct_name, const, ctype)

    def _GetCopierSignature(self, e, ctype):
        return "bool %s_copy_%s(const %ssrc, %sdst)" % (
                self._struct_name, e.GetIdentifierName(), ctype, ctype)

    def _CopySignature(self, e, ctype):
        return "bool %s_copy_%s(const %ssrc, %sdst)" % (
                self._struct_name, e.GetIdentifierName(), ctype, ctype)

    def _FreeSignature(self, e, ctype):
        return "void %s_free_%s(%scfg)" % (self._struct_name, e.GetIdentifierName(), ctype)

    def _SchemaSignature(self, e):
        return "const struct mgos_conf_entry *%s_schema_%s(void)" % (self._struct_name, e.GetIdentifierName())

    def _ParseSignature(self, e, ctype):
        return "bool %s_parse_%s(struct mg_str json, %scfg)" % (self._struct_name, e.GetIdentifierName(), ctype)

    # Returns array of lines to be pasted to the header.
    def GetHeaderLines(self):
        lines = []

        if self._c_global_name:
            lines.append("extern struct %s %s;" % (self._struct_name, self._c_global_name))
            lines.append("extern const struct %s %s_defaults;" % (self._struct_name, self._struct_name))

        for e in self._entries:
            iname = e.GetIdentifierName()
            lines.append("")
            lines.append("/* %s */" % e.path)
            lines.append("#define %s_HAVE_%s" % (self._struct_name.upper(), iname.upper()))
            if self._c_global_name:
                lines.append("#define %s_HAVE_%s" % (self._c_global_name.upper(), iname.upper()))
            ctype = e.GetCType(self._struct_name)
            if e.vtype == SchemaEntry.V_OBJECT:
                getter, setter, copy, free, const = True, False, True, True, "const "
                ctype += " *"
            elif e.vtype == SchemaEntry.V_STRING:
                getter, setter, copy, free, const = True, True, False, False, ""
            else:
                getter, setter, copy, free, const = True, True, False, False, ""

            if getter:
                lines.append("%s;" % self._GetterSignature(e, ctype, const))
                if self._c_global_name:
                    lines.append("static inline %s%s %s_get_%s(void) { return %s_get_%s(&%s); }" % (
                        const, ctype, self._c_global_name, iname, self._struct_name, iname, self._c_global_name))
            if setter:
                lines.append("%s;" % self._SetterSignature(e, ctype, const))
                if self._c_global_name:
                    lines.append("static inline void %s_set_%s(%s%s v) { %s_set_%s(&%s, v); }" % (
                        self._c_global_name, iname, const, ctype, self._struct_name, iname, self._c_global_name))

            if e.vtype == SchemaEntry.V_OBJECT:
                lines.append("%s;" % self._SchemaSignature(e))
                lines.append("%s;" % self._ParseSignature(e, ctype))
                lines.append("%s;" % self._CopySignature(e, ctype))
                lines.append("%s;" % self._FreeSignature(e, ctype))

        if self._c_global_name:
            lines.append("")
            lines.append("bool %s_get(const struct mg_str key, struct mg_str *value);" % self._c_global_name)
            lines.append("bool %s_set(const struct mg_str key, const struct mg_str value, bool free_strings);" % self._c_global_name)

        return lines

    @staticmethod
    def EscapeCString(s):
        # JSON encoder will provide acceptable escaping.
        s = json.dumps(s)
        # Get rid of trigraph sequences.
        for a, b in ((r"??(", r"?\?("), (r"??)", r"?\?)"), (r"??<", r"?\?<"), (r"??>", r"?\?>"),
                     (r"??=", r"?\?="), (r"??/", r"?\?/"), (r"??'", r"?\?'"), (r"??!", r"?\?-")):
            if a in s:
                s = s.replace(a, b)
        return s

    # Returns array of lines to be pasted to the C source file.
    def GetSourceLines(self):
        lines = []

        if self._c_global_name:
            lines.append("/* Global instance */")
            lines.append("struct %s %s;" % (self._struct_name, self._c_global_name))
            lines.append("const struct %s %s_defaults = {" % (self._struct_name, self._struct_name))
            for e in self._entries:
                if e.vtype == SchemaEntry.V_OBJECT:
                    pass
                elif e.vtype == SchemaEntry.V_STRING:
                    if e.default:
                        lines.append("  .%s = %s," % (e.path, self.EscapeCString(e.default)))
                    else:
                        lines.append("  .%s = NULL," % e.path)
                elif e.vtype == SchemaEntry.V_BOOL:
                    lines.append("  .%s = %d," % (e.path, (e.default and 1 or 0)))
                else:
                    lines.append("  .%s = %s," % (e.path, e.default))
            lines.append("};")

        for e in self._entries:
            iname = e.GetIdentifierName()
            lines.append("")
            lines.append("/* %s */" % e.path)
            lines.append("#define %s_HAVE_%s" % (self._struct_name.upper(), iname.upper()))
            if self._c_global_name:
                lines.append("#define %s_HAVE_%s" % (self._c_global_name.upper(), iname.upper()))
            ctype = e.GetCType(self._struct_name)
            amp = ""
            if e.vtype == SchemaEntry.V_OBJECT:
                getter, setter, copy, free, const = True, False, True, True, "const "
                ctype += " *"
                amp = "&"
            elif e.vtype == SchemaEntry.V_STRING:
                getter, setter, copy, free, const = True, True, False, False, ""
            else:
                getter, setter, copy, free, const = True, True, False, False, ""

            if getter:
                lines.append("%s {" % self._GetterSignature(e, ctype, const))
                lines.append("  return %scfg->%s;" % (amp, e.path))
                lines.append("}")
            if setter:
                lines.append("%s {" % self._SetterSignature(e, ctype, const))
                if e.vtype == SchemaEntry.V_STRING:
                    lines.append("  mgos_conf_set_str(&cfg->%s, v);" % e.path)
                else:
                    lines.append("  %scfg->%s = v;" % (amp, e.path))
                lines.append("}")

            if e.vtype == SchemaEntry.V_OBJECT:
                lines.append("%s {" % self._SchemaSignature(e))
                lines.append('  return mgos_conf_find_schema_entry("%s", %s_schema());' % (e.path, self._struct_name))
                lines.append("}")
                lines.append("%s {" % self._ParseSignature(e, ctype))
                lines.append('  return mgos_conf_parse_sub(json, %s_schema(), cfg);' % (self._struct_name))
                lines.append("}")
                lines.append("%s {" % self._CopySignature(e, ctype))
                lines.append("  return mgos_conf_copy(%s_schema_%s(), src, dst);" % (self._struct_name, e.GetIdentifierName()))
                lines.append("}")
                lines.append("%s {" % self._FreeSignature(e, ctype))
                lines.append("  return mgos_conf_free(%s_schema_%s(), cfg);" % (self._struct_name, e.GetIdentifierName()))
                lines.append("}")

        if self._c_global_name:
            lines.append("bool %s_get(const struct mg_str key, struct mg_str *value) {" % self._c_global_name)
            lines.append("  return mgos_config_get(key, value, &%s, %s_schema());" % (self._c_global_name, self._struct_name))
            lines.append("}")
            lines.append("bool %s_set(const struct mg_str key, const struct mg_str value, bool free_strings) {" % self._c_global_name,)
            lines.append("  return mgos_config_set(key, value, &%s, %s_schema(), free_strings);" % (self._c_global_name, self._struct_name))
            lines.append("}")

        return lines


# Writes C header file.
class HWriter(object):
    def __init__(self, struct_name, c_global_name):
        self._acc_gen = AccessorsGen(struct_name, c_global_name)
        self._struct_def_gen = StructDefGen(struct_name)
        self._struct_name = struct_name

    def ObjectStart(self, e):
        self._acc_gen.ObjectStart(e)
        self._struct_def_gen.ObjectStart(e)

    def Value(self, e):
        self._acc_gen.Value(e)
        self._struct_def_gen.Value(e)

    def ObjectEnd(self, e):
        self._acc_gen.ObjectEnd(e)
        self._struct_def_gen.ObjectEnd(e)

    def __str__(self):
        return """\
/* clang-format off */
/*
 * Generated file - do not edit.
 * Command: {cmd}
 */

#pragma once

#include <stdbool.h>
#include "common/mg_str.h"

#ifdef __cplusplus
extern "C" {{
#endif

{struct_def_lines}

const struct mgos_conf_entry *{name}_schema();

{lines}

#ifdef __cplusplus
}}
#endif
""".format(cmd=' '.join(sys.argv),
           name=self._struct_name,
           struct_def_lines="\n".join(self._struct_def_gen.GetLines()),
           lines="\n".join(self._acc_gen.GetHeaderLines()))


# Writes C source file with schema definition
class CWriter(object):
    _CONF_TYPES = {
        SchemaEntry.V_INT: "CONF_TYPE_INT",
        SchemaEntry.V_UNSIGNED_INT: "CONF_TYPE_UNSIGNED_INT",
        SchemaEntry.V_BOOL: "CONF_TYPE_BOOL",
        SchemaEntry.V_DOUBLE: "CONF_TYPE_DOUBLE",
        SchemaEntry.V_STRING: "CONF_TYPE_STRING",
    }

    def __init__(self, struct_name, c_global_name):
        self._acc_gen = AccessorsGen(struct_name, c_global_name)
        self._struct_name = struct_name
        self._schema_lines = []
        self._start_indices = []

    def ObjectStart(self, _e):
        self._acc_gen.ObjectStart(_e)
        self._start_indices.append(len(self._schema_lines))
        self._schema_lines.append(None)  # Placeholder

    def Value(self, e):
        self._acc_gen.Value(e)
        self._schema_lines.append(
            '  {.type = %s, .key = "%s", .offset = offsetof(struct %s, %s)},'
            % (self._CONF_TYPES[e.vtype], e.key, self._struct_name, e.path))

    def ObjectEnd(self, e):
        self._acc_gen.ObjectEnd(e)
        si = self._start_indices.pop()
        num_desc = len(self._schema_lines) - si - 1
        self._schema_lines[si] = (
            '  {.type = CONF_TYPE_OBJECT, .key = "%s", .offset = offsetof(struct %s, %s), .num_desc = %d},'
            % (e.key, self._struct_name, e.path, num_desc))

    def __str__(self):
        return """\
/* clang-format off */
/*
 * Generated file - do not edit.
 * Command: {cmd}
 */

#include "{name}.h"

#include <stddef.h>

#include "mgos_config_util.h"

const struct mgos_conf_entry {name}_schema_[{num_entries}] = {{
  {{.type = CONF_TYPE_OBJECT, .key = "", .offset = 0, .num_desc = {num_desc}}},
{schema_lines}
}};

const struct mgos_conf_entry *{name}_schema() {{
  return {name}_schema_;
}}

{accessor_lines}
""".format(cmd=' '.join(sys.argv),
           name=self._struct_name,
           num_entries=len(self._schema_lines) + 1,
           num_desc=len(self._schema_lines),
           schema_lines="\n".join(self._schema_lines),
           accessor_lines="\n".join(self._acc_gen.GetSourceLines()))


@contextlib. contextmanager
def open_with_temp(name):
    """Perform a write-and-rename maneuver to write a file safely."""
    tmpname = "%s.%d.tmp" % (name, os.getpid())
    dirname = os.path.dirname(name)
    if dirname:
        try:
            os.makedirs(dirname, mode=0o755)
        except Exception:
            pass
    f = open(tmpname, "w")
    try:
        yield f
        f.close()
        if sys.platform == "win32":
            # This is no longer atomic, but Win does not support atomic renames.
            if os.path.exists(name):
                os.remove(name)
        os.rename(tmpname, name)
    except Exception as e:
        os.remove(tmpname)
        raise


if __name__ == "__main__":
    args = parser.parse_args()
    schema = Schema()
    for schema_file in args.schema_files:
        with open(schema_file) as sf:
            s = yaml.load(sf)
            try:
                schema.Merge(s)
            except (TypeError, ValueError, KeyError) as e:
                print("While parsing %s: %s" % (schema_file, e), file=sys.stderr)
                sys.exit(1)

    jsw = JSONSchemaWriter()
    schema.Walk(jsw)
    jsfn = os.path.join(args.dest_dir, "%s_schema.json" % args.c_name)
    with open_with_temp(jsfn) as jsf:
        jsf.write(str(jsw))

    hw = HWriter(args.c_name, args.c_global_name)
    schema.Walk(hw)
    hfn = os.path.join(args.dest_dir, "%s.h" % args.c_name)
    with open_with_temp(hfn) as hf:
        hf.write(str(hw))

    cw = CWriter(args.c_name, args.c_global_name)
    schema.Walk(cw)
    cfn = os.path.join(args.dest_dir, "%s.c" % args.c_name)
    with open_with_temp(cfn) as cf:
        cf.write(str(cw))
