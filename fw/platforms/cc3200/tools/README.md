# Using GDB with cc3200

- Build the project with GCC (a flag `--build-var=TOOLCHAIN:gcc` for `mos build`)
- Flash the device
- Make sure you do NOT have a wire attached to J8, and you do have a jumper there
  (i.e. undo what's suggested here http://energia.nu/cc3200guide/)
- From this directory (`fw/platforms/cc3200/tools`) invoke (using path to your elf):
  `arm-none-eabi-gdb -x gdbinit ../../../examples/c_hello/build/objs/fw.elf`

Now the target is paused, and you can hit set breakpoints, etc.

# Known issues

- Resetting the target does not work. It says that the target is going to run
  from the beginning, but in fact it just continues
- Source paths in the `fw.elf` are wrong (they are from the docker container)

# Troubleshooting

Check that openocd can work with your cc3200: `openocd -f ocd.cfg`. If you see
something like this:

```
Error: JTAG scan chain interrogation failed: all zeroes
Error: Check JTAG interface, timings, target power, etc.
Error: Trying to use configured scan chain anyway...
Error: cc3200.jrc: IR capture error; saw 0x00 not 0x01
Warn : Bypassing JTAG setup events due to errors
```

It means that you didn't remove the wire from J8.


