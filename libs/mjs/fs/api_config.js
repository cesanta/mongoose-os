let Cfg = {
  _get: ffi('void *mgos_mjs_get_config()'),
  _set: ffi('bool mgos_config_apply(char *, bool)'),
  _desc: ffi('void *mgos_config_schema()'),
  _find: ffi('void *mgos_conf_find_schema_entry(char *, void *)'),
  _type: ffi('int mgos_conf_value_type(void *)'),
  _str: ffi('char *mgos_conf_value_string_nonnull(void *, void *)'),
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
    if (entry === null) return undefined;
    let type = this._type(entry);
    let cfg = this._get();
    if (type === this._STR) {
      return this._str(cfg, entry);
    } else if (type === this._INT) {
      return this._int(cfg, entry);
    } else if (type === this._DBL) {
      return this._dbl(cfg, entry);
    } else if (type === this._BOOL) {
      return (this._int(cfg, entry) !== 0);
    } else if (type === this._OBJ) {
      /* TODO */
      return undefined;
    } else {
      /* TODO: an error */
      return undefined;
    }
  },

  // ## **`Cfg.set(obj, opt_save)`**
  // Set the configuration. `obj` must be a subset of the whole configuation
  // tree. `save` is boolean flag that indicating whether the change should
  // be saved - it could be omitted, in which case it defaults to `true`.
  // Examples:
  // ```javascript
  // load('api_config.js');
  // Cfg.set({wifi: {ap: {enable: false}}});  // Disable WiFi AP mode
  // Cfg.set({debug: {level: 3}});            // Set debug level to 3
  // ```
  set: function(obj, save) {
    return this._set(JSON.stringify(obj), save === undefined ? true : save);
  },
};
