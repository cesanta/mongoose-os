---
title: File
---

The File API is a wrapper around standard C calls: `fopen()`, `fclose()`,
`fread()`, `fwrite()`, `rename()`, `remove()`.

- `File.open(file_name [, mode]) -> file_object or null`: Opens a file `path`.
  For a list of valid `mode` values, see `fopen()` documentation. If `mode` is
  not specified, mode `rb` is used, i.e. file is opened in read-only mode.
  Return an opened file object or null on error. Example: `var f =
  File.open('/etc/passwd'); f.close();`
- `File.read(file_name) -> string`: Reads a whole file into a string.
- `File.write(file_name, body) -> boolean`: Writes a string into a new file,
  creating one if it doesn't exist or truncating it otherwise.
- `file_obj.close() -> undefined`: Closes an opened file object.  NOTE: it is
  the user's responsibility to close all opened file streams. V7 does not do that
  automatically.
- `file_obj.read() -> string`: Reads the portion of data from an opened file stream.
  Return a string with data or an empty string on EOF or error. The portion size is
  platform dependent and cannot be changed through the File API.
- `file_obj.write(str) -> num_bytes_written`: Writes a string `str` to the opened
  file object. Returns a number of bytes written.
- `File.rename(old_name, new_name) -> errno`: Renames file `old_name` to
  `new_name`. Returns 0 on success or `errno` value on error.
- `File.remove(file_name) -> errno`: Deletes file `file_name`.  Returns 0 on
  success or `errno` value on error.
- `File.list(dir_name) -> array_of_names`: Returns a list of files in a given
  directory or `undefined` on error.

NOTE: some file systems, e.g. SPIFFS on the ESP8266 platform, are flat. They
do not support directory structure. Instead, all files reside in the
top-level directory.
