function merge(_a, _b) {
  var a = JSON.parse(JSON.stringify(_a));  // make a copy
  var b = JSON.parse(JSON.stringify(_b));  // make a copy
  if (typeof(a[0]) !== 'undefined' && typeof(b[0]) !== 'undefined') {
    // Arrays
    return a.concat(b);
  }
  for (k in b) {
    if (typeof(a[k]) === 'object' && typeof(b[k]) === 'object') {
      a[k] = merge(a[k], b[k]);
    } else {
      a[k] = b[k];
    }
  }
  return a;
};

function ajax(url, callback, post_data) {
  var httpRequest = window.XMLHttpRequest ?
    new XMLHttpRequest() :
    new ActiveXObject("Microsoft.XMLHTTP");

  httpRequest.onreadystatechange = function() {
    if (httpRequest.readyState == 4) {
      var obj;
      try {
        var t = httpRequest.responseText;
        obj = t ? JSON.parse(t) : '';
        if (httpRequest.status != 200) throw(httpRequest.statusText);
        callback(obj);
      } catch(e) {
        callback(obj, e);
      }
    }
  };
  httpRequest.open(post_data !== undefined ? 'POST' : 'GET', url, true);
  httpRequest.send(post_data);
};

function getElementValue(el) {
  if (!el) return undefined;
  if (el.type == 'checkbox') return el.checked ? true : false;
  return el.value;
};

function setElementValue(el, val) {
  if (el.type == 'checkbox') el.checked = !!val;
  el.value = val;
};

function confGet(c, p) {
  if (!p) return undefined;
  var result, i;
  var parts = p.split('.');
  for (i = 0; i < parts.length && typeof(c) === 'object'; i++) {
    c = c[parts[i]];
  }
  if (i < parts.length) c = undefined;
  return c;
}

function confSet(c, p, v) {
  var result, i;
  var parts = p.split('.');
  var o, k;
  for (i = 0; i < parts.length && typeof(c) === 'object'; i++) {
    k = parts[i];
    o = c;
    c = c[k];
    if (typeof(c) === 'undefined' && i < parts.length - 1) {
      o[k] = {};
      c = o[k];
    } else if (typeof(c) !== 'object' && i < parts.length - 1) {
      return;
    }
  }
  console.debug('set', p, v);
  o[k] = v;
}

function buildConfig(schema, defaults, current) {
  var cfg = {};
  for (var i in schema) {
    var entry = schema[i];
    var ep = entry[0], et = entry[1], ed = entry[2];
    if (et === 'o') continue;  // Only process leaf nodes.
    if (ed.read_only) continue;  // No point in setting them anyway
    var defaultVal = confGet(defaults, ep);
    var currentVal = confGet(current, ep);
    var newVal = getElementValue(document.getElementById(ep));
    // console.debug(ep, defaultVal, currentVal, newVal);
    if (newVal !== undefined) {
      switch (et) {
        case 'b': newVal = !!newVal; break;
        case 's': break;  // No conversion necessary;
        case 'i': {
          newVal = parseInt(newVal);
          if (isNaN(newVal)) throw('invalid integer value for ' + ep);
          break;
        }
        default: throw('unsupported type ' + et + ' for ' + ep);
      }
      confSet(cfg, ep, newVal);
    } else if (currentVal !== undefined) {
      confSet(cfg, ep, currentVal);
    }
  }
  return cfg;
}

/*
 * Device will do this as well, this is just to give a hint to the user.
 * It should be kept in sync with mg_conf_check_access in mg_config.c.
 */
function mg_conf_check_access(key, acl) {
  if (acl === null || acl === undefined) return true;
  var entries = acl.split(',');
  for (var i = 0; i < entries.length; i++) {
    var e = entries[i];
    if (e === "") continue;
    var result = (e[0] !== '-');
    var re = e.substr(1)
        .replace(/\./g, '\\.')
        .replace(/\?/g, '.')
        .replace(/\*/g, '.*');
    var matched = ((new RegExp(re)).exec(key) !== null);
    // console.debug(' ', e, re, matched);
    if (matched) return result;
  }
  return false;
}

var rebootMsg =
    'Success. Module is rebooting, please reload this page.\n' +
    'Note: if you changed WiFi settings you may need to navigate ' +
    'your browser to new device address.';

function confReboot() {
  ajax('/reboot', function(res, err) {
    alert(err ? 'Error: ' + err : rebootMsg);
  });
}

function confSave(schema, defaults, current) {
  var c = buildConfig(schema, defaults, current);
  console.log('saving', c);

  var text = JSON.stringify(c, null, 2);
  ajax('/conf/save', function(res, err) {
    if (!err) {
      alert(rebootMsg);
    } else {
      var err_text = '';
      if (typeof res == "object") {
        err_text = (res.message ? res.message : res.status);
      } else {
        err_text = err;
      }
      alert('Error saving settings: ' + err_text);
    }
  }, text);
}

function confReset() {
  ajax('/conf/reset', function(res, err) {
    alert(err ? 'Error: ' + err : rebootMsg);
  });
}

function confLoad(loadedAll) {
  var toLoad = 5;
  var schema = [], defaults = {}, current = {};
  var vars_schema = [], vars = {};
  function loadedOne() {
    if (--toLoad == 0) {
      console.log('All data loaded');
      setTimeout(function() { loadedAll(schema, defaults, current, vars_schema, vars); }, 10);
    }
  }
  // Load configuration and update the UI
  ajax('/conf/defaults', function(obj, error) {
    console.log('Defaults loaded', (error ? ', err ' + error : obj));
    defaults = merge(defaults, obj || {});
    loadedOne();
  });
  ajax('/conf/current', function(obj, error) {
    console.log('Config loaded', (error ? ', err ' + error : obj));
    current = merge(current, obj || {});
    loadedOne();
  });
  ajax('/ro_vars', function(obj, error) {
    console.log('Vars loaded', (error ? ', err ' + error : obj));
    vars = merge(vars, obj || {});
    loadedOne();
  });
  ajax('/sys_config_schema.json', function(obj, error) {
    console.log('Sys schema loaded', (error ? ', err ' + error : obj));
    schema = merge(schema, obj || {});
    loadedOne();
  });
  ajax('/sys_ro_vars_schema.json', function(obj, error) {
    console.log('Vars schema loaded', (error ? ', err ' + error : obj));
    vars_schema = merge(vars_schema, obj || {});
    loadedOne();
  });
}
