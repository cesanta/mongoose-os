// System Config functions
//
let Cfg = {
  _get: ffi('void *get_cfg()'),
  _get_schema: ffi('void *sys_config_schema()'),
  _find: ffi('void *mgos_conf_find_schema_entry(char *, void *)'),
  _type: ffi('int mgos_conf_value_type(void *)'),
  _str: ffi('char *mgos_conf_value_string(void *, void *)'),
  _int: ffi('int mgos_conf_value_int(void *, void *)'),
  _INT: 0,
  _BOOL: 1,
  _STR: 2,
  _OBJ: 3,

  // **`Cfg.get(path)`** - get the config value by the path
  //
  // Examples:
  //
  // `Cfg.get("device.id")` - returns a string
  // `Cfg.get("debug.level")` - returns a number
  // `Cfg.get("wifi.sta.enable")` - returns a boolean
  //
  // Returning objects are not yet supported
  get: function(path) {
    let entry = Cfg._find(path, Cfg._get_schema());
    let type = Cfg._type(entry);
    if (type === Cfg._STR) {
      return Cfg._str(Cfg._get(), entry);
    } else if (type === Cfg._INT) {
      return Cfg._int(Cfg._get(), entry);
    } else if (type === Cfg._BOOL) {
      return (Cfg._int(Cfg._get(), entry) !== 0);
    } else if (type === Cfg._OBJ) {
      /* TODO */
      return undefined;
    } else {
      /* TODO: an error */
      return undefined;
    }
  },
};

