---
title: Overview
---

Mongoose OS provides a default, "system" HTTP/Websocket server, provides an API
to add custom endpoints to it. The default HTTP server also has a configuration
exposed through the system config, which could be managed either programmatically
via `cfg_get()` and `cfg_save()` calls, or manually using `mos` tool.

