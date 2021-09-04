#!/usr/bin/env python3
#
# Copyright (c) 2014-2018 Cesanta Software Limited
# All rights reserved
#
# Licensed under the Apache License, Version 2.0 (the ""License"");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an ""AS IS"" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import hashlib
import json
import multiprocessing
import os
import subprocess
import tempfile
import time

import requests  # apt-get install python3-requests || pip3 install requests

import github_api

token = ""

parser = argparse.ArgumentParser(description="")
parser.add_argument("--token_filepath", type=str, default=None, required=True)
parser.add_argument("--out_dir", type=str)
parser.add_argument("--parallelism", type=int, default=32)
parser.add_argument("--tag", type=str)
parser.add_argument("repos", type=str, nargs="*", help="Run on these repos")

args = parser.parse_args()

TOKEN = "file:%s" % args.token_filepath


def handle_repo(repo_name, tag):
    res, ok = github_api.CallReleasesAPI(repo_name, TOKEN, f"/tags/{tag}")
    if not ok:
        print(f"{repo_name}: No {tag} release")
        return

    repo_out_dir = os.path.join(args.out_dir, repo_name, tag)
    meta_file = os.path.join(repo_out_dir, ".meta.json")
    meta = {"assets": {}}
    if os.path.exists(meta_file):
        try:
            with open(meta_file) as f:
                meta = json.load(f)
        except Exception as e:
            print("Invalid meta {meta_file}: {e}")

    token = None
    if args.token_filepath:
        with open(args.token_filepath) as f:
            token = f.read().strip()

    for asset in res["assets"]:
        asset_name, asset_url, asset_mtime = asset["name"], asset["url"], asset["updated_at"]
        asset_meta = meta["assets"].get(asset_name)
        if asset_meta and asset_meta["updated_at"] == asset_mtime:
            print(f"{repo_name}: {asset_name} is up to date ({asset_mtime})")
            continue
        r = requests.get(asset_url, auth=(token, ""), headers={"Accept": "application/octet-stream"})
        if r.status_code == 200 and len(r.content) == asset["size"]:
            asset_hash = hashlib.sha256(r.content).hexdigest()
            asset_file = os.path.join(repo_out_dir, f"{asset_name}.{asset_hash}")
            print(f"{repo_name}: {asset_name} {len(r.content)} bytes, hash {asset_hash}")
            os.makedirs(os.path.dirname(asset_file), mode=0o755, exist_ok=True)
            with open(asset_file, "wb") as f:
                f.write(r.content)
            meta["assets"][asset_name] = asset
        else:
            print(f"Failed to download asset {asset_url}: {r.text}")

    meta["last_update"] = time.time()
    with open(meta_file, "w") as f:
        json.dump(meta, f, indent="  ", sort_keys=True)


# Wrappers which return an error instead of throwing it, this is for
# them to work in multithreading.pool
def handle_repo_noexc(repo_name, tag):
    try:
        handle_repo(repo_name, tag)
        return None
    except Exception as e:
        return (repo_name, str(e))

repos = args.repos

if not repos:
    print("Repos not specified")
    exit(1)

if not args.tag:
    print("Tag not specified")
    exit(1)

if not args.out_dir or not os.path.exists(args.out_dir):
    print("--out_dir not specified or does not exist")
    exit(1)

pool = multiprocessing.Pool(processes=args.parallelism)
tasks = []

# Enqueue repos which need a release to be copied, with all assets etc
for repo_name in repos:
    tasks.append((repo_name, pool.apply_async(handle_repo_noexc, [repo_name, args.tag])))

errs = []
while True:
    new_tasks = []
    for r in tasks:
        repo, res = r
        if res.ready():
            result = res.get()
            if result is not None:
                errs.append(result)
        else:
            new_tasks.append(r)
    if len(new_tasks) == 0:
        break
    else:
        print("%d active (%s%s)" % (
            len(new_tasks),
            " ".join(r[0] for r in new_tasks[:5]),
            " ..." if len(new_tasks) > 5 else ""))
        if errs:
            print("%d errors (%s)" % (len(errs), " ".join(err[0] for err in errs)))
        tasks = new_tasks
    time.sleep(2)

if len(errs) == 0:
    print("Success!")
else:
    print("------------------------------------------------------")
    print("Errors: %d" % len(errs))
    for err in errs: # Replace `None` as you need.
        print("ERROR in %s: %s" % (err[0], err[1]))
        print("---")
    exit(1)
