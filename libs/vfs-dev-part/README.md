# A VFS device that exposes a part of another device

## Example

```c
vfs_dev_create("big0part", "{\"dev\": \"big0\", \"offset\": 32768, \"size\": 65536}");
```
