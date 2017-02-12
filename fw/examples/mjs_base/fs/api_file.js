let File = {
  // **`File.read(name)`**  - read the whole file into a string variable.
  // Return value: a string contents of the file.
  // If file does not exist, an empty string is returned.
  // Example: read a .json configuration file into a config object:
  // ```javascript
  // let obj = JSON.parse(File.read('settings.json')); 
  // ```
  read: function(path) {
    let n = 0; let res = ''; let buf = 'xxxxxxxxxx'; // Should be > 5
    let fp = File.fopen(path, 'r');
    if (fp === null) return null;
    while ((n = File.fread(buf, 1, buf.length, fp)) > 0) {
      res += buf.slice(0, n);
    }
    File.fclose(fp);
    return res;
  },

  // **`File.remove(name)`** - delete file with a given name. Return value: 0
  // on success, non-0 on failure.
  remove: ffi('int remove(char *)'),

  // **`File.write(str, name, mode)`**  - write string `str` into file `name`.
  // If file does not exist, it is created. `mode` is an optional file open
  // mode argument, `'w'` by default, which means that previous content is
  // deleted. Set `mode` to `'a'` in order to append to the existing content.
  // Return value: number of bytes written.
  // Example - write a configuration object into a file:
  // ```javascript
  // File.write(JSON.stringify(obj, 'settings.json'));
  // ```
  write: function(str, path, oMode) {
    let fp = File.fopen(path, oMode || 'w');
    if (fp === null) return 0;
    let off = 0; let tot = str.length;
    while (off < tot) {
      let len = 5;  // Use light 5-byte strings for writing
      if (off + len > tot) len = tot - off;
      let n = File.fwrite(str.slice(off, off + len), 1, len, fp);
      // if (n <= 0) break;
      off += n;
    }
    File.fclose(fp);
    return off;
  },

  fopen: ffi('void *fopen(char *, char *)'),
  fclose: ffi('void fclose(void *)'),
  fread: ffi('int fread(char *, int, int, void *)'),
  fwrite: ffi('int fwrite(char *, int, int, void *)'),
};
