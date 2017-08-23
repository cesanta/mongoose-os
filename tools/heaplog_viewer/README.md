# Heaplog viewer quick overview

So typical use case for the heaplog viewer is when we get some heap integrity
violations, something like this in the log:

    heap integrity broken: block links don't match: 1110 -> 1111, but 1111 -> 0

So we enable poisoning, heaplog, and call stack trace, in order to understand
better what's going on:

    $ mos build --build-var MGOS_ENABLE_HEAP_LOG:1 --build-var MGOS_ENABLE_CALL_TRACE:1

Before flashing this firmware, make sure you'll be able to save full session's
log into a file. I usually adjust MFT `console-line-count` to be
really large, say, 50000 lines, and clear the console before flashing the
device. It will be easy to just copy-paste the whole log afterwards.
Alternatively, you may use `console-log` option of MFT to save the log to the
file for you, although you'll have to clean it yourself before each new
session.

Then, flash device and reproduce the problem. Now, instead of "heap integrity
broken" message, we should get something more concrete about the poison being
missing at some exact address, for example:

    there is no poison after the block. Expected poison address: 0x3fff3dfc,
    actual data: 0x1f 0x00

Once reproduced, copy-paste complete session's log from MFT to some file. It
should typically begin with something like:

    --- flashed successfully
    --- connected
    [some garbage omitted]
    rlǃ�hlog_param:{"heap_start":0x3fff01b0, "heap_end":0x3fffc000}

The key thing here is `hlog_param:{"heap_start":0x3fff01b0, "heap_end":0x3fffc000}`

Now, it's very useful to convert all FW addresses to symbolic names. There is a
script `heaplog_symbolize.py` for that (located in the same directory as this
readme file: `tools/heaplog_viewer`).

### Using the heap log server

Heap log server, located under the `heaplog_server` directory, simplifies the
process of collecting logs and providing symbols. Once built (you'll need to
install [Go](https://golang.org/), you can run it like this:
```
./heaplog_server --logtostderr \
  --document_root .. \
  --binary $(HOME)/cesanta/dev/fw/platforms/esp8266/.build/mongoose-os.out \
  --console_log /tmp/console.log
```

Point it at the firmware binary and console log. Console log can be a serial port
or a file that you collected before. After that, you can navigate to
http://localhost:8910/ and use the "Connect to server" button, viewer will
load the symbols and log automatically.

If the log file is a file (and not a port), the server tries to automatically find
the latest heap log start and uses it as a starting point, so you do not need to
clean up the log every time.

### Symbolizing manually

Assuming you saved a heaplog to `/tmp/log1`, a command to symbolize it would
be:

`python heaplog_symbolize.py --out-suffix _symb /tmp/log1`

It will create a new file `/tmp/log1_symb`. If you omit `--out-suffix` option,
output will go to stdout.

So now you have a heaplog with symbolic call traces. Open heaplog viewer in
your browser: `tools/heaplog_viewer/heaplog_viewer.html` (If you had it already
opened with some other data, please refresh it by hitting F5 before loading new
file, since viewer is not yet polished)

Click "Choose File", and select the file with log you just saved. After a few
seconds, you should see a visual heap map.

NOTE: If the map doesn't appear, you log file might be corrupted, or you might
forget to hit F5 before loading new heaplog file. To check if there are some
errors, open developers console (in Opera or Chrome, it's Ctrl+Shift+J), and
check if there are any uncaught exceptions. If help is needed, ping `dfrank` in
Slack.

So now you have a visual heap map and an address where poison is missing
(in the example above, it's 0x3fff3dfc). By moving your mouse over the map,
you'll see corresponding addresses. This way it's easy to find the offending
address on the map; usually, it's the end of some block. Now we know that this
particular block was overflown. In the details, there will be call stack like
the following:

```
#4849 Alloc: 0x3fff70ac, size: 1024, shim: 1, calls: __wrap_pvPortMalloc ←
malloc ← _malloc_r ← cs_read_file ← init_device
```

Having that information now, it's much easier for you to find a bug. Good luck!

### Shortening the log

If the log is too long to be parsed by the heaplog viewer, there is a
standalone shortener script which takes a heaplog, parses it, and swallows all
the history, leaving just a set of `malloc`s which reconstruct the final
picture from the log. Usage:

    $ cd tools/heaplog_viewer/heaplog_shortener && \
      go build && \
      ./heaplog_shortener --console_log /path/to/src_log > target_short_log
