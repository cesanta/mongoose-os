Need to implement:
`vfs-common/src/ubuntu/ubuntu_fs.c`
`bool mgos_core_fs_init(void)`

Need to figure out:
```
/mongoose-os/fw/platforms/ubuntu/src/ubuntu_main.c:17: undefined reference to `mongoose_init'

Hints:
mongoose/include/mgos_mongoose_internal.h:enum mgos_init_result mongoose_init(void);
```
