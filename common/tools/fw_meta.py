#!/usr/bin/env python

import argparse
import datetime
import hashlib
import json
import os
import sys
import zipfile

# Debian/Ubuntu: python-git
import git

FW_MANIFEST_FILE_NAME = 'manifest.json'


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
        version = ts.strftime('%Y%m%d%H%M%S')
    elif args.version != '':
        version = args.version
    if version is not None:
        bi['build_version'] = version

    id = None
    if args.id is None:
        # This is a temporary workaround until we get a version of python-git
        # that supports search_parent_directories=True (1.0 and up).
        repo_dir = os.getcwd()
        repo = None
        while repo is None:
            try:
                repo = git.Repo(repo_dir)
            except git.exc.InvalidGitRepositoryError:
                if repo_dir != '/':
                    repo_dir = os.path.split(repo_dir)[0]
                    continue
                raise RuntimeError("--id is not set and CWD doesn't look like a Git repo")
        branch_or_tag = '?'
        if repo.head.is_detached:
            # Try to find tag for the current commit.
            for tag in repo.tags:
                if tag.commit == repo.head.commit:
                    branch_or_tag = tag.name
        else:
            branch_or_tag = repo.active_branch
        id = '%s/%s@%s%s' % (
            ts.strftime('%Y%m%d-%H%M%S'),
            branch_or_tag,
            str(repo.head.commit)[:8],
            '+' if repo.is_dirty() else '')
    elif args.id != '':
        id = args.id
    if id is not None:
        bi['build_id'] = id

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
    with zipfile.ZipFile(args.output, 'w', zipfile.ZIP_DEFLATED) as zf:
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
        manifest_arc_name = os.path.join(arc_dir, FW_MANIFEST_FILE_NAME)
        zf.writestr(manifest_arc_name, json.dumps(manifest, indent=2, sort_keys=True))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='FW metadata tool', prog='fw_manifest')
    cmd = parser.add_subparsers(dest='cmd')
    gbi_cmd = cmd.add_parser('gen_build_info')
    gbi_cmd.add_argument('--timestamp', '-t')
    gbi_cmd.add_argument('--version', '-v')
    gbi_cmd.add_argument('--id', '-i')
    gbi_cmd.add_argument('--json_output')
    gbi_cmd.add_argument('--c_output')

    cm_cmd = cmd.add_parser('create_manifest')
    cm_cmd.add_argument('--name', '-n', required=True)
    cm_cmd.add_argument('--platform', '-p', required=True)
    cm_cmd.add_argument('--build_info', '-i', required=True)
    cm_cmd.add_argument('--description', '-d')
    cm_cmd.add_argument('--checksums', default='sha1')
    cm_cmd.add_argument('--src_dir')
    cm_cmd.add_argument('--output', '-o')
    cm_cmd.add_argument('parts', nargs='+')

    cf_cmd = cmd.add_parser('create_fw')
    cf_cmd.add_argument('--manifest', '-m', required=True)
    cf_cmd.add_argument('--output', '-o', required=True)
    cf_cmd.add_argument('--src_dir')

    args = parser.parse_args()
    if args.cmd == 'gen_build_info':
        cmd_gen_build_info(args)
    elif args.cmd == 'create_manifest':
        cmd_create_manifest(args)
    elif args.cmd == 'create_fw':
        cmd_create_fw(args)
