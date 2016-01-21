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


def cmd_gen_build_info(args):
    bi = {}

    ts = datetime.datetime.utcnow()
    timestamp = None
    if args.timestamp is None:
        timestamp = ts.strftime('%Y%m%d%H%M')
    elif args.timestamp != '':
        timestamp = args.timestamp
    if timestamp is not None:
        bi['timestamp'] = timestamp

    version = None
    if args.version is None:
        version = timestamp
    elif args.version != '':
        version = args.version
    if version is not None:
        bi['version'] = version

    build = None
    if args.build is None:
        try:
            repo = git.Repo('.', search_parent_directories=True)
        except git.exc.InvalidGitRepositoryError:
            raise RuntimeError("--build is not set and CWD doesn't look like a Git repo")
        build = '%s/%s@%s%s' % (
                ts.strftime('%Y%m%d-%H%M%S'),
                repo.active_branch,
                str(repo.head.commit)[:8],
                '+' if repo.is_dirty() else '.')
    elif args.build != '':
        build = args.build
    if build is not None:
        bi['build'] = build

    if args.output:
        out = open(args.output, 'w')
    else:
        out = sys.stdout
    json.dump(bi, out, indent=2, sort_keys=True)


def cmd_create(args):
    meta = {
        'name': args.name,
        'platform': args.platform,
    }
    if args.description:
        meta['description'] = args.description

    if os.path.exists(args.build_info[0]):
        bi = json.load(open(args.build_info))
    else:
        bi = json.loads(args.build_info)
    meta.update(bi)

    for p in args.parts:
        name, attrs = p.split(':', 2)
        part = {}
        for kv in attrs.split(','):
            k, v = kv.split('=', 2)
            part[k] = v
        if args.checksums and 'src' in part:
            # TODO(rojer): Support non-local sources.
            with open(part['src']) as f:
                for algo in args.checksums.split(','):
                    h = hashlib.new(algo)
                    h.update(f.read())
                    part['cs_%s' % algo] = h.hexdigest()
        meta.setdefault('parts', {})[name] = part

    if args.output:
        out = open(args.output, 'w')
    else:
        out = sys.stdout
    json.dump(meta, out, indent=2, sort_keys=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='FW metadata tool', prog='fw_meta')
    cmd = parser.add_subparsers(dest='cmd')
    gbi_cmd = cmd.add_parser('gen_build_info')
    gbi_cmd.add_argument('--timestamp', '-t')
    gbi_cmd.add_argument('--version', '-v')
    gbi_cmd.add_argument('--build', '-b')
    gbi_cmd.add_argument('--output', '-o')

    cm_cmd = cmd.add_parser('create_meta')
    cm_cmd.add_argument('--name', '-n', required=True)
    cm_cmd.add_argument('--platform', '-p', required=True)
    cm_cmd.add_argument('--build_info', '-i', required=True)
    cm_cmd.add_argument('--description', '-d')
    cm_cmd.add_argument('--checksums', default='sha1')
    cm_cmd.add_argument('--output', '-o')
    cm_cmd.add_argument('parts', nargs='+')

    args = parser.parse_args()
    if args.cmd == 'gen_build_info':
        cmd_gen_build_info(args)
    elif args.cmd == 'create_meta':
        cmd_create(args)
