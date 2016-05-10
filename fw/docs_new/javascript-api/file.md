---
title: File
---

File API is a wrapper around standard C calls `fopen()`, `fclose()`,
`fread()`, `fwrite()`, `rename()`, `remove()`.

- `File.open(file_name [, mode]) -> file_object or null`: Open a file `path`.
  For list of valid `mode` values, see `fopen()` documentation. If `mode` is
  not specified, mode `rb` is used, i.e. file is opened in read-only mode.
  Return an opened file object, or null on error. Example: `var f =
  File.open('/etc/passwd'); f.close();`
- `File.read(file_name) -> string`: Reads a whole file into a string.
- `File.write(file_name, body) -> boolean`: Writes a string into a new file,
  creating if it doesn't exist, or truncating it otherwise.
- `file_obj.close() -> undefined`: Close opened file object.  NOTE: it is
  user's responsibility to close all opened file streams. V7 does not do that
  automatically.
- `file_obj.read() -> string`: Read portion of data from an opened file stream.
  Return string with data, or empty string on EOF or error.
- `file_obj.write(str) -> num_bytes_written`: Write string `str` to the opened
  file object. Return number of bytes written.
- `File.rename(old_name, new_name) -> errno`: Rename file `old_name` to
  `new_name`. Return 0 on success, or `errno` value on error.
- `File.remove(file_name) -> errno`: Delete file `file_name`.  Return 0 on
  success, or `errno` value on error.
- `File.list(dir_name) -> array_of_names`: Return a list of files in a given
  directory, or `undefined` on error.

NOTE: some file systems, e.g. SPIFFS on ESP8266 platform, are flat. They
do not support directory structure. Instead, all files reside in the
top-level directory.
