#!/usr/bin/env python
# vim: tabstop=4 expandtab shiftwidth=4 ai cin smarttab
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
# struct foo_config {
#   struct foo_config_foo {
#     struct foo_config_foo_bar {
#       char *name;
#       int num_quux;
#     } bar;
#     int frombulate;
#   } foo;
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
parser.add_argument("--c_const_char", type=bool, default=False, help="Generate const char members for strings")
parser.add_argument("--dest_dir", default=".", help="base path of generated files")
parser.add_argument("schema_files", nargs="+", help="YAML schema files")


class SchemaEntry(object):
    V_OBJ = "o"
    V_BOOL = "b"
    V_INT = "i"
    V_STRING = "s"

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
            elif self.vtype == SchemaEntry.V_STRING:
                self.default = ""
        elif len(e) == 4:
            self.path, self.vtype, self.default, self.params = e
        else:
            raise ValueError("Invalid entry length: %s (%s)" % (len(e), e))

        if not isinstance(self.path, basestring):
            raise TypeError("Path is not a string (%s)" % e)

        if self.vtype is not None:
            if self.vtype not in (self.V_OBJ, self.V_BOOL, self.V_INT, self.V_STRING):
                raise TypeError("%s: Invalid value type '%s'" % (self.path, self.vtype))
            self.ValidateDefault()

        if self.params is not None and not isinstance(self.params, dict):
            raise TypeError("%s: Invalid params" % self.path)

    def ValidateDefault(self):
        if (self.vtype == SchemaEntry.V_BOOL and not isinstance(self.default, bool) or
            self.vtype == SchemaEntry.V_INT and not isinstance(self.default, int) or
            self.vtype == SchemaEntry.V_STRING and not isinstance(self.default, basestring)):
            raise TypeError("%s: Invalid default value type" % self.path)

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
                    if oe.vtype == SchemaEntry.V_OBJ:
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
        if e.vtype == SchemaEntry.V_OBJ:
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
        self._schema.append([e.path, SchemaEntry.V_OBJ, e.params])

    def Value(self, e):
        self._schema.append([e.path, e.vtype, e.params])

    def ObjectEnd(self, _e):
        pass

    def __str__(self):
        # return a bit more compact representation, one entry per line.
        return "[\n%s\n]\n" % ",\n".join(
            "  %s" % json.dumps(e, sort_keys=True) for e in self._schema)


# Writes C header file.
class HWriter(object):
    def __init__(self, struct_name, const_char):
        self._struct_name = struct_name
        self._const_char = const_char
        self._lines = []
        self._indent = 2

    def _Indent(self):
        return " " * self._indent

    def ObjectStart(self, e):
        self._lines.append(
            (" " * self._indent) +
            ("struct %s_%s {" % (self._struct_name, e.path.replace(".", "_"))))
        self._indent += 2

    def Value(self, e):
        key = e.key
        if e.vtype in (SchemaEntry.V_BOOL, SchemaEntry.V_INT):
            self._lines.append(self._Indent() + ("int %s;" % key))
        elif e.vtype == SchemaEntry.V_STRING:
            if self._const_char:
                self._lines.append(self._Indent() + ("const char *%s;" % key))
            else:
                self._lines.append(self._Indent() + ("char *%s;" % key))

    def ObjectEnd(self, e):
        self._indent -= 2
        self._lines.append(self._Indent() + ("} %s;" % e.key))

    def __str__(self):
        return """\
/* Generated file - do not edit. */

#ifndef {name_uc}_H_
#define {name_uc}_H_

#include "fw/src/miot_config.h"

struct {name} {{
{lines}
}};

const struct miot_conf_entry *{name}_schema();

#endif /* {name_uc}_H_ */
""".format(name=self._struct_name,
           name_uc=self._struct_name.upper(),
           lines="\n".join(self._lines))


# Writes C source file with schema definition.
class CWriter(object):
    _CONF_TYPES = {
        SchemaEntry.V_BOOL: "CONF_TYPE_BOOL",
        SchemaEntry.V_INT: "CONF_TYPE_INT",
        SchemaEntry.V_STRING: "CONF_TYPE_STRING",
    }

    def __init__(self, struct_name):
        self._struct_name = struct_name
        self._lines = []
        self._start_indices = []

    def ObjectStart(self, _e):
        self._start_indices.append(len(self._lines))
        self._lines.append(None)  # Placeholder

    def Value(self, e):
        self._lines.append(
            '  {.type = %s, .key = "%s", .offset = offsetof(struct %s, %s)},'
            % (self._CONF_TYPES[e.vtype], e.key, self._struct_name, e.path))

    def ObjectEnd(self, e):
        si = self._start_indices.pop()
        num_desc = len(self._lines) - si - 1
        self._lines[si] = (
            '  {.type = CONF_TYPE_OBJECT, .key = "%s", .num_desc = %d},'
            % (e.key, num_desc))

    def __str__(self):
        return """\
/* Generated file - do not edit. */

#include <stddef.h>
#include "{name}.h"

const struct miot_conf_entry {name}_schema_[{num_entries}] = {{
  {{.type = CONF_TYPE_OBJECT, .key = "", .num_desc = {num_desc}}},
{lines}
}};

const struct miot_conf_entry *{name}_schema() {{
  return {name}_schema_;
}}
""".format(name=self._struct_name,
           num_entries=len(self._lines) + 1,
           num_desc=len(self._lines),
           lines="\n".join(self._lines))


@contextlib. contextmanager
def open_with_temp(name):
    """Perform a write-and-rename maneuver to write a file safely."""
    tmpname = "%s.%d.tmp" % (name, os.getpid())
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

    hw = HWriter(args.c_name, args.c_const_char)
    schema.Walk(hw)
    hfn = os.path.join(args.dest_dir, "%s.h" % args.c_name)
    with open_with_temp(hfn) as hf:
        hf.write(str(hw))

    cw = CWriter(args.c_name)
    schema.Walk(cw)
    cfn = os.path.join(args.dest_dir, "%s.c" % args.c_name)
    with open_with_temp(cfn) as cf:
        cf.write(str(cw))
