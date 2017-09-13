---
title: Filesystem
---

Mongoose OS uses a SPIFFS filesystem.
SPIFFS is a flat, i.e. it has no directories.

When firmware is built, all files from the `fs/` directory are copied
to the filesystem, plus few files that store device configuration.
