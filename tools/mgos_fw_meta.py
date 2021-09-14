#!/usr/bin/env python3
#
# Firmware metadata management script.
#
# Generally, executed 3 times:
# 1) To extract build info from Git repo and produce build info .c and/or .json:
#    mgos_fw_meta.py gen_build_info \
#      --c_output=build_info.c \
#      --json_output=build_info.json
#    This assumes that the script is run from within a Git repo.
# 2) Once firmware parts are ready, to create the firmware contents manifest:
#    mgos_fw_meta.py create_manifest \
#      --name=MyApp --platform=MyPlatform \
#      --build_info=build_info.json \
#      --output=manifest.json \
#      --src_dir=/path/to/where/fw/parts/are/located \
#      $(FW_PARTS)
#    FW_PARTS is a list of firmware "parts", each defined by name:k=v,k=v,...
#    entry. Exact key and values are dependent on platform, also for some
#    platforms names may have special meaning. For parts that have the "src"
#    attribute, the script will interpret it as a file or directory and will
#    compute SHA1 checksum for it and add as a "cs_sha1". If src is a file name
#    and is relative, it is based in --src_dir. If --staging_dir is set,
#    files are copied there. If src is a directory, "src" element in manifest
#    will be an object, with files as subobjects (and cs_sha1 as attribute)
# 3) To create firmware from parts and manifest:
#    mgos_fw_meta.py create_fw \
#      --manifest=manifest.json \
#      --src_dir=/path/to/where/fw/parts/are/located \
#      --output=firmware.zip
#    This will roll a ZIP archive with the firmware.
#    ZIP file will nclude the manifest and any files mentioned in "src"
#    attributes of the parts.

import argparse
import datetime
import hashlib
import json
import os
import re
import string
import subprocess
import sys
import zipfile

FW_MANIFEST_FILE_NAME = 'manifest.json'

# From http://stackoverflow.com/questions/241327/python-snippet-to-remove-c-and-c-comments#241506
def remove_comments(text):
    def replacer(match):
        s = match.group(0)
        if s.startswith('/'):
            return " "  # note: a space and not an empty string
        else:
            return s
    pattern = re.compile(
        r'//.*?$|/\*.*?\*/|\'(?:\\.|[^\\\'])*\'|"(?:\\.|[^\\"])*"',
        re.DOTALL | re.MULTILINE
    )
    return re.sub(pattern, replacer, text)


class FFISymbol:
    def __init__(self, name, return_type, args):
        self.name = name
        self.return_type = return_type
        self.args = args.replace("userdata", "void *")
        if self.return_type == "":
            self.return_type = "void"
        if self.args == "":
            self.args = "(void)"

    def __lt__(self, other):
        return self.name < other.name

    def symbol_name(self):
        return self.name

    def signature(self):
        return self.return_type + " " + self.name + self.args


class WriteIfModifiedFile:
    def __init__(self, name):
        self._name = name
        self._data = bytes()

    def write(self, data):
        self._data += data.encode("utf-8")

    def close(self):
        try:
            with open(self._name, "rb") as f:
                data = f.read()
                if data == self._data:
                    return
        except OSError:
            pass
        with open(self._name, "wb") as f:
            f.write(self._data)


def file_or_stdout(fname):
    if fname == '-':
        return sys.stdout
    dirname = os.path.dirname(fname)
    if dirname:
        try:
            os.makedirs(dirname, mode=0o755)
        except Exception:
            pass
    return WriteIfModifiedFile(fname)


def _write_build_info(bi, args):
    if args.json_output:
        out = file_or_stdout(args.json_output)
        if args.var_prefix:
            bij = dict((args.var_prefix + k, v) for k, v in bi.items())
        else:
            bij = bi
        json.dump(bij, out, indent=2, sort_keys=True)
        out.close()

    bi['var_prefix'] = args.var_prefix

    if args.c_output:
        out = file_or_stdout(args.c_output)
        print("""\
/* Auto-generated, do not edit. */
const char *%(var_prefix)sbuild_id = "%(build_id)s";
const char *%(var_prefix)sbuild_timestamp = "%(build_timestamp)s";
const char *%(var_prefix)sbuild_version = "%(build_version)s";\
""" % bi, file=out)
        out.close()

    if args.go_output:
        out = file_or_stdout(args.go_output)
        print("""\
/* Auto-generated, do not edit. */
package version

const (
    %(var_prefix)sVersion = "%(build_version)s"
    %(var_prefix)sBuildId = "%(build_id)s"
    %(var_prefix)sBuildTimestamp = "%(build_timestamp)s"
)\
""" % bi, file=out)
        out.close()


def cmd_gen_build_info(args):
    bi = {}
    repo = None
    repo_path = args.repo_path or os.getcwd()

    ts = datetime.datetime.utcnow()
    timestamp = None
    if args.timestamp:
        timestamp = args.timestamp
    else:
        timestamp = ts.replace(microsecond=0).isoformat() + 'Z'
    if timestamp is not None:
        bi['build_timestamp'] = timestamp

    try:
        # This will output distance to most recent tag and dirty marker:
        # 2.10.0
        # 2.10.0-dirty
        # 2.10.0-280-g20ab9a031-dirty
        # or, for repo that doesn;t have any tags:
        # 20ab9a031
        # 20ab9a031-dirty
        git_describe_out = subprocess.check_output(
                ["git", "-C", repo_path, "describe", "--dirty", "--tags", "--always"],
                universal_newlines=True, stderr=subprocess.PIPE).strip()
        # branch name (if any)
        git_revparse_out = subprocess.check_output(
                ["git", "-C", repo_path, "rev-parse", "--abbrev-ref", "HEAD"],
                universal_newlines=True).strip()
        git_log_head_out = subprocess.check_output(
                ["git", "-C", repo_path, "log", "-n", "1", "HEAD", "--format=%H %ct"],
                universal_newlines=True).strip()
    except Exception as e:
        git_describe_out = ""
        git_revparse_out = ""
        git_log_head_out = ""

    version = None
    if args.version:
        version = args.version
    else:
        version = None
        # If we are precisely at a specific tag, repo is clean - use it as version.
        if args.tag_as_version and git_describe_out and "-" not in git_describe_out:
            version = git_describe_out
        # Otherwise, if it's a clean git repo, use last commit's timestamp.
        elif git_log_head_out and "dirty" not in git_describe_out:
            vts = datetime.datetime.utcfromtimestamp(int(git_log_head_out.split()[1]))
        # If all else fails, use current timestamp
        else:
            vts = ts
        if version is None:
            version = vts.strftime('%Y%m%d%H%M')
    if version is not None:
        bi['build_version'] = version

    build_id = None
    if args.id:
        build_id = args.id
    else:
        build_id = ts.strftime('%Y%m%d-%H%M%S')
        if git_describe_out:
            build_id += "/"
            parts = []
            if "-dirty" in git_describe_out:
                dirty = True
                git_describe_out = git_describe_out.replace("-dirty", "")
            else:
                dirty = False
            # Most recent tag + offset
            head_hash = git_log_head_out.split()[0][:7]
            if not git_describe_out.startswith(head_hash):
                # We have tag(s).
                parts.append(git_describe_out)
            # Add current hash (it won't be there if HEAD is exactly at tag)
            if head_hash not in git_describe_out or git_describe_out.startswith(head_hash):
                parts.append("g%s" % head_hash)
            # Branch name
            if git_revparse_out != "HEAD":
                parts.append(git_revparse_out)
            if dirty:
                parts.append("dirty")
            build_id += "-".join(parts)
            # Some possible results:
            # 20190127-050931/gc2393b3-master - repo with no tags, master branch, clean.
            # 20190127-050931/gc2393b3-master-dirty - as above + local chnages.
            # 20190127-051054/2.0.0-gc2393b3-master - tag 2.0.0 (exactly), master branch, clean.
            # 20190127-051234/2.0.0-gc2393b3-master-dirty - as above + local changes.
            # 20190127-051341/2.0.0-1-g02e78c7-master - 1 commit from 2.0.0, master branch, clean.

    if build_id is not None:
        bi['build_id'] = build_id

    _write_build_info(bi, args)


def cmd_gen_ffi_exports(args):
    patterns = args.patterns.split()
    symbols = []

    # Blindly append non-glob patterns
    for p in patterns:
        if "*" not in p:
            symbols.append(FFISymbol(p, "", ""))

    # Scan all provided js files for ffi("..."), and fetch symbol names from
    # there
    for js_file in args.js_files:
        with open(js_file, encoding="utf-8") as f:
            data = f.read()
            # Remove // and /* */ comments from the data
            data = remove_comments(data)
            # Remove newlines
            data = data.replace('\n', '')
            # Note: must match things like:
            #       char *foo(int bar)
            #
            # symbol_type: "char *"
            # symbol_name: "foo"
            for m in re.finditer(r"""\bffi\s*\(\s*(?P<quote>['"])(?P<symbol_type>[^)]+?\W)(?P<symbol_name>\w+)(?P<args>.+?)(?P=quote)""", data):
                symbols.append(FFISymbol(m.group("symbol_name"), m.group("symbol_type"), m.group("args")))

    symbols.sort()

    out = file_or_stdout(args.c_output)
    print("/* Auto-generated, do not edit. */\n", file=out)
    print("/*", file=out)
    print(" * Symbols filtered by the following globs:", file=out)
    for p in patterns:
        print(" *  %s" % p, file=out)
    print(" */\n", file=out)
    print("#include <stdbool.h>\n", file=out)
    print("#include \"mgos_dlsym.h\"\n", file=out)

    # Emit forward declarations of all symbols to be exported
    print("/* NOTE: signatures are fake */", file=out)
    for symbol in symbols:
        print("%s;" % symbol.signature(), file=out)

    print("""\

const struct mgos_ffi_export ffi_exports[] = {""", file=out)

    # Emit all symbols
    for symbol in symbols:
        print("  {\"%s\", %s}," % (symbol.symbol_name(), symbol.symbol_name()), file=out)

    print("};", file=out)
    print("const int ffi_exports_cnt = %d;" % len(symbols), file=out)
    out.close()


def cmd_get_build_info(args):
    manifest = json.load(open(args.manifest, encoding="utf-8"))
    bi = dict(
        build_timestamp=manifest.get("build_timestamp"),
        build_version=manifest.get("version"),
        build_id=manifest.get("build_id"),
    )

    _write_build_info(bi, args)


def unquote_string(qs):
    if len(qs) < 2 or qs[0] != qs[-1]:
        raise ValueError("unmatched quote")
    q = qs[0]
    s = qs[1:-1]
    s = s.replace(r'\%s' % q, q)
    return s


def stage_file_and_calc_digest(args, part, fname, staging_dir):
    with open(fname, 'rb') as f:
        data = f.read()
        part['size'] = len(data)
        if staging_dir:
            staging_file = os.path.join(staging_dir,
                                        os.path.basename(fname))
            with open(staging_file, 'wb') as sf:
                sf.write(data)
        for algo in args.checksums.split(','):
            h = hashlib.new(algo)
            h.update(data)
            part['cs_%s' % algo] = h.hexdigest()


def cmd_create_manifest(args):
    manifest = {
        'name': args.name,
        'platform': args.platform,
    }
    if args.description:
        manifest['description'] = args.description

    if os.path.exists(args.build_info):
        bi = json.load(open(args.build_info, encoding="utf-8"))
    else:
        bi = json.loads(args.build_info)

    manifest['version'] = bi['build_version']
    for k in ('build_id', 'build_timestamp'):
        if k in bi:
            manifest[k] = bi[k]

    for e in args.extra_attr:
        if not e:
            continue
        k, v = e.split('=', 1)
        if v in ('true', 'false'):
            v = (v == 'true')
        elif v == 'null':
            v = None
        else:
            try:
                v = int(v, 0)
            except ValueError:
                pass
        manifest[k] = v

    for p in args.parts:
        name, attrs = p.split(':', 2)
        part = {}
        for kv in attrs.split(','):
            k, v = kv.split('=', 2)
            if v == '':
                pass
            elif v[0] == "'" or v[0] == '"':
                v = unquote_string(v)
            elif v in ('true', 'false'):
                v = (v == 'true')
            else:
                try:
                    v = int(v, 0)  # Parses ints, including 0x
                except ValueError:
                    try:
                        v = float(v)
                    except ValueError:
                        pass  # Ok, leave as string.
            part[k] = v
        if args.checksums and 'src' in part:
            # TODO(rojer): Support non-local sources.
            src = part['src']
            if not os.path.isabs(src) and not os.path.exists(src):
                src = os.path.join(args.src_dir, src)
            if os.path.isfile(src):
                part['src'] = os.path.basename(src)
                stage_file_and_calc_digest(args, part, src, args.staging_dir)
            else:
                files = {}
                staging_dir = os.path.join(args.staging_dir, name)
                if not os.path.exists(staging_dir):
                    os.makedirs(staging_dir)
                for fname in os.listdir(src):
                    file_attrs = {}
                    stage_file_and_calc_digest(args, file_attrs,
                                               os.path.join(src, fname),
                                               staging_dir)
                    files[fname] = file_attrs
                del part['src']
                part['src'] = files

        manifest.setdefault('parts', {})[name] = part

    if args.output:
        out = open(args.output, 'w', encoding="utf-8")
    else:
        out = sys.stdout
    json.dump(manifest, out, indent=2, sort_keys=True)


def add_file_to_arc(args, part, arc_dir, src_file, added):
    if args.src_dir:
        src_file = os.path.join(args.src_dir, src_file)
    arc_file = os.path.join(arc_dir, os.path.basename(src_file))
    if arc_file not in added:
        added[arc_file] = src_file


def cmd_create_fw(args):
    manifest = json.load(open(args.manifest, encoding="utf-8"))
    arc_dir = '%s-%s' % (manifest['name'], manifest['version'])
    to_add = {}
    with zipfile.ZipFile(args.output, 'w', zipfile.ZIP_STORED) as zf:
        manifest_arc_name = os.path.join(arc_dir, FW_MANIFEST_FILE_NAME)
        zf.writestr(manifest_arc_name, json.dumps(manifest, indent=2, sort_keys=True))
        to_add = {}
        for part_name, part in manifest['parts'].items():
            if 'src' not in part:
                continue
            # TODO(rojer): Support non-local sources.
            src = part['src']
            if isinstance(src, str):
                add_file_to_arc(args, part, arc_dir, src, to_add)
            else:
                # src is object with files as a keys
                for fname, _ in src.items():
                    add_file_to_arc(args, part,
                                    os.path.join(arc_dir, part_name),
                                    os.path.join(part_name, fname), to_add)
        for arc_file, src_file in sorted(to_add.items()):
            print('     Adding %s' % src_file)
            zf.write(src_file, arc_file)


def cmd_get(args):
    o = json.load(open(args.json_file, encoding="utf-8"))
    for key in args.keys:
        d = o
        parts = key.split('.')
        for p in parts:
            v = d[p]
            d = v
        print(v)


def cmd_set(args):
    o = json.load(open(args.json_file, encoding="utf-8"))
    for key_value in args.key_values:
        key, value = key_value.split("=")
        d = o
        parts = key.split('.')
        for p in parts[:-1]:
            v = d[p]
            d = v
        d[parts[-1]] = value
    if args.inplace:
        json.dump(o, open(args.json_file, "w", encoding="utf-8"))
    else:
        print(json.dumps(o))


def cmd_xxd(args):
    total_len = 0
    const = "const " if args.const else ""
    sect = ' __attribute((section("%s")))' % args.section if args.section else ""
    with open(args.input_file, "rb") as f:
        print("""\
/* Generated file, do not edit! */
/* Source: %s */
%sunsigned char %s[]%s = {\
""" % (args.input_file, const, args.var_name, sect))
        while True:
            bb = f.read(16)
            if len(bb) == 0: break
            print("  ", end="")
            print(*["0x%02x" % b for b in bb], sep=", ", end=",\n")
            total_len += len(bb)
    print("""\
};
const unsigned int %s_len = %u;\
""" % (args.var_name, total_len))


if __name__ == '__main__':
    handlers = {}
    parser = argparse.ArgumentParser(description='Mopngoose OS metadata tool', prog='mgos_fw_meta')
    cmd = parser.add_subparsers(dest='cmd')
    gbi_desc = "Generate build info"
    gbi_cmd = cmd.add_parser('gen_build_info', help=gbi_desc, description=gbi_desc)
    gbi_cmd.add_argument('--timestamp', '-t')
    gbi_cmd.add_argument('--version', '-v')
    gbi_cmd.add_argument('--id', '-i')
    gbi_cmd.add_argument('--repo_path')
    gbi_cmd.add_argument('--dirty', default="auto", choices=["auto", "true", "false"])
    gbi_cmd.add_argument('--tag_as_version', type=bool, default=False)
    gbi_cmd.add_argument('--var_prefix', default='')
    gbi_cmd.add_argument('--json_output')
    gbi_cmd.add_argument('--c_output')
    gbi_cmd.add_argument('--go_output')
    handlers['gen_build_info'] = cmd_gen_build_info

    gfe_desc = "Generate FFI exports file"
    gfe_cmd = cmd.add_parser('gen_ffi_exports', help=gfe_desc, description=gfe_desc)
    gfe_cmd.add_argument('--c_output')
    gfe_cmd.add_argument('--patterns')
    gfe_cmd.add_argument('js_files', nargs='*', default=[])
    handlers['gen_ffi_exports'] = cmd_gen_ffi_exports

    gtbi_desc = "Extract build info from manifest"
    gtbi_cmd = cmd.add_parser('get_build_info', help=gtbi_desc, description=gtbi_desc)
    gtbi_cmd.add_argument('--manifest', '-m', required=True)
    gtbi_cmd.add_argument('--var_prefix', default='')
    gtbi_cmd.add_argument('--json_output')
    gtbi_cmd.add_argument('--c_output')
    gtbi_cmd.add_argument('--go_output')
    handlers['get_build_info'] = cmd_get_build_info

    cm_desc = "Create manifest"
    cm_cmd = cmd.add_parser('create_manifest', help=cm_desc, description=cm_desc)
    cm_cmd.add_argument('--name', '-n', required=True)
    cm_cmd.add_argument('--platform', '-p', required=True)
    cm_cmd.add_argument('--build_info', '-i', required=True)
    cm_cmd.add_argument('--description', '-d')
    cm_cmd.add_argument('--checksums', default='sha1,sha256')
    cm_cmd.add_argument('--src_dir', default='.')
    cm_cmd.add_argument('--staging_dir')
    cm_cmd.add_argument('--output', '-o')
    cm_cmd.add_argument('--extra-attr', metavar='extra_attrs', default=[], action='append')
    cm_cmd.add_argument('parts', nargs='+')
    handlers['create_manifest'] = cmd_create_manifest

    cf_desc = "Create firmware ZIP"
    cf_cmd = cmd.add_parser('create_fw', help=cf_desc, description=cf_desc)
    cf_cmd.add_argument('--manifest', '-m', required=True)
    cf_cmd.add_argument('--output', '-o', required=True)
    cf_cmd.add_argument('--src_dir')
    handlers['create_fw'] = cmd_create_fw

    get_desc = "Extract keys from a JSON file"
    get_cmd = cmd.add_parser('get', help=get_desc, description=get_desc)
    get_cmd.add_argument('json_file')
    get_cmd.add_argument('keys', nargs='+')
    handlers['get'] = cmd_get

    set_desc = "Modify keys in a JSON file"
    set_cmd = cmd.add_parser('set', help=set_desc, description=set_desc)
    set_cmd.add_argument('--inplace', '-i', action='store_true')
    set_cmd.add_argument('json_file')
    set_cmd.add_argument('key_values', nargs='+')
    handlers['set'] = cmd_set

    xxd_desc = "Convert a binary file into C source"
    xxd_cmd = cmd.add_parser('xxd', help=xxd_desc, description=xxd_desc)
    xxd_cmd.add_argument('--var_name', help="C variable name", required=True)
    xxd_cmd.add_argument('--section', help="Add section attribute")
    xxd_cmd.add_argument('--const', help="Make the variable const", action='store_true')
    xxd_cmd.add_argument('input_file')
    handlers['xxd'] = cmd_xxd

    args = parser.parse_args()
    handlers[args.cmd](args)
