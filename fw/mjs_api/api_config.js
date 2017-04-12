// System Config functions
//
let Cfg = {
  _get: ffi('void *get_cfg()'),
  _desc: ffi('void *sys_config_schema()'),
  _find: ffi('void *mgos_conf_find_schema_entry(char *, void *)'),
  _type: ffi('int mgos_conf_value_type(void *)'),
  _str: ffi('char *mgos_conf_value_string(void *, void *)'),
  _int: ffi('int mgos_conf_value_int(void *, void *)'),
  _dbl: ffi('double mgos_conf_value_double(void *, void *)'),
  _INT: 0,
  _BOOL: 1,
  _DBL: 2,
  _STR: 3,
  _OBJ: 4,

  // ## **`Cfg.get(path)`** 
  // Get the config value by the configuration variable. Currently, only
  // simple types are returned: strings, ints, booleans, doubles. Objects
  // are not yet supported.
  //
  // Examples:
  // ```javascript
  // load('api_config.js');
  // Cfg.get('device.id');        // returns a string
  // Cfg.get('debug.level');      // returns an integer
  // Cfg.get('wifi.sta.enable');  // returns a boolean
  // ```
  get: function(path) {
    let entry = this._find(path, this._desc());
    let type = this._type(entry);
    let val = this._get();
    if (type === this._STR) {
      return this._str(val, entry);
    } else if (type === this._INT) {
      return this._int(val, entry);
    } else if (type === this._DBL) {
      return this._dbl(val, entry);
    } else if (type === this._BOOL) {
      return (this._int(val, entry) !== 0);
    } else if (type === this._OBJ) {
      /* TODO */
      return undefined;
    } else {
      /* TODO: an error */
      return undefined;
    }
  },
};

