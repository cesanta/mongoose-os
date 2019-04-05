var util = {};


util.pubsub = function() {
  var events = {};
  return {
    subscribe: function(name, fn) {
      if (!events[name]) events[name] = [];
      events[name].push(fn);
    },
    unsubscribe: function(name, fn) {
      var index = (events[name] || []).indexOf(fn);
      if (index >= 0) events[name].splice(index, 1);
    },
    publish: function(name, data) {
      (events[name] || []).forEach(function(fn) {
        fn(data);
      });
    },
  };
};

util.websocket = function(uri) {
  var l = window.location, proto = l.protocol.replace('http', 'ws');
  var wsURI = proto + '//' + l.host + l.pathname + uri;
  var wrapper = {
    shouldReconnect: true,
    close: function() {
      wrapper.shouldReconnect = false;
      wrapper.ws.close();
    },
  };
  var reconnect = function() {
    var msg, ws = new WebSocket(wsURI);
    ws.onmessage = function(ev) {
      try {
        msg = JSON.parse(ev.data);
      } catch (e) {
        console.log('Invalid ws frame:', ev.data);  // eslint-disable-line
      }
      if (msg) wrapper.onmessage(msg);
    };
    ws.onclose = function() {
      window.clearTimeout(wrapper.tid);
      if (wrapper.shouldReconnect) {
        wrapper.tid = window.setTimeout(reconnect, 1000);
      }
    };
    wrapper.ws = ws;
  };
  reconnect();
  return wrapper;
};

util.createClass = function(obj) {
  var F = function() {
    preact.Component.apply(this, arguments);
    if (obj.init) obj.init.call(this, this.props);
  };
  var p = F.prototype = Object.create(preact.Component.prototype);
  for (var i in obj) p[i] = obj[i];
  return p.constructor = F;
};

util.login = function(tok) {
  if (!tok) return;
  preactRouter.route('');
};

util.logout = function(err) {
  document.cookie = 'access_token=;expires=Thu, 01 Jan 1970 00:00:00 GMT';
  preactRouter.route('login');
};

util.errorHandler = function(error) {
  if (error.response && error.response.status == 401) {
    util.logout(error);
  } else {
    alert(
        (((error.response || {}).data || {}).error || {}).message ||
        error.message || error);
  }
};

util.copyToClipboard = function(text) {
  var el = document.createElement('input');
  el.style = 'position: absolute; left: -1000px; top: -1000px';
  el.value = text;
  document.body.appendChild(el);
  el.select();
  document.execCommand('copy');
  document.body.removeChild(el);
};

util.mkClipboardLink = function(text, title) {
  var onclick = function(ev) {
    ev.preventDefault();
    util.copyToClipboard(text);
  };
  return h(
      'a', {href: '#', onClick: onclick, tooltip: 'Click to copy to clipboard'},
      title || 'click to copy to clipboard');
};

util.map = function(arr, f) {
  var newarr = [];
  for (var i = 0; i < arr.length; i++) newarr.push(f(arr[i], i, arr));
  return newarr;
};

util.mkicon = function(icon) {
  return h('i', {class: 'mr-2 fa fa-fw ' + icon});
};

util.mkinput = function(self, k, isPassword, placeholder) {
  return h('input', {
    type: isPassword ? 'password' : 'text',
    placeholder: placeholder || '',
    onInput: function(ev) {
      self.state[k] = ev.target.value
      self.setState(self.state);
    },
    class: 'form-control form-control-sm',
    value: self.state[k],
  });
};

util.SpinButton = util.createClass({
  init: function() {
    this.state = {spin: false};
  },
  render: function(props, state) {
    var self = this;
    return h(
        'button', {
          class: 'btn btn-sm ' + (props.class || ''),
          disabled: props.disabled || state.spin,
          ref: props.ref,
          onClick: function() {
            self.setState({spin: true});
            props.onClick()
                .catch(function(err) {
                  alert(err);
                })
                .then(function() {
                  self.setState({spin: false});
                });
          }
        },
        h('i', {
          class: 'mr-1 fa fa-fw ' +
              (state.spin ? 'fa-refresh' : (props.icon || 'fa-save')) +
              (state.spin ? ' fa-spin' : '')
        }),
        props.title || 'submit');
  },
});

util.Dropdown = util.createClass({
  init: function() {
    this.state = {expanded: false};
  },
  render: function(props, state) {
    var self = this, dd = '';
    if (state.expanded) {
      dd =
          h('div', {
            class: 'position-absolute bg-white border rounded mt-1 p-2',
            style: 'right: 1em; z-index: 9',
          },
            props.children);
    }
    return h(
        'div', {class: 'position-relative d-inline-block'},
        h('div', {
          class: props.class || 'btn',
          onClick: function() {
            self.setState({expanded: !state.expanded});
          },
        },
          props.text || '', h('i', {class: 'fa fa-caret-down ml-2'})),
        dd);
  },
});


util.getCookie = function(name) {
  var v = document.cookie.match('(^|;) ?' + name + '=([^;]*)(;|$)');
  return v ? v[2] : '';
};
