#!/usr/bin/env python
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
# Resulting configuration is an object which is represented as a C struct,
# but definition is a list of entries, where each entry represents a key
# in the resulting struct. Each item is a [path, type, default, params] array,
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
# $ gen_sys_config.py --c_name=foo_config example.yaml
# $ cat foo_config.h
#     struct foo_config_foo_bar {
# struct foo_config_foo {
#     struct foo_config_foo_bar {
#       char *name;
#       int num_quux;
#     } bar;
#     int frombulate;
#   }
# struct foo_config {
#   struct foo_config_foo foo;
# };
#
# If default value is not specified when an entry is defined, a zero-value
# is assumed: false for bo9oleans, 0 for ints and an empty string for strings.
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


class SchemaEntry(object):
    V_INT = "i"
    V_BOOL = "b"
    V_DOUBLE = "d"
    V_STRING = "s"
    V_OBJECT = "o"

    def __init__(self, e):
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
            elif self.vtype == SchemaEntry.V_DOUBLE:
                self.default = 0.0
            elif self.vtype == SchemaEntry.V_STRING:
                self.default = ""
        elif len(e) == 4:
            self.path, self.vtype, self.default, self.params = e
        else:
            raise ValueError("Invalid entry length: %s (%s)" % (len(e), e))

        if not isinstance(self.path, basestring):
            raise TypeError("Path is not a string (%s)" % e)

        if self.vtype is not None:
            if self.vtype not in (self.V_OBJECT, self.V_BOOL, self.V_INT, self.V_DOUBLE, self.V_STRING):
                raise TypeError("%s: Invalid value type '%s'" % (self.path, self.vtype))
            if self.vtype == SchemaEntry.V_DOUBLE and isinstance(self.default, int):
                self.default = float(self.default)
            self.ValidateDefault()

        if self.params is not None and not isinstance(self.params, dict):
            raise TypeError("%s: Invalid params" % self.path)

    def ValidateDefault(self):
        if (self.vtype == SchemaEntry.V_BOOL and not isinstance(self.default, bool) or
            self.vtype == SchemaEntry.V_INT and not isinstance(self.default, int) or
            self.vtype == SchemaEntry.V_DOUBLE and not isinstance(self.default, float) or
            self.vtype == SchemaEntry.V_STRING and not isinstance(self.default, basestring)):
            raise TypeError("%s: Invalid default value type (%s)" % (self.path, type(self.default)))

    @property
    def key(self):
        return self.path.split(".")[-1]

    def Assign(self, other):
        self.path = other.path
        self.vtype = other.vtype
        self.default = other.default
        self.params = other.params


class Schema(object):
    def __init__(self):
        self._schema = []

    def Merge(self, s):
        if type(s) is not list:
            raise TypeError("Schema must be a list, not %s" % (type(s)))
        for el in s:
            e = SchemaEntry(el)
            for oe in self._schema:
                if oe.path == e.path:
                    if oe.vtype == SchemaEntry.V_OBJECT:
                        raise TypeError("%s: Cannot override an object" % e.path)
                    break
            else:
                oe = None
            if e.vtype is None:
                if oe is None:
                    raise KeyError("%s: Cannot override, no such entry" % e.path)
                oe.default = e.default
                oe.ValidateDefault()
            else:
                if oe is None:
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
        else:
            writer.Value(e)

    def Walk(self, writer):
        for e in self._schema:
            if '.' in e.path:
                continue
            self._EmitEntry(e, writer)


# Writes a JSON object with defaults.
class DefaultsJSONWriter(object):
    def __init__(self):
        self._defaults = collections.OrderedDict()
        self._path = [self._defaults]

    def ObjectStart(self, e):
        d = collections.OrderedDict()
        self._path[-1][e.key] = d
        self._path.append(d)

    def Value(self, e):
        self._path[-1][e.key] = e.default

    def ObjectEnd(self, _e):
        self._path.pop()

    def __str__(self):
        return json.dumps(self._defaults, indent=2)


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

def get_ctype(vtype):
    if vtype in (SchemaEntry.V_BOOL, SchemaEntry.V_INT):
        ctype_api = "int         "
        ctype_field = "int "
    elif vtype == SchemaEntry.V_DOUBLE:
        ctype_api = "double      "
        ctype_field = "double "
    elif vtype == SchemaEntry.V_STRING:
        ctype_api = "const char *"
        ctype_field = "char *"
    return (ctype_api, ctype_field)

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
        new_obj_type = "struct %s_%s" % (self._struct_name, e.path.replace(".", "_"))
        self._fields.append("%s %s" % (new_obj_type, e.key))
        self._stack.append((self._obj_type, self._fields))
        self._obj_type, self._fields = new_obj_type, []

    def Value(self, e):
        key = e.key
        _, ctype_field = get_ctype(e.vtype)
        self._fields.append("%s%s" % (ctype_field, key))

    def ObjectEnd(self, e):
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
        self._stack = []
        self._getters = []
        self._setters = []
        self._cur_acc_prefix = ""

    def ObjectStart(self, e):
        self._stack.append(self._cur_acc_prefix)
        self._cur_acc_prefix += ".%s" % e.key
        self._getters.append(("const struct %s%s *" % (self._struct_name, self._cur_acc_prefix.replace(".", "_")), "", self._cur_acc_prefix))

    def Value(self, e):
        key = e.key
        ctype_api, ctype_field = get_ctype(e.vtype)
        cur_path = "%s.%s" % (self._cur_acc_prefix, key)
        self._getters.append((ctype_api, ctype_field, cur_path))
        self._setters.append((ctype_api, ctype_field, cur_path))

    def ObjectEnd(self, e):
        self._cur_acc_prefix = self._stack.pop()

    def GetGetterSignature(self, path, ctype):
        name = path.replace(".", "_")
        return "%s%s_get%s(struct %s *cfg)" % (ctype, self._struct_name, name, self._struct_name)
    def GetSetterSignature(self, path, ctype):
        name = path.replace(".", "_")
        return "void %s_set%s(struct %s *cfg, %sval)" % (self._struct_name, name, self._struct_name, ctype)

    # Returns array of lines to be pasted to the header.
    def GetHeaderLines(self):
        lines = []

        lines.append("/* Parametrized accessor prototypes {{{ */")
        for ctype_api, ctype_field, path in self._getters:
            lines.append("%s;" % self.GetGetterSignature(path, ctype_api))
        lines.append("")

        for ctype_api, ctype_field, path in self._setters:
            lines.append("%s;" % self.GetSetterSignature(path, ctype_api))
        lines.append("/* }}} */")
        lines.append("")

        if self._c_global_name != None:
            lines.append("extern struct %s %s;" % (self._struct_name, self._c_global_name))
            lines.append("")
            for ctype_api, ctype_field, path in self._getters:
                name = path.replace(".", "_")
                lines.append("static inline %s%s_get%s(void) { return %s_get%s(&%s); }" % (ctype_api, self._c_global_name, name, self._struct_name, name, self._c_global_name))
            lines.append("")
            for ctype_api, ctype_field, path in self._setters:
                name = path.replace(".", "_")
                lines.append("static inline void %s_set%s(%sval) { %s_set%s(&%s, val); }" % (self._c_global_name, name, ctype_api, self._struct_name, name, self._c_global_name))
            lines.append("")

        return lines

    # Returns array of lines to be pasted to the C source file.
    def GetSourceLines(self):
        lines = []

        if self._c_global_name != None:
            lines.append("/* Global instance */")
            lines.append("struct %s %s;" % (self._struct_name, self._c_global_name))
            lines.append("")

        lines.append("/* Getters {{{ */")
        for ctype_api, ctype_field, path in self._getters:
            # If we're going to return a struct, then we need to take an
            # address of the value being returned; otherwise return the value
            # itself
            ampersand = ""
            if "struct" in ctype_api:
                ampersand = "&"

            lines.append("%s {" % self.GetGetterSignature(path, ctype_api))
            lines.append("  return %scfg->%s;" % (ampersand, path[1:]))
            lines.append("}")
        lines.append("/* }}} */")
        lines.append("")

        lines.append("/* Setters {{{ */")
        for ctype_api, ctype_field, path in self._setters:
            lines.append("%s {" % self.GetSetterSignature(path, ctype_api))
            if "char" in ctype_api:
                lines.append("  mgos_conf_set_str(&cfg->%s, val);" % path[1:])
            else:
                lines.append("  cfg->%s = val;" % path[1:])
            lines.append("}")
        lines.append("/* }}} */")

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
/*
 * Generated file - do not edit.
 * Command: {cmd}
 */

#ifndef {name_uc}_H_
#define {name_uc}_H_

#include "mgos_config_util.h"

#ifdef __cplusplus
extern "C" {{
#endif /* __cplusplus */

{struct_def_lines}
{lines}

const struct mgos_conf_entry *{name}_schema();

#ifdef __cplusplus
}}
#endif /* __cplusplus */

#endif /* {name_uc}_H_ */
""".format(cmd=' '.join(sys.argv),
           name=self._struct_name,
           name_uc=self._struct_name.upper(),
           struct_def_lines="\n".join(self._struct_def_gen.GetLines()),
           lines="\n".join(self._acc_gen.GetHeaderLines()))


# Writes C source file with schema definition
class CWriter(object):
    _CONF_TYPES = {
        SchemaEntry.V_INT: "CONF_TYPE_INT",
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
            '  {.type = CONF_TYPE_OBJECT, .key = "%s", .num_desc = %d},'
            % (e.key, num_desc))

    def __str__(self):
        return """\
/* Generated file - do not edit. */

#include <stddef.h>
#include "{name}.h"

const struct mgos_conf_entry {name}_schema_[{num_entries}] = {{
  {{.type = CONF_TYPE_OBJECT, .key = "", .num_desc = {num_desc}}},
{schema_lines}
}};

const struct mgos_conf_entry *{name}_schema() {{
  return {name}_schema_;
}}

{accessor_lines}
""".format(name=self._struct_name,
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
            os.makedirs(dirname, mode=0755)
        except Exception:
            pass
    f = open(tmpname, "w")
    try:
        yield f
        f.close()
        if sys.platform == "win32":
            # This is no longer atomic, but Win does not support atomic renames.
            os.remove(name)
        os.rename(tmpname, name)
    except Exception, e:
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
            except (TypeError, ValueError, KeyError), e:
                print >>sys.stderr, ("While parsing %s: %s" % (schema_file, e))
                sys.exit(1)

    dw = DefaultsJSONWriter()
    schema.Walk(dw)
    dfn = os.path.join(args.dest_dir, "%s_defaults.json" % args.c_name)
    with open_with_temp(dfn) as df:
        df.write(str(dw))

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
