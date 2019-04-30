# File-logger

File-logger is a library which implements log file rotation: it maintains max X
log files of max size Y, so that you always have latest logs from the device
persisted on the filesystem. By default there are max 10 files, prefixed with
`log_`, each of max size 5000 bytes.

See `mos.yml` for the possible options. At least you'd have to
enable this lib in your app's `mos.yml`, like this:

```yaml
libs:
  - origin: https://github.com/cesanta/mongoose-os/libs/file-logger

config_schema:
  - ["file_logger.enable", true]
```
