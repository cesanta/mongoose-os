#!/usr/bin/env python
# vim: tabstop=2 expandtab bs=2 shiftwidth=2 ai cin smarttab

import argparse
import collections
import json
import os
import sys

parser = argparse.ArgumentParser(description='Create C config boilerplate from a JSON config definition')
parser.add_argument('--c_name', required=True, help="base path of generated files")
parser.add_argument('--dest_dir', default=".", help="base path of generated files")
parser.add_argument('json', nargs='+', help="JSON config definition files")

def do(obj, first_file, path, hdr, src_parse, src_emit, src_free):
  level = len(path) - 1
  indent = '  ' * level
  json_indent = '  ' * (level + 1)
  # path is e.g. "['sys_conf', 'wifi', 'ap']"
  # name is the last component of it, i.e. current structure name
  name = path[-1]
  # Start structure definition
  if level > 0:
    hdr.append(indent + 'struct ' + '_'.join(path) + ' {')
  first_key = True
  for k, v in obj.iteritems():
    key = '.'.join(path[1:] + [k])
    comma = ',' if (not first_key) or (level == 0 and not first_file) else ''
    if isinstance(v, dict):
      # Nested structure
      src_emit.append(
          r'  sj_conf_emit_str(&out, "{comma}\n{indent}\"", "{k}", "\": {{");'
              .format(comma=comma, indent=json_indent, k=k)
      );
      do(v, first_file, path + [k], hdr, src_parse, src_emit, src_free)
      src_emit.append(
          r'  sj_conf_emit_str(&out, "\n{indent}", "}}", "");'
              .format(indent=json_indent)
      );
    else:
      c_type = ('char *' if isinstance(v, basestring) else 'int ')
      getter = ('sj_conf_get_str' if isinstance(v, basestring) else
                'sj_conf_get_bool' if isinstance(v, bool) else
                'sj_conf_get_int')
      # Add "  int foo;" line to the header - goes inside structure definition
      hdr.append(indent + '  ' + c_type + k + ';')

      # Add statements to parse ...
      src_parse.append(
          r'  if ({getter}(toks, "{key}", acl, &cfg->{key}) == 0 && require_keys) goto done;'
          .format(getter=getter, key=key));
      # ... emit ...
      src_emit.append('')
      prefix = comma + r'\n' + json_indent
      if isinstance(v, bool):
        src_emit.append(
            r'''
  sj_conf_emit_str(&out, "{p}\"", "{k}",
                   (cfg->{key} ? "\": true" : "\": false"));'''
                .format(p=prefix, k=k, key=key)
        );
      else:
        src_emit.append(
            r'  sj_conf_emit_str(&out, "{p}\"", "{k}", "\": ");'
                .format(p=prefix, k=k)
        );
        if isinstance(v, basestring):
          src_emit.append(
              r'  sj_conf_emit_str(&out, "\"", cfg->{key}, "\"");'
                  .format(key=key)
          );
        else:
          src_emit.append(
              r'''  sj_conf_emit_int(&out, cfg->{key});'''.format(key=key)
          );
      # ... and free functions.
      if isinstance(v, basestring):
        src_free.append(r'  free(cfg->{key});'.format(key=key));
    first_key = False

  if level > 0:
    hdr.append(indent + '} %s;' % name)

if __name__ == '__main__':
  args = parser.parse_args()
  origin = ' '.join(args.json)
  hdr = []
  src_parse, src_emit, src_free = [], [], []
  name = os.path.basename(args.c_name)

  first_file = True
  for json_file in args.json:
    if os.path.isdir(json_file):
      continue
    with open(json_file) as jf:
      obj = json.load(jf, object_pairs_hook=collections.OrderedDict)
      do(obj, first_file, [name], hdr, src_parse, src_emit, src_free)
    first_file = False

  hdr.insert(0, '''/* generated from {origin} - do not edit */
#ifndef _{name_uc}_H_
#define _{name_uc}_H_

struct {name} {{\
'''.format(name=name, name_uc=name.upper(), origin=origin))
  hdr.append('''\
}};

int parse_{name}(const char *json, const char *acl, int require_keys,
                 struct {name} *cfg);
char *emit_{name}(const struct {name} *cfg);
void free_{name}(struct {name} *cfg);

#endif /* _{name_uc}_H_ */
'''.format(name=name, name_uc=name.upper()));
  open(os.path.join(args.dest_dir, name + '.h'), 'w+').write('\n'.join(hdr));

  with open(os.path.join(args.dest_dir, name + '.c'), 'w') as sf:
    sf.write('''\
/* generated from {origin} - do not edit */
#include "mongoose/mongoose.h"
#include "smartjs/src/sj_config.h"
#include "{name}.h"

int parse_{name}(const char *json, const char *acl, int require_keys,
                 struct {name} *cfg) {{
  struct json_token *toks = NULL;
  int result = 0;

  if (json == NULL) goto done;
  if ((toks = parse_json2(json, strlen(json))) == NULL) goto done;

{src_parse}

  result = 1;

done:
  free(toks);
  return result;
}}

char *emit_{name}(const struct {name} *cfg) {{
  struct mbuf out;
  mbuf_init(&out, 2000);

  mbuf_append(&out, "{{", 1);

{src_emit}

  mbuf_append(&out, "\\n}}", 3);  /* Including NUL. */
  mbuf_trim(&out);

  return out.buf;
}}

void free_{name}(struct {name} *cfg) {{
{src_free}
}}
'''.format(origin=origin, name=name,
           src_parse='\n'.join(src_parse),
           src_emit='\n'.join(src_emit),
           src_free='\n'.join(src_free)))
