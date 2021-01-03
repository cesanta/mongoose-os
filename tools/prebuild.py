#!/usr/bin/env python3
#
# A utility for prebuilding binary libs and apps.
# Can publish outputs to GitHub.
#
# Main input is a YAML config file that specifies which libs/apps to build
# in which variants and what to do with them.
#
# Top object of the config files is an array, each entry is as follows:
#  location: path to git repo
#  locations: [ multiple, paths, to, repos ]
#    location, locations or both can be used.
#  variants: array of variants, each variant must specify name and platform
#            and can additionally specify build vars and c/cxx flags.
#  out: output specification(s). currently only github output type is supported.
#       if output is not specified and input looks like a github repo,
#       github output is assumed.
#
# Example config file:
#
# - locations:
#    - https://github.com/mongoose-os-apps/demo-c
#    - https://github.com/mongoose-os-apps/demo-js
#   variants:
#     - name: esp8266
#       platform: esp8266
#     - name: esp8266-1M
#       platform: esp8266
#       build_vars:
#         FLASH_SIZE: 1048576
#
# This file will build 3 variants for each of the two apps and upload artifacts
# to GitHub.

import argparse
import copy
import glob
import logging
import os
import shutil
import subprocess
import time
import yaml

import github_api

# NB: only support building from master right now.


def RunCmd(cmd):
    logging.info("  %s", " ".join(cmd))
    subprocess.check_call(cmd)


def DeleteRelease(repo, token, rel_id):
    github_api.CallReleasesAPI(
        repo, token,
        method="DELETE",
        releases_url=("/%d" % rel_id),
        decode_json=False)


def CreateGitHubRelease(spec, tag, token, tmp_dir, re_create=False):
    logging.debug("GH release spec: %s", spec)
    repo = spec["repo"]

    logging.info("  Publishing release %s / %s", repo, tag)

    # If tag already exists, make sure it points to master.
    tag_ref, ok1 = github_api.CallRefsAPI(repo, token, ("/tags/%s" % tag))
    master_ref, ok2 = github_api.CallRefsAPI(repo, token, "/heads/master")
    if ok1 and ok2 and tag_ref["object"]["sha"] != master_ref["object"]["sha"]:
        logging.info("    Updating tag %s (%s -> %s)",
                tag, tag_ref["object"]["sha"], master_ref["object"]["sha"])
        r, ok = github_api.CallRefsAPI(
            repo, token, method="PATCH",
            uri=("/tags/%s" % tag),
            json_data={
                "sha": master_ref["object"]["sha"],
                "force": False,
            })
    # If target release already exists, avoid re-creating it - simply delete previous assets.
    rel, ok = github_api.CallReleasesAPI(repo, token, releases_url=("/tags/%s" % tag))
    if ok:
        if re_create:
            logging.info("    Release already exists (id %d), deleting", rel["id"])
            DeleteRelease(repo, token, rel["id"])
            rel = None
        else:
            logging.info("    Release already exists (id %d), deleting assets", rel["id"])
            for a in rel.get("assets", []):
                logging.info("      Deleting asset %s (%d)", a["name"], a["id"])
                github_api.CallReleasesAPI(
                    repo, token,
                    method="DELETE",
                    releases_url=("/assets/%d" % a["id"]),
                    decode_json=False)
    else:
        rel = None

    if rel is None:
        logging.info("    Creating release")
        rel, ok = github_api.CallReleasesAPI(repo, token, "", method="POST", json_data={
            "name": tag,
            "draft": False,
            "tag_name": tag,
            "target_commitish": "master",
        })
        if not ok:
            logging.error("Failed to create a release draft: %s", r)
            raise RuntimeError
    rel_id = rel["id"]
    logging.debug("Release id: %d", rel_id)
    logging.info("    Uploading assets")
    for asset_name, asset_file in spec["assets"]:
        ct = "application/zip" if asset_name.endswith(".zip") else "application/octet-stream"
        logging.info("      Uploading %s to %s", asset_file, asset_name)
        with open(asset_file, "rb") as f:
            r, ok = github_api.CallReleasesAPI(
                repo, token, method="POST", subdomain="uploads", data=f,
                releases_url=("/%d/assets" % rel_id),
                headers = {"Content-Type": ct},
                params = {"name": asset_name})
        if not ok:
            logging.error("Failed to upload %s: %s", asset_name, r)
            raise RuntimeError
    logging.info("  Published release %s / %s (%d)", repo, tag, rel["id"])


def UpdateGitHubRelease(spec, tag, token, tmp_dir):
    logging.debug("GH release spec: %s", spec)
    repo = spec["repo"]

    rel, ok = github_api.CallReleasesAPI(repo, token, releases_url=("/tags/%s" % tag))
    if not ok:
        logging.error("Failed to get release info for %s/%s: %s", repo, tag, rel)
        raise RuntimeError

    logging.info("  Updating release %s / %s (%d)", repo, tag, rel["id"])
    for asset_name, asset_file in spec["assets"]:
        ct = "application/zip" if asset_name.endswith(".zip") else "application/octet-stream"
        for a in rel.get("assets", []):
            if a["name"] == asset_name:
                logging.info("    Deleting asset %s (%d)", asset_name, a["id"])
                github_api.CallReleasesAPI(
                    repo, token,
                    method="DELETE",
                    releases_url=("/assets/%d" % a["id"]),
                    decode_json=False)
        logging.info("    Uploading %s to %s", asset_file, asset_name)
        with open(asset_file, "rb") as f:
            r, ok = github_api.CallReleasesAPI(
                repo, token, method="POST", subdomain="uploads", data=f,
                releases_url=("/%d/assets" % rel["id"]),
                headers = {"Content-Type": ct},
                params = {"name": asset_name})
        if not ok:
            logging.error("Failed to upload %s: %s", asset_name, r)
            if r and r.get("errors", [{}])[0].get("code", "") == "already_exists":
                # This is a bug in GitHub where sometimes "phantom asset" will block an upload.
                # The asset is not listed (or it would've been deleted), but an uplaod will fail.
                # There is no way around it exept re-creating a release.
                # Here we'll just delete it and next run will re-create properly. Ugh.
                logging.error("*BUG* Phantom asset, nuking release")
                DeleteRelease(repo, token, rel["id"])
            raise RuntimeError

    logging.info("  Updated release %s / %s (%d)", repo, tag, rel["id"])


def MakeAsset(an, asf, tmp_dir):
    af = os.path.join(tmp_dir, an)
    logging.info("  Copying %s -> %s", asf, af)
    shutil.copy(asf, af)
    return [an, af]


def ProcessLoc(e, loc, args, created_repos):
    parts = loc.split("/")
    pre, name, i, repo_loc, repo_subdir = "", "", 0, loc, ""
    for p in parts:
        pre = name.split(":")[-1]
        name = p
        i += 1
        if p.endswith(".git"):
            repo_loc = "/".join(parts[:i])
            repo_subdir = "/".join(parts[i:])
            name = p[:-4]
            break
    repo_dir = os.path.join(args.tmp_dir, pre, name)
    if os.path.exists(repo_loc):
        rl = repo_loc + ("/" if not repo_loc.endswith("/") else "")
        os.makedirs(repo_dir, exist_ok=True)
        cmd = ["rsync", "-a", "--delete", rl, repo_dir + "/"]
        name = os.path.basename(repo_loc)
    else:
        if not os.path.exists(repo_dir):
            logging.info("Cloning into %s", repo_dir)
            cmd = ["git", "clone", repo_loc, repo_dir]
        else:
            logging.info("Pulling %s", repo_dir)
            cmd = ["git", "-C", repo_dir, "pull"]
    if repo_subdir:
        name = os.path.basename(repo_subdir)
        logging.info("== %s: %s / %s", name, repo_loc, repo_subdir)
    else:
        name = os.path.basename(repo_loc)
        logging.info("== %s: %s", name, repo_loc)
    RunCmd(cmd)
    if repo_subdir:
        tgt_dir = os.path.join(repo_dir, repo_subdir)
    else:
        tgt_dir = repo_dir
    tgt_name = os.path.split(tgt_dir)[-1]
    assets = []
    common = e.get("common", {})
    # Build all the variants, collect assets
    for v in e["variants"]:
        logging.info(" %s %s", tgt_name, v["name"])
        mos_cmd = [args.mos, "build", "-C", tgt_dir, "--local", "--clean"]
        if args.repo_dir:
            mos_cmd.append("--repo=%s" % args.repo_dir)
        if args.deps_dir:
            mos_cmd.append("--deps-dir=%s" % args.deps_dir)
        if args.binary_libs_dir:
            mos_cmd.append("--binary-libs-dir=%s" % args.binary_libs_dir)
        if args.lib:
            for lib in args.lib:
                mos_cmd.append("--lib=%s" % lib)
        if args.libs_dir:
            mos_cmd.append("--libs-dir=%s" % args.libs_dir)
        if args.no_libs_update:
            mos_cmd.append("--no-libs-update")
        mos_cmd.append("--platform=%s" % v["platform"])
        for bvk, bvv in sorted(list(common.get("build_vars", {}).items()) +
                               list(v.get("build_vars", {}).items())):
            mos_cmd.append("--build-var=%s=%s" % (bvk, bvv))
        cflags = (common.get("cflags", "") + " " + v.get("cflags", "")).strip()
        if cflags:
            mos_cmd.append("--cflags-extra=%s" % cflags)
        cxxflags = (common.get("cxxflags", "") + " " + v.get("cxxflags", "")).strip()
        if cflags:
            mos_cmd.append("--cxxflags-extra=%s" % cflags)
        mos_args = (common.get("mos_args", []) + v.get("mos_args", []))
        if mos_args:
            mos_cmd.extend(mos_args)
        RunCmd(mos_cmd)
        bl = os.path.join(args.tmp_dir, "%s-%s-build.log" % (tgt_name, v["name"]))
        logging.info("  Saving build log to %s", bl)
        shutil.copy(os.path.join(tgt_dir, "build", "build.log"), bl)
        # Ok, what did we just build?
        with open(os.path.join(tgt_dir, "mos.yml")) as f:
            m = yaml.safe_load(f)
            if m.get("type", "") == "lib":
                assets.append(MakeAsset("lib%s-%s.a" % (tgt_name, v["name"]), os.path.join(tgt_dir, "build", "lib.a"), args.tmp_dir))
            else:
                assets.append(MakeAsset("%s-%s.zip" % (tgt_name, v["name"]), os.path.join(tgt_dir, "build", "fw.zip"), args.tmp_dir))
                for fn in glob.glob(os.path.join(tgt_dir, "build", "objs", "*.elf")):
                    an = os.path.basename(fn).replace(tgt_name, "%s-%s" % (tgt_name, v["name"]))
                    assets.append(MakeAsset(an, fn, args.tmp_dir))
    outs = e.get("out", [])
    if not outs and loc.startswith("https://github.com/"):
        outs = [{"github": {"repo": "%s/%s" % (pre, tgt_name)}}]
    for out in outs:
        gh_out = copy.deepcopy(out.get("github", {}))
        # Push to GitHub
        if gh_out:
            gh_out["assets"] = assets
            gh_out["repo"] = gh_out["repo"] % {
                "name": name,
                "repo_subdir": repo_subdir,
            }

            if not args.gh_token_file:
                logging.info("Token file not set, GH uploads disabled")
                return
            if not os.path.isfile(args.gh_token_file):
                logging.error("Token file %s does not exist", args.gh_token_file)
                exit(1)
            logging.debug("Using token file at %s", args.gh_token_file)
            with open(args.gh_token_file, "r") as f:
                token = f.read().strip()
            i = 1
            while True:
                try:
                    should_update = gh_out.get("update", False)
                    # If we already (re)created this repo during this run, do not do it again.
                    if created_repos.get(gh_out["repo"]):
                        should_update = True
                    if not should_update:
                        # Looks like after some number of asset deletions / uploads GH release
                        # gets into a bad state where new asset uploads are just failing.
                        # So we try once, try twice and on the third time we re-create the release
                        # even if it exists.
                        re_create = (i > 2)
                        CreateGitHubRelease(gh_out, args.gh_release_tag, token, args.tmp_dir, re_create=re_create)
                        created_repos[gh_out["repo"]] = True
                    else:
                        UpdateGitHubRelease(gh_out, args.gh_release_tag, token, args.tmp_dir)
                    break
                except (Exception, KeyboardInterrupt) as e:
                    logging.exception("Exception (attempt %d): %s", i, e)
                    if not isinstance(e, KeyboardInterrupt) and i < 5:
                        time.sleep(1)
                        i += 1
                    else:
                        if not isinstance(e, KeyboardInterrupt) and gh_out.get("update"):
                            logging.error("*BUG* Phantom asset (probably), nuking release")
                            rel, ok = github_api.CallReleasesAPI(gh_out["repo"], token,
                                                                 releases_url=("/tags/%s" % args.gh_release_tag))
                            if ok:
                                DeleteRelease(gh_out["repo"], token, rel["id"])
                        raise


def ProcessEntry(e, args):
    created_repos = {}
    for loc in e.get("locations", []) + [e.get("location")]:
        if loc:
            ProcessLoc(e, loc, args, created_repos)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Prebuild script for apps and libs")
    parser.add_argument("--v", type=int, default=logging.INFO)
    parser.add_argument("--config", type=str, required=True)
    parser.add_argument("--tmp-dir", type=str, default=os.path.join(os.getenv("TMPDIR", "/tmp"), "mos_prebuild"))
    parser.add_argument("--deps-dir", type=str)
    parser.add_argument("--binary-libs-dir", type=str)
    parser.add_argument("--lib", type=str, action="append")
    parser.add_argument("--libs-dir", type=str)
    parser.add_argument("--repo-dir", type=str)
    parser.add_argument("--no-libs-update", action="store_true")
    parser.add_argument("--mos", type=str, default="/usr/bin/mos")
    parser.add_argument("--gh-token-file", type=str)
    parser.add_argument("--gh-release-tag", type=str)
    args = parser.parse_args()

    logging.basicConfig(level=args.v, format="[%(asctime)s %(levelno)d] %(message)s", datefmt="%Y/%m/%d %H:%M:%S")
    logging.info("Reading %s", args.config)

    with open(args.config) as f:
        cfg = yaml.safe_load(f)

    for e in cfg:
        ProcessEntry(e, args)
    logging.info("All done")
