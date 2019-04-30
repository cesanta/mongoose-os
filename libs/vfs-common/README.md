VFS subsystem multiplexes calls to libc file API methods such as open,
read, write and close between (potentially) several filesystems attached
at different mount points.

A filesystem is backed by a device which supports block reads and writes.
