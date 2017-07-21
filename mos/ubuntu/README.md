# Building Ubuntu packages

We have a PPA on Launchpad called [mongoose-os](https://launchpad.net/~mongoose-os/+archive/ubuntu/mos).

PPAs are cool in that they free us from the need of hosting a Debian repo, but they are restrictive too: binary packages must be built from a source deb on their build machines.
This is fine because all of `mos` is open source anyway, but it restricts things we can do, like for example this is the reason why mos must be buildable with Go 1.6 (version bundled with 16.04 Xenial, which is the current LTS at the time of writing).

Long story short, we managed to get it going, and here's how to do it, followed by the nitty-gritty if you are curious and/or want to understand how things work or why they don't.

## How to do it

 * Get a hold of the `Cesanta Bot`'s GPG keys (`/data/.gnupg-cesantabot @ secure`), put it somewhere on your machine. Let's say, `~/.gnupg-cesantabot`.
 * Get a clone of `cesanta/mongoose-os`. `NB: `cesanta/dev` will not do`
   * No, really, it won't. May look like it should, but it won't. Use the public repo.
   * Even if it does you should not do it, because all of it is archived in the source .deb and we don't want that.
 * Set the `DISTR` variable for use in the commands below: `DISTR=xenial`
 * From the repo dir, run:
```
[rojer@nbmbp ~/cesanta/mongoose-os master]$ docker run -i -t --rm -v $PWD:/src -v /tmp/out-$DISTR:/tmp/work docker.cesanta.com/ubuntu-golang:$DISTR \
    /bin/bash -l -c "cd /src && rm -rf /tmp/work/* && git-build-recipe --allow-fallback-to-native --package mos-latest --distribution $DISTR /src/mos/ubuntu/mos-latest-$DISTR.recipe /tmp/work && \
    cd /tmp/work/mos-latest && debuild --no-tgz-check -us -uc -b"
...
   dh_builddeb -u-Zxz -O--buildsystem=golang
dpkg-deb: building package 'mos-latest' in '../mos-latest_201707202349+c65c499d-zesty0_amd64.deb'.
 dpkg-genchanges --build=any,all >../mos-latest_201707202349+c65c499d-zesty0_amd64.changes
dpkg-genchanges: info: binary-only upload (no source code included)
 dpkg-source --after-build mos-latest
dpkg-buildpackage: info: binary-only upload (no source included)
Now running lintian...
warning: the authors of lintian do not recommend running it with root privileges!
W: mos-latest: binary-without-manpage usr/bin/mos
Finished running lintian.
```
   * The output directory should look like this:
```
[rojer@nbmbp ~/cesanta/mongoose-os master]$ ls -la /tmp/out-$DISTR
total 15960
drwxrwxrwx  4 root root     4096 Jul 21 02:53 .
drwxrwxrwt 35 root root    94208 Jul 21 02:51 ..
drwx------  3 root root     4096 Jul 20 10:22 .cache
drwxr-xr-x 14 root root     4096 Jul 21 02:53 mos-latest
-rw-r--r--  1 root root     7143 Jul 21 02:53 mos-latest_201707202349+c65c499-xenial0_amd64.build
-rw-r--r--  1 root root      785 Jul 21 02:53 mos-latest_201707202349+c65c499-xenial0_amd64.changes
-rw-r--r--  1 root root  3691710 Jul 21 02:53 mos-latest_201707202349+c65c499-xenial0_amd64.deb
-rw-r--r--  1 root root      815 Jul 21 02:53 mos-latest_201707202349+c65c499-xenial0.dsc
-rw-r--r--  1 root root     1456 Jul 21 02:53 mos-latest_201707202349+c65c499-xenial0_source.build
-rw-r--r--  1 root root     1081 Jul 21 02:53 mos-latest_201707202349+c65c499-xenial0_source.changes
-rw-r--r--  1 root root 12511684 Jul 21 02:53 mos-latest_201707202349+c65c499-xenial0.tar.xz
```
   * (optional) Try installing the binary package: `sudo dpkg -i /tmp/out-$DISTR/*.deb`
 * If everything checks out, it's time to upload to the PPA.
   * Sign the source package: `[rojer@nbmbp ~/cesanta/mongoose-os master]$ docker run -it --rm -v $HOME/.gnupg-cesantabot:/root/.gnupg -v /tmp/out-$DISTR:/work docker.cesanta.com/ubuntu-golang:yakkety /bin/bash -l -c "cd /work; debsign *_source.changes"
`
     * You will be asked for a passphrase for the keyring. You should know who to ask.
     * Note: always use `:yakkety` here, `zesty` has trouble with passphrase prompt and doesnt work for some reason (`clearsign failed: Inappropriate ioctl for device`).
   * Upload the source deb to PPA for building:
```
[rojer@nbmbp /tmp/out-xenial ]$ docker run -it --rm -v $HOME/.gnupg-cesantabot:/root/.gnupg -v /tmp/out-$DISTR:/work docker.cesanta.com/ubuntu-golang:$DISTR /bin/bash -l -c "cd /work; dput ppa:mongoose-os/mos *_source.changes"
...
Good signature on /work/mos-latest_201707202349+c65c499-xenial0.dsc.
Uploading to ppa (via ftp to ppa.launchpad.net):
  Uploading mos-latest_201707202349+c65c499-xenial0.dsc: done.
  Uploading mos-latest_201707202349+c65c499-xenial0.tar.xz: done.
  Uploading mos-latest_201707202349+c65c499-xenial0_source.changes: done.
Successfully uploaded packages.
```
   * Repeat for `DISTR=yakkety` and `DISTR=zesty`.

Shortly after upload the package should be queued for building [here](https://launchpad.net/~mongoose-os/+archive/ubuntu/mos/+builds?build_text=&build_state=all) and once finished, will appear in the [package list](https://launchpad.net/~mongoose-os/+archive/ubuntu/mos/+packages).

## How it works

  * [git-build-recipe](https://launchpad.net/git-build-recipe) is used to prepare the source package.
    * The recipes are [here](https://github.com/cesanta/mongoose-os/tree/master/mos/ubuntu). They are identical except for the distro name (I wish they weren't, [maybe some day](https://bugs.launchpad.net/git-build-recipe/+bug/1705591)).
    * The recipe specifies:
      * Clone `/src`, which is a volume-mount into the container and must be a clone of the `cesanta/mongoose-os` repo.
      * Merge in a `deb-latest` branch, which is a separate branch with Debian build metadata and a couple symlinks (see [here](https://github.com/cesanta/mongoose-os/tree/deb-latest)). It's branched off really early so there should never be any conflicts.
      * Pull in all the vendored packages using `govendor`. This will be necessary later, when building the binary.
    * `git-build-recipe` then tweaks Debian metadata to set version and create an automatic changelog entry and creates a source deb.
  * Building the binary package
    * As mentioned, this must be doable by a remote builder which knows nothing about Docker and other useful things.
    * Go packages that do not use any external dependencies that are not packaged for the distro can be built magically with [dh-golang](https://pkg-go.alioth.debian.org/packaging.html). Unfortunately, `mos` does have external dependencies, so we perform [an elaborate dance](https://github.com/cesanta/mongoose-os/blob/deb-latest/debian/rules#L11) to prepare `GOPATH` for the build.
      * We basically construct a `$GOPATH/src` with a single `cesanta.com` package (fortunately, a symlink is enough). `cesanta.com` package will have a `vendor` dir with all the dependencies (synced while building the source package).
