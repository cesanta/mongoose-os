// File API. Exports ISO C low level API, and read/write higher level API.

let File = {
  // ISO C API
  fopen: ffi('void *fopen(char *, char *)'),
  fclose: ffi('void fclose(void *)'),
  fread: ffi('int fread(char *, int, int, void *)'),
  fwrite: ffi('int fwrite(char *, int, int, void *)'),

  // Read the whole file into a string
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

  // Write string into a file
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
};
