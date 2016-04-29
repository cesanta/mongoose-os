#!/usr/bin/env python
#
# Firmware metadata management script.
#
# Generally, executed 3 times:
# 1) To extract build info from Git repo and produce build info .c and/or .json:
#    fw_meta.py gen_build_info \
#      --c_output=build_info.c \
#      --json_output=build_info.json
#    This assumes that the script is run from within a Git repo.
# 2) Once firmware parts are ready, to create the firmware contents manifest:
#    fw_meta.py create_manifest \
#      --name=MyApp --platform=MyPlatform \
#      --build_info=build_info.json \
#      --output=manifest.json \
#      --src_dir=/path/to/where/fw/parts/are/located \
#      $(FW_PARTS)
#    FW_PARTS is a list of firmware "parts", each defined by name:k=v,k=v,...
#    entry. Exact key and values are dependent on platform, also for some
#    platforms names may have special meaning. For parts that have the "src"
#    attribute, the script will interpret it as a file relative to --src_dir
#    and will compute SHA1 checksum for it and add as a "cs_sha1".
# 3) To create firmware from parts and manifest:
#    fw_meta.py create_fw \
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
import sys
import zipfile

# Debian/Ubuntu: apt-get install python-git
# PIP: pip install GitPython
import git

FW_MANIFEST_FILE_NAME = 'manifest.json'


def get_git_repo(path):
    # This is a temporary workaround until we get a version of python-git
    # that supports search_parent_directories=True (1.0 and up).
    repo_dir = path
    repo = None
    while repo is None:
        try:
            return git.Repo(repo_dir)
        except git.exc.InvalidGitRepositoryError:
            if repo_dir != '/':
                repo_dir = os.path.split(repo_dir)[0]
                continue
            raise RuntimeError("%s doesn't look like a Git repo" % path)


def get_tag_for_commit(repo, commit):
    for tag in repo.tags:
        if tag.commit == commit:
            return tag.name
    return None

def _write_build_info(bi, args):
    if args.json_output:
        if args.json_output == '-':
            out = sys.stdout
        else:
            out = open(args.json_output, 'w')
        json.dump(bi, out, indent=2, sort_keys=True)

    if args.c_output:
        if args.c_output == '-':
            out = sys.stdout
        else:
            out = open(args.c_output, 'w')
        print >>out, """\
/* Auto-generated, do not edit. */
const char *build_id = "%(build_id)s";
const char *build_timestamp = "%(build_timestamp)s";
const char *build_version = "%(build_version)s";\
""" % bi


def cmd_gen_build_info(args):
    bi = {}

    ts = datetime.datetime.utcnow()
    timestamp = None
    if args.timestamp is None:
        timestamp = ts.isoformat()
    elif args.timestamp != '':
        timestamp = args.timestamp
    if timestamp is not None:
        bi['build_timestamp'] = timestamp

    version = None
    if args.version is None:
        version = None
        if args.tag_as_version:
            repo = get_git_repo(os.getcwd())
            version = get_tag_for_commit(repo, repo.head.commit)
        if version is None:
            version = ts.strftime('%Y%m%d%H%M%S')
    elif args.version != '':
        version = args.version
    if version is not None:
        bi['build_version'] = version

    id = None
    if args.id is None:
        repo = get_git_repo(os.getcwd())
        if repo.head.is_detached:
            branch_or_tag = get_tag_for_commit(repo, repo.head.commit)
            if branch_or_tag is None:
                branch_or_tag = '?'
        else:
            branch_or_tag = repo.active_branch

        if args.dirty == "auto":
            dirty = repo.is_dirty()
        else:
            dirty = args.dirty == "true"
        id = '%s/%s@%s%s' % (
            ts.strftime('%Y%m%d-%H%M%S'),
            branch_or_tag,
            str(repo.head.commit)[:8],
            '+' if dirty else '')
    elif args.id != '':
        id = args.id
    if id is not None:
        bi['build_id'] = id

    _write_build_info(bi, args)


def cmd_get_build_info(args):
    manifest = json.load(open(args.manifest))
    bi = dict(
        build_timestamp=manifest.get("build_timestamp"),
        build_version=manifest.get("version"),
        build_id=manifest.get("build_id"),
    )

    _write_build_info(bi, args)

def cmd_create_manifest(args):
    manifest = {
        'name': args.name,
        'platform': args.platform,
    }
    if args.description:
        manifest['description'] = args.description

    if os.path.exists(args.build_info):
        bi = json.load(open(args.build_info))
    else:
        bi = json.loads(args.build_info)

    manifest['version'] = bi['build_version']
    for k in ('build_id', 'build_timestamp'):
        if k in bi:
            manifest[k] = bi[k]

    for p in args.parts:
        name, attrs = p.split(':', 2)
        part = {}
        for kv in attrs.split(','):
            k, v = kv.split('=', 2)
            part[k] = v
        if args.checksums and 'src' in part:
            # TODO(rojer): Support non-local sources.
            src_file = part['src']
            if args.src_dir:
                src_file = os.path.join(args.src_dir, src_file)
            with open(src_file) as f:
                for algo in args.checksums.split(','):
                    h = hashlib.new(algo)
                    h.update(f.read())
                    part['cs_%s' % algo] = h.hexdigest()
        manifest.setdefault('parts', {})[name] = part

    if args.output:
        out = open(args.output, 'w')
    else:
        out = sys.stdout
    json.dump(manifest, out, indent=2, sort_keys=True)


def cmd_create_fw(args):
    manifest = json.load(open(args.manifest))
    arc_dir = '%s-%s' % (manifest['name'], manifest['version'])
    with zipfile.ZipFile(args.output, 'w', zipfile.ZIP_STORED) as zf:
        manifest_arc_name = os.path.join(arc_dir, FW_MANIFEST_FILE_NAME)
        zf.writestr(manifest_arc_name, json.dumps(manifest, indent=2, sort_keys=True))
        for _, part in manifest['parts'].items():
            if 'src' not in part:
                continue
            # TODO(rojer): Support non-local sources.
            src_file = part['src']
            if args.src_dir:
                src_file = os.path.join(args.src_dir, src_file)
            arc_file = os.path.join(arc_dir, os.path.basename(src_file))
            zf.write(src_file, arc_file)
            part['src'] = os.path.basename(arc_file)


def cmd_get(args):
    o = json.load(open(args.json_file))
    for key in args.keys:
        d = o
        parts = key.split('.')
        for p in parts:
            v = d[p]
            d = v
        print v


def cmd_set(args):
    o = json.load(open(args.json_file))
    for key_value in args.key_values:
        key, value = key_value.split("=")
        d = o
        parts = key.split('.')
        for p in parts[:-1]:
            v = d[p]
            d = v
        d[parts[-1]] = value
    if args.inplace:
        json.dump(o, open(args.json_file, "w"))
    else:
        print json.dumps(o)

if __name__ == '__main__':
    handlers = {}
    parser = argparse.ArgumentParser(description='FW metadata tool', prog='fw_manifest')
    cmd = parser.add_subparsers(dest='cmd')
    gbi_desc = "Generate build info"
    gbi_cmd = cmd.add_parser('gen_build_info', help=gbi_desc, description=gbi_desc)
    gbi_cmd.add_argument('--timestamp', '-t')
    gbi_cmd.add_argument('--version', '-v')
    gbi_cmd.add_argument('--id', '-i')
    gbi_cmd.add_argument('--dirty', default="auto", choices=["auto", "true", "false"])
    gbi_cmd.add_argument('--tag_as_version', type=bool, default=False)
    gbi_cmd.add_argument('--json_output')
    gbi_cmd.add_argument('--c_output')
    handlers['gen_build_info'] = cmd_gen_build_info

    gtbi_desc = "Extract build info from manifest"
    gtbi_cmd = cmd.add_parser('get_build_info', help=gtbi_desc, description=gtbi_desc)
    gtbi_cmd.add_argument('--manifest', '-m', required=True)
    gtbi_cmd.add_argument('--json_output')
    gtbi_cmd.add_argument('--c_output')
    handlers['get_build_info'] = cmd_get_build_info

    cm_desc = "Create manifest"
    cm_cmd = cmd.add_parser('create_manifest', help=cm_desc, description=cm_desc)
    cm_cmd.add_argument('--name', '-n', required=True)
    cm_cmd.add_argument('--platform', '-p', required=True)
    cm_cmd.add_argument('--build_info', '-i', required=True)
    cm_cmd.add_argument('--description', '-d')
    cm_cmd.add_argument('--checksums', default='sha1')
    cm_cmd.add_argument('--src_dir')
    cm_cmd.add_argument('--output', '-o')
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

    args = parser.parse_args()
    handlers[args.cmd](args)
