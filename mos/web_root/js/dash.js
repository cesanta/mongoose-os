var ui = {
  connected: null,    // Whether the device is connected
  address: null,      // Serial port, or RPC address of the device
  info: null,         // Result of the Sys.GetInfo call
  pageCache: {}
};

PNotify.prototype.options.styling = 'fontawesome';
PNotify.prototype.options.delay = 5000;

var getCookie = function(name) {
  var m = (document.cookie || '').match(new RegExp(name + '=([^;"\\s]+)'));
  return m ? m[1] : '';
};

$.ajaxSetup({
  type: 'POST',
  // beforeSend: function() { if (!devConnected) return false; },
});

$(document.body).on('click', '.dropdown-menu li', function(ev) {
  var text = $(this).find('a').text();
  $(this).closest('.input-group').find('input').val(text).trigger('change');
});

$(document).ajaxSend(function(event, xhr, settings) {
  $('#top_spinner').addClass('spinner');
}).ajaxStop(function() {
  $('#top_spinner').removeClass('spinner');
}).ajaxComplete(function(event, xhr) {
  if (xhr.status == 200) return;
  var errStr = xhr.responseText || 'connection errStror';
  if (errStr.match(/deadline exceeded/)) {
    errStr = 'Lost connection with your board. Please restart mos tool from terminal';
  }
  // new PNotify({ title: 'Server Error', text: 'Reason: ' + errStr, type: 'error' });
  addLog('Server Error: ' + errStr);
});

$(document).on('click', '#reboot-button', function() {
  $.ajax({url: '/call', data: {method: 'Sys.Reboot'}}).done(function() {
    addLog('Device rebooted\n');
  });
});

$(document).on('click', '#version-update', function(ev) {
  if (!confirm('Click "OK" to start self-update. Restart mos tool when done.')) return;
  var btn = $(this);
  startSpinner(btn);
  $.ajax({url: '/update', global: false}).done(function() {
    new PNotify({title: 'Update successful. Please restart mos tool.', type: 'success'});
  }).fail(function(err) {
    var text = err.responseJSON ? err.responseJSON.error : err.responseText;
    if (text) {
      new PNotify({title: 'Error', text: text, type: 'error'});
    }
  }).always(function() {
    btn.button('reset');
  });
});

$.ajax({url: '/version'}).done(function(json) {
  if (!json.result) return;
  $('#version').text(json.result);
  $.get('https://mongoose-os.com/downloads/mos/version.json', function(data) {
    if (!data.build_id) return;
    if (data.build_id != json.result) {
      $('#version-update').removeClass('hidden');
    }
  });
});

$(document).on('click', '.clear-logs-button', function() {
  $(this).parent().parent().find('.logs').empty();
});

var connected = false;

// A page which needs to be loaded once serial port connection is established
var deferredLoadPage = undefined;

var loadPage = function(page) {
  var doit = function(html) {
    $('#app_view').html(html);
    $('#breadcrumb').html($('[data-title]').attr('data-title'));
    location.hash = page;
  };
  if (ui.pageCache[page]) {
    doit(ui.pageCache[page]);
  } else {
    $.get('page_' + page + '.html').done(function(html) {
      ui.pageCache[page] = html;
      doit(html);
    });
  }
};

$(document).on('click', 'a[tab]', function() {
  var page = $(this).attr('tab');
  loadPage(page);
});

$(document).ready(function() {
  var tab = location.hash.substring(1);
  var link = $('a[tab="' + tab + '"]');
  if (link.length == 0) link = $('a[tab]').first();
  link.click();
});

$('#app_view').resizable({
  handleSelector: ".splitter-horizontal",
  resizeWidth: false
});

$('#d1').height($(document.body).height() - 94);
$('#app_view').height($(d1).height() * 0.75);
$('#device-logs-panel').height($(d1).height() * 0.25);

var guesslang = function(filename) {
  if ((filename || '').match(/\.js$/)) return 'javascript';
  if ((filename || '').match(/\.json$/)) return 'json';
  if ((filename || '').match(/\.ya?ml$/)) return 'yaml';
  return 'text';
};
var mkeditor = function(id, lang) {
  var editor = ace.edit(id || 'editor');

  editor.$blockScrolling = Infinity;
  editor.setTheme('ace/theme/tomorrow');
  editor.setOptions({
    enableLiveAutocompletion: true,
    enableBasicAutocompletion: true,
    highlightActiveLine: false,
  });
  editor.renderer.setOptions({
    fontSize: '14px',
    fontFamily: '"SFMono-Regular", Consolas, "Liberation Mono", Menlo, Courier, monospace',
  });
  editor.session.setOptions({
    tabSize: 2,
    useSoftTabs: true,
  });
  if (lang) editor.session.setMode('ace/mode/' + lang);

  editor.scanTextForSnippets = function() {
    var text = this.getValue() || '';
    var m, re = /['"](api_\w+.js)['"]/g, snippets = [], qname = 'snippets';
    if (!this.loadedSnippets) this.loadedSnippets = {};
    var ls = this.loadedSnippets;
    var scanSnippets = function(text) {
      var snippets = [], m, re = /((?:\s*\/\/.*\n)+)/g;
      while ((m = re.exec(text)) !== null) {
        var t = m[1].replace(/(\n)\s*\/\/ ?/g, '$1').replace(/^\s+|\s+$/, '');
        var m2 = t.match(/^[^\n]+`((.+?)\(.+?)`.*?\n/);
        if (!m2) continue;
        snippets.push({
          caption: m2[2],
          snippet: m2[1],
          docHTML: marked(t),
          type: 'snippet',
        });
      }
      return snippets;
    };
    while ((m = re.exec(text)) !== null) {
      var file = m[1];

      // Ignore files that are not present on the device
      if ($('.file[rel="' + file + '"]').length === 0) continue;

      if (ls[file]) {
        Array.prototype.push.apply(snippets, ls[file]);
      } else {
        ls[file] = [];
        // Using immediate function for passing a file loop arg to the deferred
        (function(file) {
          $.ajax({url: '/get', data: { name: file }}).done(function(json) {
            ls[file] = scanSnippets(json.result || '');
          });
        })(file);
      }
    }
    Array.prototype.push.apply(snippets, scanSnippets(text));
    return snippets;
  };
  editor.completers = [{
    getCompletions: function(editor, session, pos, prefix, callback) {
     callback(null, editor.scanTextForSnippets());
    },
  }];
  // NOTE(lsm): enable this for local autocompletions
  // var lt = ace.require('ace/ext/language_tools');
  // if (lt) editor.completers.push(lt.textCompleter);

  $(document).off('click', '.ace_doc-tooltip pre');
  $(document).on('click', '.ace_doc-tooltip pre', function() {
    var text = $(this).find('code').text();
    var c = editor.getCursorPosition();
    editor.session.remove({start: {row: c.row, column: 0}, end: {row: c.row, column: 999}});
    editor.session.insert({row: c.row, column: 0}, text);
    editor.completer.detach();
  });

  return editor;
};

$(window).resize(function(ev) {
  if(this.resizeTO) clearTimeout(this.resizeTO);
  this.resizeTO = setTimeout(function() {
    location.reload();
  }, 500);
});


// Device connect code
///////////////////////////////////////////////////////////////////////////////

var startSpinner = function(btn) {
  $(btn).attr('data-loading-text', '<i class="fa fa-refresh fa-spin"></i>' + $(btn).text());
  $(btn).prop('disabled', true);
  $(btn).button('loading');
};

var formatSize = function(free, max) {
  max |= Infinity;
  var i = Math.floor(Math.log(max) / Math.log(1024));
  var tostr = function(v, i) {
    return (v / Math.pow(1024, i)).toFixed(0) * 1 + ['B', 'k', 'M', 'G', 'T'][i];
  };
  return tostr(free, i) + '/' + tostr(max, i);
};

var formatDevInfo = function(json) {
  var ip = json.wifi.sta_ip || json.wifi.ap_ip;
  var id = '', m = json.fw_id.match(/(....)(..)(..)-/);
  if (m) {
    id = moment(m[1] + '-' + m[2] + '-' + m[3]).format('MMMDD');
  }
  var link = 'n/a';
  if (ip) link = '<a target="_blank" href=http://' + ip + '>' + ip + '</a>';
  let html = '<i class="fa fa-microchip" title="Hardware architecture"></i> ' + json.arch +
              ' | <i class="fa fa-wrench" title="Build date"></i> ' + id +
              ' | <i class="fa fa-wifi" title="IP address"></i> ' + link +
              ' | <i class="fa fa-hdd-o" title="FLASH size"></i> ' + formatSize(json.fs_free || 0, json.fs_size || 0) +
              ' | <i class="fa fa-square-o" title="RAM size"></i> ' + formatSize(json.ram_free || 0, json.ram_size || 0);
  return html;
};

var updateDeviceStatus = function() {
  var classes = ['red', 'orange', 'yellow', 'green'];
  var titles = ['mo device address', 'not connected', 'connected, no IP', 'online'];
  var wifi = ui.info && ui.info.wifi && ui.info.wifi.sta_ip;
  var n = !ui.address ? 0 : !ui.connected || !ui.info ? 1 : wifi ? 3 : 2;
  console.log(n, ui.connected, ui.address, ui.info);

  $('.devconn-icon').removeClass(classes.join(' ')).addClass(classes[n] || classes[0]);
  $('.devconn-text').text(titles[n] || titles[0]);
  $('.connect-input').val(ui.address);

  // Step1
  $('#step1 .done').toggleClass('hidden', n == 0);

  // Step2
  $('#step2 a.tag').toggleClass('greyed', n == 0);
  $('#step2 .btn').prop('disabled', n == 0);
  $('#step2 .done').toggleClass('hidden', n < 2);
  if (ui.info) $('.devinfo').html(formatDevInfo(ui.info));
  $('.devinfo, #found-device-info').toggle(n > 1);

  // Step3
  $('#step3 a.tag').toggleClass('greyed', n < 2);
  $('#step3 .btn, #step3 input').prop('disabled', n < 2);
  $('#step3 .done').toggleClass('hidden', n < 3);

  $('#prototype-button').prop('disabled', n < 2);
};

var probeDevice = function() {
  return $.ajax({url: '/call', global: false, data: {method: 'Sys.GetInfo', timeout: 1}}).then(function(data) {
    ui.info = data.result;
  }).fail(function() {
    ui.info = null;
  }).always(function() {
    updateDeviceStatus();
  });
};

// Repeatedly pull list of serial ports when we're on the first tab
var checkPorts = function() {
  return $.ajax({url: '/getports', global: false}).then(function(json) {
    $('.dropdown-ports').empty();
    var result = json.result || {};
    var ports = result.Ports || [];
    var port = (result.CurrentPort || '').replace(/^serial:\/\//, '');
    if (ui.connected != result.IsConnected || ui.address != port) {
      ui.connected = result.IsConnected;
      ui.address = port;
      if (ui.connected && ui.address) probeDevice();
      if (!ui.connected) $('#splash').modal();
      updateDeviceStatus();
    }
    if (ports.length > 0) {
      $.each(ports, function(i, v) {
        $('<li><a href="#">' + v + '</a></li>').appendTo('.dropdown-ports');
      });
      $('#noports-warning').hide();
    } else {
      $('#noports-warning').fadeIn();
      $('#found-device-info').hide();
    }
  }).always(function() { setTimeout(checkPorts, 1000); });
};
checkPorts();

$(document).on('click', '.connect-button', function() {
  var btn = $(this);
  var port = btn.closest('.form').find('.connect-input').val();
  if (!port || port.match(/^\s*$/)) return;
  startSpinner(btn);
  $.ajax({url: '/connect', global: false, data: {port: port, reconnect: true}}).always(function() {
    checkPorts().always(function() {
      btn.button('reset');
    });
  });
});

$(document).on('click', '#flash-button', function() {
  var btn = $(this);
  var arch = btn.closest('.block_content').find('[name="options"]:checked').val();
  startSpinner(btn);
  $.ajax({url: '/flash', global: false, data: {firmware: arch}}).always(function() {
    setTimeout(function() {
      probeDevice().always(function() { btn.button('reset'); });
    }, 2000);
  });
});

$(document).on('click', '#wifi-button', function() {
  var ssid = $('#wifi\\.sta\\.ssid').val();
  var pass = $('#wifi\\.sta\\.pass').val();
  var btn = $(this);
  startSpinner(btn);
  $.ajax({url: '/wifi', data: {ssid: ssid, pass: pass}}).done(function() {
    document.cookie = 'ssid=' + ssid + '; pass=' + pass;
  }).always(function(json) {
    setTimeout(function() {
      probeDevice().always(function() { btn.button('reset'); });
    }, 2000);
  });
});

$(document).on('click', '#prototype-button', function() {
  $('#splash').modal('toggle');
  var currentTab = $('.side-menu li.active a').attr('tab');
  loadPage(currentTab);
});

var addLog = function(msg, type) {
  var el = type == 'uart' ? $('#device-logs') : $('#mos-logs');
  el.each(function(i, el) {
    var mustScroll = (el.scrollTop === (el.scrollHeight - el.clientHeight));
    var data = (msg || '').replace(/</g, '&lt;').replace(/>/g, '&gt;');
    el.innerHTML += data;
    if (mustScroll) el.scrollTop = el.scrollHeight;
  });
};