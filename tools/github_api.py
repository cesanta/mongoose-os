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

import json

import requests  # apt-get install python3-requests || pip3 install requests

# call_api: Calls github API with the provided args. {{{
#
# url is an url like "https://api.github.com/repos/foo/bar/releases".
#
# params is a dictionary with query string params.
# json_data, if not None, will be encoded as JSON body.
# data, if not None, will be send as a body literally.
# subdomain is "api" by default, but should be set to "uploads" for file uploads.
# headers is a dictionary with headers to send. Independently of that argument,
# authentication header is always sent (with the token we've read above)
# if response is not expected to be a JSON, set decode_json to False; in this
# case, the first returned value is the response returned right from
# requests.request().
def call_api(
        token, url,
        params = {}, method = "GET", json_data = None,
        data = None, headers = {}, decode_json = True
        ):

    if token:
        if token.startswith("file:"):
            with open(token[5:], "r") as f:
                token = f.read().strip()

        headers.update({
          "Authorization": "token %s" % token,
        })

    resp = requests.request(
        method, url=url, params=params, json=json_data, headers=headers, data=data
        )

    if decode_json:
        resp_data = json.loads(resp.text)
        return (resp_data, resp.ok)
    else:
        return (resp, resp.ok)
# }}}


def CallRefsAPI(
        repo_name, token, uri,
        subdomain="api",
        params={}, method="GET", json_data=None,
        data=None, headers={}, decode_json=True):
    url = 'https://%s.github.com/repos/%s/git/refs%s' % (subdomain, repo_name, uri)
    return call_api(token, url, params=params, method=method, json_data=json_data, data=data, headers=headers, decode_json=decode_json)

# CallReleasesAPI: a wrapper for call_api which constructs the
# releases-related url.
#
# releases_url is an url part after "/repos/<repo_name>/releases".
# Where repo_name consists of two parts: "org/repo".
# releases_url should be either be empty or start with a slash.
# The rest of the arguments are the same as call_api has.
def CallReleasesAPI(
        repo_name, token, releases_url,
        subdomain = "api",
        params = {}, method = "GET", json_data = None,
        data = None, headers = {}, decode_json = True
        ):
    url = 'https://%s.github.com/repos/%s/releases%s' % (subdomain, repo_name, releases_url)
    return call_api(token, url, params=params, method=method, json_data=json_data, data=data, headers=headers, decode_json=decode_json)


def CallUsersAPI(
        org, token, users_url, params = {}, method = "GET", json_data = None, subdomain = "api",
        data = None, headers = {}, decode_json = True
        ):
    url = 'https://%s.github.com/users/%s%s' % (subdomain, org, users_url)
    return call_api(token, url, params=params, method=method, json_data=json_data, data=data, headers=headers, decode_json=decode_json)


def GetRepos(org, token):
    repos = []
    page = 1
    while True:
        # Get repos on the current "page"
        r, ok = CallUsersAPI(org, token, "/repos", params={"page": page})

        if len(r) == 0:
            # No more repos, we're done
            break

        repos += r
        page += 1

    return repos


