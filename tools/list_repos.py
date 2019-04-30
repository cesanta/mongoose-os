#!/usr/bin/env python3
#
# Copyright (c) 2014-2019 Cesanta Software Limited
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

import github_api

parser = argparse.ArgumentParser(description="")
parser.add_argument("--org", required=True)

args = parser.parse_args()

repos = github_api.GetRepos(args.org, None)

for r in sorted(repos, key=lambda e: e["name"]):
    dp, l = [], 0
    for p in (r["description"] or "").split():
        if l + len(p) >= 90:
            dp.append("...")
            break
        l += len(p)
        dp.append(p)
    descr = " ".join(dp)
    print("| [%s](%s) | %s |" % (r["name"], r["html_url"], descr))
