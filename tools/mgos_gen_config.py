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


def EscapeCString(s):
    # JSON encoder will provide acceptable escaping.
    s = json.dumps(s)
    # Get rid of trigraph sequences.
    for a, b in ((r"??(", r"?\?("), (r"??)", r"?\?)"), (r"??<", r"?\?<"), (r"??>", r"?\?>"),
                 (r"??=", r"?\?="), (r"??/", r"?\?/"), (r"??'", r"?\?'"), (r"??!", r"?\?-")):
        if a in s:
            s = s.replace(a, b)
    return s


class SchemaEntry:
    V_INT = "i"
    V_UNSIGNED_INT = "ui"
    V_BOOL = "b"
    V_FLOAT = "f"
    V_DOUBLE = "d"
    V_STRING = "s"
    V_OBJECT = "o"

    _CONF_TYPES = {
        V_INT: "CONF_TYPE_INT",
        V_UNSIGNED_INT: "CONF_TYPE_UNSIGNED_INT",
        V_BOOL: "CONF_TYPE_BOOL",
        V_FLOAT: "CONF_TYPE_FLOAT",
        V_DOUBLE: "CONF_TYPE_DOUBLE",
        V_STRING: "CONF_TYPE_STRING",
        V_OBJECT: "CONF_TYPE_OBJECT",
    }

    def __init__(self, e, schema=None):
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
            elif self.vtype in (SchemaEntry.V_FLOAT, SchemaEntry.V_DOUBLE):
                self.default = 0.0
            elif self.vtype == SchemaEntry.V_STRING:
                self.default = ""
            else:
                self.default = None
        elif len(e) == 4:
            self.path, self.vtype, self.default, self.params = e
        else:
            raise ValueError("Invalid entry length: %d (entry: %s)" % (len(e), e))

        if not isinstance(self.path, str):
            raise TypeError("Path must be a string, not %s (entry: %s)" % (type(self.path), e))

        for part in self.path.split("."):
            if part in RESERVED_WORDS:
                raise NameError("Cannot use '%s' as a config key, it's a C/C++ reserved keyword" % part)

        if self.params is not None and not isinstance(self.params, dict):
            raise TypeError("Params must be an object, not %s (entry: %s)" % (type(self.params), e))

        self.orig_path = None

        if self.vtype is not None:
            if self.vtype in (SchemaEntry.V_FLOAT, SchemaEntry.V_DOUBLE) and isinstance(self.default, int):
                self.default = float(self.default)
            if self.IsPrimitiveType():
                self.ValidateDefault()
            else:
                # Postpone validation, it may refer to an entry that doesn't exist yet.
                pass

        if self.params is not None and not isinstance(self.params, dict):
            raise TypeError("%s: Invalid params" % self.path)

        if self.params is not None:
            if "abstract" in self.params:
                if type(self.params["abstract"]) is not bool:
                    raise TypeError("%s: Abstract parameter must be boolean" % self.path)
                if "." in self.path or self.vtype != SchemaEntry.V_OBJECT:
                    raise ValueError("%s: Only top-level objects can be abstract" % self.path)

        self.schema = schema

    def IsPrimitiveType(self):
        return self.vtype in (
            self.V_OBJECT, self.V_BOOL, self.V_INT, self.V_UNSIGNED_INT,
            self.V_FLOAT, self.V_DOUBLE, self.V_STRING)


    def ValidateDefault(self):
        if (self.vtype in (SchemaEntry.V_FLOAT, SchemaEntry.V_DOUBLE) and
            type(self.default) in (int, str)):
            try:
                self.default = float(self.default)
            except ValueError:
                raise TypeError("%s: Invalid default value '%s'" % (self.path, self.default))
        if (self.vtype == SchemaEntry.V_BOOL and not isinstance(self.default, bool) or
            self.vtype == SchemaEntry.V_INT and not isinstance(self.default, int) or
            self.vtype == SchemaEntry.V_UNSIGNED_INT and not isinstance(self.default, int) or
            # In Python, boolvalue is an instance of int, but we don't want that.
            self.vtype == SchemaEntry.V_INT and isinstance(self.default, bool) or
            self.vtype in (SchemaEntry.V_FLOAT, SchemaEntry.V_DOUBLE) and not isinstance(self.default, float) or
            self.vtype == SchemaEntry.V_STRING and not isinstance(self.default, str)):
            raise TypeError("%s: Invalid default value type '%s'" % (self.path, type(self.default)))
        if self.vtype == SchemaEntry.V_UNSIGNED_INT and self.default < 0:
            raise TypeError("%s: Invalid default unsigned value '%d'" % (self.path, self.default))

    def GetIdentifierName(self):
        return self.path.replace(".", "_")

    def GetCType(self, struct_prefix):
        if self.vtype in (SchemaEntry.V_BOOL, SchemaEntry.V_INT):
            return "int"
        elif self.vtype == SchemaEntry.V_UNSIGNED_INT:
            return "unsigned int"
        elif self.vtype == SchemaEntry.V_FLOAT:
            return "float"
        elif self.vtype == SchemaEntry.V_DOUBLE:
            return "double"
        elif self.vtype == SchemaEntry.V_STRING:
            return "const char *"
        elif self.vtype == SchemaEntry.V_OBJECT:
            if not self.orig_path:
                return "struct %s_%s" % (struct_prefix, self.GetIdentifierName())
            return self.schema.FindEntry(self.orig_path).GetCType(struct_prefix)
        else:
            return self.schema.FindEntry(self.vtype).GetCType(struct_prefix)

    def GetCDefaultValue(self):
        if self.vtype == SchemaEntry.V_OBJECT:
            raise TypeError("shouldn't happen")
        elif self.vtype == SchemaEntry.V_BOOL:
            return "true" if self.default else "false"
        elif self.vtype == SchemaEntry.V_STRING:
            return EscapeCString(self.default) if self.default else "NULL"
        else:
            return self.default

    def GetParam(self, param):
        if self.params is None:
            return None
        return self.params.get(param)

    def IsAbstract(self):
        if "." in self.path:
            return self.schema.FindEntry(self.path.split(".")[0]).IsAbstract()
        return self.GetParam("abstract")

    @property
    def key(self):
        return self.path.split(".")[-1]

    @property
    def obj_path(self):
        return ".".join(self.path.split(".")[:-1])

    def Assign(self, other):
        self.path = other.path
        self.vtype = other.vtype
        self.default = other.default
        self.params = other.params
        self.orig_path = other.orig_path
        self.schema = other.schema

    def __str__(self):
        return '{["%s", "%s", "%s", %s], %s}' % (
                self.path, self.vtype, self.default, self.params, self.IsAbstract())


class Schema:
    def __init__(self):
        self._schema = []
        self._overrides = {}

    def FindEntry(self, path):
        for e in self._schema:
            if e.path == path: return e
        return None

    def Merge(self, s):
        if type(s) is not list:
            raise TypeError("Schema must be a list, not %s" % (type(s)))
        for el in s:
            e = SchemaEntry(el, schema=self)
            if e.vtype is None:
                self._overrides[e.path] = e.default
            else:
                oe = self.FindEntry(e.path)
                if oe is None:
                    # Construct containing objects as needed.
                    pc = e.path.split(".")
                    for l in range(1, len(pc)):
                        op = ".".join(pc[:l])
                        if not self.FindEntry(op):
                            ne = SchemaEntry([op, SchemaEntry.V_OBJECT, {}], schema=self)
                            self._schema.append(ne)
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

            # Construct a new entry with params from this one.
            e2 = SchemaEntry([e1.path, e1.vtype, e1.default, e.params], schema=e.schema)
            prw = PathRewriter(self, e1, e, writer)
            self._EmitEntry(e2, prw)

    def Walk(self, writer):
        for e in self._schema:
            if '.' in e.path:
                continue
            self._EmitEntry(e, writer)


class PathRewriter:
    def __init__(self, schema, from_e, to_e, writer):
        self._schema = schema
        self._from_e = from_e
        self._to_e = to_e
        self._writer = writer

    def _RewritePath(self, e):
        ec = SchemaEntry(e)
        ec.orig_path = ec.path
        ec.path = ec.path.replace(self._from_e.path, self._to_e.path, 1)
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
class JSONSchemaWriter:
    def __init__(self):
        self._schema = []

    def ObjectStart(self, e):
        if not e.IsAbstract():
            self._schema.append([e.path, SchemaEntry.V_OBJECT, e.params])

    def Value(self, e):
        if not e.IsAbstract():
            self._schema.append([e.path, e.vtype, e.params])

    def ObjectEnd(self, _e):
        pass

    def __str__(self):
        # return a bit more compact representation, one entry per line.
        return "[\n%s\n]\n" % ",\n".join(
            "  %s" % json.dumps(e, sort_keys=True) for e in self._schema)


class Gen:
    def ObjectStart(self, e):
        pass

    def Value(self, e):
        pass

    def ObjectEnd(self, e):
        pass

    def GetHeaderLines(self):
        return []

    def GetSourceLines(self):
        return []


class StructDefGen(Gen):
    def __init__(self, struct_name):
        self._struct_name = struct_name
        self._obj_type = "struct %s" % struct_name
        self._objs = []
        self._fields = []
        self._stack = []

    def ObjectStart(self, e):
        new_obj_type = e.GetCType(self._struct_name)
        self._fields.append(e)
        self._stack.append((self._obj_type, self._fields))
        self._obj_type, self._fields = new_obj_type, []

    def Value(self, e):
        self._fields.append(e)

    def ObjectEnd(self, e):
        self._objs.append((e, len(self._stack), self._obj_type, self._fields))
        self._obj_type, self._fields = self._stack.pop()

    def _GetSchemaDecl(self, e):
        ident_suff = "_" + e.GetIdentifierName() if e else ""
        return "const struct mgos_conf_entry *%s%s_get_schema(void)" % (self._struct_name, ident_suff)

    def _Finalize(self):
        if not self._obj_type:
            return
        self._objs.append((None, len(self._stack), self._obj_type, self._fields))
        self._obj_type = None

    def GetHeaderLines(self):
        self._Finalize()

        lines = []
        for oe, _, obj_type, fields in self._objs:
            lines.append("")
            obj_path = oe.path if oe else "<root>"
            lines.append("/* %s type %s */" % (obj_path, obj_type))
            if not oe or not oe.orig_path:
                lines.append("%s {" % obj_type)
                for fe in fields:
                    if not oe and fe.IsAbstract():
                        continue
                    field_type = fe.GetCType(self._struct_name)
                    lines.append("  %s %s;" % (field_type, fe.key))
                lines.append("};")

            ident_suff = "_" + oe.GetIdentifierName() if oe else ""
            ctype = (oe.GetCType(self._struct_name) if oe else "struct %s" % self._struct_name)

            lines.append("%s;" % self._GetSchemaDecl(oe))
            lines.append("void %s%s_set_defaults(%s *cfg);" % (self._struct_name, ident_suff, ctype))
            lines.append("static inline bool %s%s_parse(struct mg_str json, %s *cfg) {" % (self._struct_name, ident_suff, ctype))
            lines.append("  %s%s_set_defaults(cfg);" % (self._struct_name, ident_suff))
            lines.append("  return mgos_conf_parse_sub(json, %s%s_get_schema(), cfg);" % (self._struct_name, ident_suff))
            lines.append("}")
            lines.append("bool %s%s_parse_f(const char *fname, %s *cfg);" % (self._struct_name, ident_suff, ctype))
            lines.append("static inline bool %s%s_emit(const %s *cfg, bool pretty, struct json_out *out) {" % (
                self._struct_name, ident_suff, ctype))
            lines.append("  return mgos_conf_emit_json_out(cfg, NULL, %s%s_get_schema(), pretty, out);" % (self._struct_name, ident_suff))
            lines.append("}")
            lines.append("static inline bool %s%s_emit_f(const %s *cfg, bool pretty, const char *fname) {" % (
                self._struct_name, ident_suff, ctype))
            lines.append("  return mgos_conf_emit_f(cfg, NULL, %s%s_get_schema(), pretty, fname);" % (self._struct_name, ident_suff))
            lines.append("}")
            lines.append("static inline bool %s%s_copy(const %s *src, %s *dst) {" % (self._struct_name, ident_suff, ctype, ctype))
            lines.append("  return mgos_conf_copy(%s%s_get_schema(), src, dst);" % (self._struct_name, ident_suff))
            lines.append("}")
            lines.append("static inline void %s%s_free(%s *cfg) {" % (self._struct_name, ident_suff, ctype))
            lines.append("  return mgos_conf_free(%s%s_get_schema(), cfg);" % (self._struct_name, ident_suff))
            lines.append("}")

        return lines

    def _GetSchemaLines(self, top_path, top_ctype, obj_path, obj_key):
        lines = []
        embedded_ctypes = {}
        for oe, _, obj_type, fields in reversed(self._objs):
            for fe in fields:
                if fe.obj_path != obj_path:
                    continue
                if not oe and fe.IsAbstract():
                    continue
                field_type = fe.GetCType(self._struct_name)
                field_top_path = fe.path[len(top_path)+1:] if top_path else fe.path
                if fe.vtype == SchemaEntry.V_OBJECT:
                    embedded_ctypes[field_type] = len(lines) + 1
                    ll, ect = self._GetSchemaLines(top_path, top_ctype, fe.path, fe.key)
                    for k, v in ect.items():
                        embedded_ctypes[k] = v + len(lines) + 1
                    lines.extend(ll)
                else:
                    lines.append(
                        '    {.type = %s, .key = "%s", .offset = offsetof(%s, %s)},'
                        % (SchemaEntry._CONF_TYPES[fe.vtype], fe.key, top_ctype, field_top_path))
        if obj_path != top_path:
            if len(top_path) > 0:
                sub_path = obj_path[len(top_path)+1:]
            else:
                sub_path = obj_path
            obj_offset = "offsetof(%s, %s)" % (top_ctype, sub_path)
        else:
            obj_key = ""
            obj_offset = "0"
        hdr = '    {.type = %s, .key = "%s", .offset = %s, .num_desc = %d},' % (
                SchemaEntry._CONF_TYPES[SchemaEntry.V_OBJECT], obj_key, obj_offset, len(lines))
        return [hdr] + lines, embedded_ctypes

    def GetSourceLines(self):
        self._Finalize()

        lines = []
        schema_by_ctype = {}
        for oe, _, obj_type, fields in reversed(self._objs):
            if oe and oe.orig_path:
                continue
            if obj_type in schema_by_ctype:
                continue
            lines.append("")
            lines.append("/* %s */" % obj_type)
            var_name = "%s%s_schema_" % (self._struct_name, "_" + oe.GetIdentifierName() if oe else "")
            lines.append("static const struct mgos_conf_entry %s[] = {" % var_name)
            oe_path = oe.path if oe else ""
            oe_key = oe.key if oe else ""
            ll, ect = self._GetSchemaLines(oe_path, obj_type, oe_path, oe_key)
            lines.extend(ll)
            schema_by_ctype[obj_type] = (var_name, 0)
            for ctype, index in ect.items():
                schema_by_ctype[ctype] = (var_name, index)
            lines.append("};")

        for oe, _, obj_type, fields in self._objs:
            lines.append("")
            lines.append("/* %s */" % obj_type)
            lines.append("%s {" % self._GetSchemaDecl(oe))
            lines.append("  return &%s[%d];" % (schema_by_ctype[obj_type]))
            lines.append("}")
            lines.append("")
            ident_suff = "_" + oe.GetIdentifierName() if oe else ""
            ctype = oe.GetCType(self._struct_name) if oe else "struct %s" % self._struct_name
            lines.append("void %s%s_set_defaults(%s *cfg) {" % (self._struct_name, ident_suff, ctype))
            for fe in fields:
                field_type = fe.GetCType(self._struct_name)
                if fe.vtype == SchemaEntry.V_OBJECT:
                    if not oe and fe.IsAbstract():
                        continue
                    lines.append("  %s_%s_set_defaults(&cfg->%s);" % (
                        self._struct_name, fe.GetIdentifierName(), fe.key))
                else:
                    lines.append("  cfg->%s = %s;" % (fe.key, fe.GetCDefaultValue()))
            if len(fields) == 0:
                lines.append("  (void) cfg;")
            lines.append("}")
            lines.append("bool %s%s_parse_f(const char *fname, %s *cfg) {" % (self._struct_name, ident_suff, ctype))
            lines.append("  size_t len;")
            lines.append("  char *data = cs_read_file(fname, &len);")
            lines.append("  if (data == NULL) return false;")
            lines.append("  bool res = %s%s_parse(mg_mk_str_n(data, len), cfg);" % (self._struct_name, ident_suff))
            lines.append("  free(data);")
            lines.append("  return res;")
            lines.append("}")
        return lines


# Generators of accessors, for both header and source files. If `c_global_name`
# is not None, then header will additionally contain static inline functions
# to access a global config instance, allocated globally in the source.
class AccessorsGen(Gen):
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

    # Returns array of lines to be pasted to the header.
    def GetHeaderLines(self):
        lines = []

        if self._c_global_name:
            lines.append("extern struct %s %s;" % (self._struct_name, self._c_global_name))

        for e in self._entries:
            iname = e.GetIdentifierName()
            if not e.IsAbstract():
                lines.append("")
                lines.append("/* %s */" % e.path)
                lines.append("#define %s_HAVE_%s" % (self._struct_name.upper(), iname.upper()))
                if self._c_global_name:
                    lines.append("#define %s_HAVE_%s" % (self._c_global_name.upper(), iname.upper()))
            ctype = e.GetCType(self._struct_name) + " "
            if e.vtype == SchemaEntry.V_OBJECT:
                getter, setter, const = True, False, "const "
                ctype += "*"
            else:
                getter, setter, const = True, True, ""

            # Note: Getters and setters should not be inlined, it's important for compatibility with binary-only libraries.
            if getter and not e.IsAbstract():
                lines.append("%s%s%s_get_%s(const struct %s *cfg);" % (
                    const, ctype, self._struct_name, e.GetIdentifierName(), self._struct_name))
                if e.vtype != SchemaEntry.V_OBJECT:
                    lines.append("%s%s%s_get_default_%s(void);" % (
                        const, ctype, self._struct_name, e.GetIdentifierName()));
                if self._c_global_name:
                    lines.append("static inline %s%s%s_get_%s(void) { return %s_get_%s(&%s); }" % (
                        const, ctype, self._c_global_name, iname, self._struct_name, iname, self._c_global_name))
                    if e.vtype != SchemaEntry.V_OBJECT:
                        lines.append("static inline %s%s%s_get_default_%s(void) { return %s_get_default_%s(); }" % (
                            const, ctype, self._c_global_name, iname, self._struct_name, iname))

            if setter and not e.IsAbstract():
                lines.append("void %s_set_%s(struct %s *cfg, %s%sv);" % (
                    self._struct_name, e.GetIdentifierName(), self._struct_name, const, ctype))
                if self._c_global_name:
                    lines.append("static inline void %s_set_%s(%s%sv) { %s_set_%s(&%s, v); }" % (
                        self._c_global_name, iname, const, ctype, self._struct_name, iname, self._c_global_name))

        if self._c_global_name:
            lines.append("")
            lines.append("bool %s_get(const struct mg_str key, struct mg_str *value);" % self._c_global_name)
            lines.append("bool %s_set(const struct mg_str key, const struct mg_str value, bool free_strings);" % self._c_global_name)

        return lines

    # Returns array of lines to be pasted to the C source file.
    def GetSourceLines(self):
        lines = []

        if self._c_global_name:
            lines.append("/* Global instance */")
            lines.append("struct %s %s;" % (self._struct_name, self._c_global_name))

        lines.append("")
        lines.append("/* Accessors */")
        for e in self._entries:

            if not e.IsAbstract():
                lines.append("")
                lines.append("/* %s */" % e.path)

            ctype = e.GetCType(self._struct_name) + " "
            amp = ""
            if e.vtype == SchemaEntry.V_OBJECT:
                getter, setter, const = True, False, "const "
                ctype += "*"
                amp = "&"
            else:
                getter, setter, const = True, True, ""

            if getter and not e.IsAbstract():
                lines.append("%s%s%s_get_%s(const struct %s *cfg) { return %scfg->%s; }" % (
                    const, ctype, self._struct_name, e.GetIdentifierName(), self._struct_name, amp, e.path))
                if e.vtype != SchemaEntry.V_OBJECT:
                    lines.append("%s%s%s_get_default_%s(void) { return %s; }" % (
                        const, ctype, self._struct_name, e.GetIdentifierName(), e.GetCDefaultValue()));

            if setter and not e.IsAbstract():
                if e.vtype == SchemaEntry.V_STRING:
                    body = "mgos_conf_set_str(&cfg->%s, v)" % e.path
                else:
                    body = "%scfg->%s = v" % (amp, e.path)
                lines.append("void %s_set_%s(struct %s *cfg, %s%sv) { %s; }" % (
                    self._struct_name, e.GetIdentifierName(), self._struct_name, const, ctype, body))

        if self._c_global_name:
            lines.append("bool %s_get(const struct mg_str key, struct mg_str *value) {" % self._c_global_name)
            lines.append("  return mgos_config_get(key, value, &%s, %s_schema());" % (self._c_global_name, self._struct_name))
            lines.append("}")
            lines.append("bool %s_set(const struct mg_str key, const struct mg_str value, bool free_strings) {" % self._c_global_name,)
            lines.append("  return mgos_config_set(key, value, &%s, %s_schema(), free_strings);" % (self._c_global_name, self._struct_name))
            lines.append("}")

        return lines


class StringTableGen(Gen):
    def __init__(self, struct_name):
        self._struct_name = struct_name
        self._str_table = set()

    def Value(self, e):
        if e.vtype == SchemaEntry.V_STRING and e.default:
            self._str_table.add(e.default)

    def GetHeaderLines(self):
        return ["bool %s_is_default_str(const char *s);" % self._struct_name]

    def GetSourceLines(self):
        stn = "%s_str_table" % self._struct_name
        lines = ["static const char *%s[] = {" % stn]
        for s in sorted(self._str_table):
            lines.append("  %s," % EscapeCString(s))
        lines.append("""\
}};

bool {name}_is_default_str(const char *s) {{
  int num_str = (sizeof({stn}) / sizeof({stn}[0]));
  for (int i = 0; i < num_str; i++) {{
    if ({stn}[i] == s) return true;
  }}
  return false;
}}""".format(name=self._struct_name, stn=stn))
        return lines


# Writes C header file.
class HWriter:
    def __init__(self, struct_name, c_global_name):
        self._acc_gen = AccessorsGen(struct_name, c_global_name)
        self._struct_def_gen = StructDefGen(struct_name)
        self._str_table_gen = StringTableGen(struct_name)
        self._struct_name = struct_name

    def ObjectStart(self, e):
        self._acc_gen.ObjectStart(e)
        self._struct_def_gen.ObjectStart(e)
        self._str_table_gen.ObjectStart(e)

    def Value(self, e):
        self._acc_gen.Value(e)
        self._struct_def_gen.Value(e)
        self._str_table_gen.Value(e)

    def ObjectEnd(self, e):
        self._acc_gen.ObjectEnd(e)
        self._struct_def_gen.ObjectEnd(e)
        self._str_table_gen.ObjectEnd(e)

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

#include "mgos_config_util.h"

#ifdef __cplusplus
extern "C" {{
#endif

{struct_def_lines}

{accessor_lines}

{str_table_lines}

/* Backward compatibility. */
const struct mgos_conf_entry *{name}_schema(void);

#ifdef __cplusplus
}}
#endif
""".format(cmd=' '.join(sys.argv),
           name=self._struct_name,
           struct_def_lines="\n".join(self._struct_def_gen.GetHeaderLines()),
           accessor_lines="\n".join(self._acc_gen.GetHeaderLines()),
           str_table_lines="\n".join(self._str_table_gen.GetHeaderLines()))


# Writes C source file with schema definition
class CWriter:
    def __init__(self, struct_name, c_global_name):
        self._acc_gen = AccessorsGen(struct_name, c_global_name)
        self._struct_def_gen = StructDefGen(struct_name)
        self._str_table_gen = StringTableGen(struct_name)
        self._struct_name = struct_name
        self._schema_lines = []
        self._start_indices = []

    def ObjectStart(self, e):
        self._acc_gen.ObjectStart(e)
        self._struct_def_gen.ObjectStart(e)
        self._str_table_gen.ObjectStart(e)
        self._start_indices.append(len(self._schema_lines))
        if not e.IsAbstract():
            self._schema_lines.append(None)  # Placeholder

    def Value(self, e):
        self._struct_def_gen.Value(e)
        self._str_table_gen.Value(e)
        if e.IsAbstract():
            return
        self._acc_gen.Value(e)
        self._schema_lines.append(
            '    {.type = %s, .key = "%s", .offset = offsetof(struct %s, %s)},'
            % (SchemaEntry._CONF_TYPES[e.vtype], e.key, self._struct_name, e.path))

    def ObjectEnd(self, e):
        self._acc_gen.ObjectEnd(e)
        self._struct_def_gen.ObjectEnd(e)
        self._str_table_gen.ObjectEnd(e)
        si = self._start_indices.pop()
        num_desc = len(self._schema_lines) - si - 1
        if not e.IsAbstract():
            self._schema_lines[si] = (
                '    {.type = CONF_TYPE_OBJECT, .key = "%s", .offset = offsetof(struct %s, %s), .num_desc = %d},'
                % (e.key, self._struct_name, e.path, num_desc))

    def __str__(self):
        return """\
/* clang-format off */
/*
 * Generated file - do not edit.
 * Command: {cmd}
 */

#include "{name}.h"

#include <stdbool.h>

#include "common/cs_file.h"

#include "mgos_config_util.h"

{struct_def_lines}

{accessor_lines}

const struct mgos_conf_entry *{name}_schema(void) {{
  return {name}_get_schema();
}}

/* Strings */
{str_table_lines}
""".format(cmd=" ".join(sys.argv),
           name=self._struct_name,
           num_entries=len(self._schema_lines) + 1,
           num_desc=len(self._schema_lines),
           schema_lines="\n".join(self._schema_lines),
           struct_def_lines="\n".join(self._struct_def_gen.GetSourceLines()),
           accessor_lines="\n".join(self._acc_gen.GetSourceLines()),
           str_table_lines="\n".join(self._str_table_gen.GetSourceLines()))


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
            s = yaml.safe_load(sf)
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
