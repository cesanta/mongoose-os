# Embedded JavaScript engine

This library brings an [mJS: restricted JavaScript-like
Engine](https://github.com/cesanta/mjs).

Apart from adding the mJS itself, the library creates a global instance of it
(available via `mgos_mjs_get_global()`), and also brings a number of
mgos-specific API files, see `fs` directory.
