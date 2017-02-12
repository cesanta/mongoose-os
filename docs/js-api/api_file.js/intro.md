---
title: "File"
items:
---



 **`File.read(name)`**  - read the whole file into a string variable.
 Return value: a string contents of the file.
 If file does not exist, an empty string is returned.
 Example: read a .json configuration file into a config object:
 ```javascript
 let obj = JSON.parse(File.read('settings.json')); 
 ```



 **`File.remove(name)`** - delete file with a given name. Return value: 0
 on success, non-0 on failure.



 **`File.write(str, name, mode)`**  - write string `str` into file `name`.
 If file does not exist, it is created. `mode` is an optional file open
 mode argument, `'w'` by default, which means that previous content is
 deleted. Set `mode` to `'a'` in order to append to the existing content.
 Return value: number of bytes written.
 Example - write a configuration object into a file:
 ```javascript
 File.write(JSON.stringify(obj, 'settings.json'));
 ```

