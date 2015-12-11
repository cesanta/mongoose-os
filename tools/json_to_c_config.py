#!/usr/bin/env python

import os
import sys
import json

def do(obj, level, path, hdr, src):
  indent = '  ' * level
  # path is e.g. "sys_conf.wifi.ap"
  # name is the last component of it, i.e. current structure name
  name = path.split('.')[-1]
  # Start structure definition
  hdr.append(indent + 'struct ' + path.replace('.', '_') + ' {')
  for k, v in obj.iteritems():
    if type(v) is dict:
      # Nested structure
      do(v, level + 1, path + '.' + k, hdr, src)
    else:
      # TODO(lsm): generate function to free the struct, and to serialize it
      # key is e.g. "wifi.ap.ssid"
      key = path[len(path.split('.')[0]) + 1:] + '.' + k
      c_type = ('char *' if type(v) is unicode else 'int ')
      suffix = ('str' if type(v) is unicode else
                'bool' if type(v) is bool else 'int')
      # Add "  int foo;" line to the header - goes inside structure definition
      hdr.append(indent + '  ' + c_type + k + ';')
      # Add a line to the parsing function to parse this field, `key`
      src.append('''  if ({getter_func}(toks, "{key}", &dst->{key}) == 0 && require_keys) goto done;
'''.format(getter_func='sj_conf_get_' + suffix, key=key));
  # If this is a not root-level structure, add field name after the
  # strucutre definition
  hdr.append(indent + '}' + ('' if level == 0 else ' ' + name) + ';')

if __name__ == '__main__':
  obj = json.load(open(sys.argv[1]))
  origin = sys.argv[1]
  dest = sys.argv[2]
  hdr = []
  src = []
  name = os.path.split(dest)[-1]
  do(obj, 0, name, hdr, src)

  hdr.insert(0, '''/* generated from {origin} - do not edit */
#ifndef _{name_uc}_H_
#define _{name_uc}_H_
'''.format(name_uc=name.upper(), origin=origin))
  hdr.append('''
int parse_{name}(const char *, struct {name} *, int);

#endif /* _{name_uc}_H_ */
'''.format(name=name, name_uc=name.upper()));

  src.insert(0, '''/* generated from {origin} - do not edit */
#include "mongoose.h"
#include "{name}.h"
#include "sj_config.h"

int parse_{name}(const char *json, struct {name} *dst, int require_keys) {{
  struct json_token *toks = NULL;
  int result = 0;

  if (json == NULL) goto done;
  if ((toks = parse_json2(json, strlen(json))) == NULL) goto done;
'''.format(origin=origin, name=name))

  src.append('''  result = 1;
done:
  free(toks);
  return result;
}
''')

  open(dest + '.c', 'w+').write('\n'.join(src));
  open(dest + '.h', 'w+').write('\n'.join(hdr));
