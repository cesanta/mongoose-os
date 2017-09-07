var ui = {
  connected: null,    // Whether the device is connected
  address: null,      // Serial port, or RPC address of the device
  info: null,         // Result of the Sys.GetInfo call
  showWizard: true,
  checkPortsTimer: null,
  checkPortsFreq: 3000,
  recentDevicesCookieName: 'recently_used',
  apiDocsURL: 'https://mongoose-os.com/downloads/api.json',
  apidocs: null,
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
  cache: false,
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
  spin(btn);
  $.ajax({url: '/update', global: false}).done(function() {
    new PNotify({title: 'Update successful. Please restart mos tool.', type: 'success'});
  }).fail(function(err) {
    var text = err.responseJSON ? err.responseJSON.error : err.responseText;
    if (text) {
      new PNotify({title: 'Error', text: text, type: 'error'});
    }
  }).always(function() {
    stopspin(btn);
  });
});

$.ajax({url: '/version'}).done(function(json) {
  if (!json.result) return;
  $('#version').text(json.result);
  $.ajax({url: '/server-version'}).done(function(jsonServer) {
    if (!jsonServer.result) return;
    if (jsonServer.result != json.result) {
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
  var doit = function() {
    $('#app_view .page').hide();
    var p = $('#app_view .page.page_' + page).show(); //fadeIn(300);
    // $('#app_view').html(html);
    $('#breadcrumb').html(p.find('[data-title]').attr('data-title'));
    location.hash = page;
  };

  if ($('#app_view .page.page_' + page).length == 0) {
    $.get('page_' + page + '.html').done(function(html) {
      $('<div class="page page_' + page + '"/>').html(html).hide().appendTo('#app_view');
      doit();
      // ui.pageCache[page] = html;
      // doit(html);
    });
  } else {
    doit();
  }

  // if (ui.pageCache[page]) {
  //   doit(ui.pageCache[page]);
  // } else {
  //   $.get('page_' + page + '.html').done(function(html) {
  //     ui.pageCache[page] = html;
  //     doit(html);
  //   });
  // }
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
  if ((filename || '').match(/\.(c|cpp|h)$/)) return 'c_cpp';
  if ((filename || '').match(/\.md$/)) return 'markdown';
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

  editor.loadSnippets = function() {
    var lang = editor.lang, snippets = [];    
    var doc = (ui.apidocs || {})[lang == 'c_cpp' ? 'c' : lang] || {};
    $.each(doc, function(k, v) {
      if (k.match(/\.h$/)) return;
      snippets.push({
        caption: k,
        snippet: k,
        docHTML: v,
        type: 'snippet',
      });
    });
    snippets.sort(function(a, b) { return a.caption - b.caption; });
    return snippets;
  };
  editor.completers = [{
    getCompletions: function(editor, session, pos, prefix, callback) {
     callback(null, editor.loadSnippets());
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

var spin = function(btn) {
  var el = $(btn).attr('orig-class', $(btn).find('.fa').attr('class'));
  el.find('.fa').attr('class', 'fa fa-refresh fa-spin')
  el.prop('disabled', true);
  return el;
};

var stopspin = function(btn) {
  $(btn).find('.fa').attr('class', btn.attr('orig-class'));
  $(btn).prop('disabled', false);
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
  var titles = ['no device address', 'not connected', 'connected, no IP', 'online'];
  var wifi = ui.info && ui.info.wifi && ui.info.wifi.sta_ip;
  var n = !ui.address ? 0 : !ui.connected || !ui.info ? 1 : wifi ? 3 : 2;

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

var addToRecentlyUsedList = function(str) {
  var list = getCookie(ui.recentDevicesCookieName).split(',');
  list.push(str);
  var unique = list.filter(function(v, i, a) { return i == a.indexOf(v); });
  document.cookie = ui.recentDevicesCookieName + '=' + unique.join(',');
};

// Setup UDP logging for Websocket-connected devices
var setUDPLog = function() {
  return $.ajax({url: '/call', global: false, data: {method: 'RPC.Ping', timeout: 5}}).then(function(data) {
    if (data && data.result && data.result.channel_info)
      var d = {method: 'Sys.SetDebug', timeout: 5, args: JSON.stringify({udp_log_addr: data.result.channel_info + ':1993'})};
      return $.ajax({url: '/call', global: false, data: d});
  });
};

var probeDevice = function() {
  return $.ajax({url: '/call', global: false, data: {method: 'Sys.GetInfo', timeout: 15}}).then(function(data) {
    ui.info = data.result;
    // Let other pages know that the device info has changed
    var infostr = JSON.stringify(ui.info);
    if (ui.infostr != infostr) {
      ui.infostr = infostr;
      addToRecentlyUsedList($('.connect-input').val());
      $(document).trigger('mos-devinfo');
      if (ui.connected && ui.address && ui.address.match(/^ws/)) setUDPLog();
    }
  }).fail(function() {
    ui.info = null;
  }).always(function() {
    updateDeviceStatus();
  });
};

// Repeatedly pull list of serial ports when we're on the first tab
var checkPorts = function() {
  return $.ajax({url: '/getports', global: false}).then(function(json) {
    $('.device-address').remove();
    var result = json.result || {};
    var ports = result.Ports || [];
    var port = (result.CurrentPort || '').replace(/^serial:\/\//, '');
    if (ui.connected != result.IsConnected || ui.address != port) {
      ui.connected = result.IsConnected;
      ui.address = port;
      if (ui.connected) {
        ui.showWizard = false;  // Dont trigger wizard dialog from this point on
        $(document).trigger('hide.bs.modal');
      }
      if (ui.connected && ui.address) probeDevice();
      if (!ui.connected && ui.showWizard) {
        ui.showWizard = false;
        $('#splash').modal();
      }
      updateDeviceStatus();
    }
    if (ports.length > 0) {
      $.each(ports.reverse(), function(i, v) {
        $('<li class="device-address"><a href="#">' + v + '</a></li>').insertAfter('.dropdown-ports .serial');
      });
      $('#noports-warning').hide();
    } else {
      $('#noports-warning').fadeIn();
      $('#found-device-info').hide();
    }
  }).always(function() {
    clearTimeout(ui.checkPortsTimer);
    ui.checkPortsTimer = setTimeout(checkPorts, ui.checkPortsFreq);
  });
};

$(document).on('click', '.connect-button', function() {
  var btn = $(this);
  var port = btn.closest('.form').find('.connect-input').val();
  if (!port || port.match(/^\s*$/)) return;
  spin(btn);
  $.ajax({url: '/connect', global: false, data: {port: port, reconnect: true}}).always(function() {
    checkPorts().always(function() {
      stopspin(btn);
    });
  });
});

$(document).on('click', '#flash-button', function() {
  var btn = $(this);
  var arch = $('.builds-input').val();
  spin(btn);
  $.ajax({url: '/flash', global: false, data: {firmware: arch}}).always(function() {
    setTimeout(function() {
      probeDevice().always(function() { stopspin(btn); });
    }, 2000);
  });
});

$(document).on('click', '#wifi-button', function() {
  var ssid = $('#wifi\\.sta\\.ssid').val();
  var pass = $('#wifi\\.sta\\.pass').val();
  var btn = $(this);
  spin(btn);
  $.ajax({url: '/wifi', data: {ssid: ssid, pass: pass}}).done(function() {
    document.cookie = 'ssid=' + ssid + '; pass=' + pass;
  }).always(function(json) {
    setTimeout(function() {
      probeDevice().always(function() { stopspin(btn); });
    }, 2000);
  });
});

$(document).on('click', '#prototype-button', function() {
  $('#splash').modal('toggle');
});

var addLog = function(msg, type) {
  var el = type == 'uart' ? $('#device-logs')[0] : $('#mos-logs')[0];
  var diff = (el.scrollHeight - (el.scrollTop + el.clientHeight));
  var mustScroll = diff <= 1;
  if (type != 'uart' && !msg.match(/.*\n$/)) msg += '\n';
  $('<span/>').text(msg || '').appendTo(el);
  if (mustScroll) el.scrollTop = el.scrollHeight;
};

// https://developer.mozilla.org/en/docs/Web/API/WindowBase64/Base64_encoding_and_decoding
function b64enc(str) {
  return btoa(
      encodeURIComponent(str).replace(/%([0-9A-F]{2})/g, function(match, p1) {
        return String.fromCharCode('0x' + p1);
      }));
};

var initRecentlyUsedDeviceAddresses = function() {
  var list = getCookie(ui.recentDevicesCookieName).split(',');
  $.each(list.reverse(), function(i, v) {
    $('<li><a href="#">' + v + '</a></li>').insertAfter('.dropdown-ports .recent');
  });
};

// Initialise device wizard address help popover
$('[data-toggle="popover"]').popover({html:true});
$(document).on('hide.bs.modal', function() {
  $('[data-toggle="popover"]').popover('hide');
  var currentTab = $('.side-menu li.active a').attr('tab');
  loadPage(currentTab);
  $('#file-refresh-button').click();
});
$(document).on('click', 'input, button', function() {
  $('[data-toggle="popover"]').popover('hide');
});

initRecentlyUsedDeviceAddresses();

// Load API documentation
$.getJSON(ui.apiDocsURL).then(function(data) {
  ui.apidocs = data;
  addLog('Loaded ' + ui.apiDocsURL, ui.apidocs);
}).fail(function() {
  addLog('Error: failed to load ' + ui.apiDocsURL);
});

// When we run first time, look at PortFlag, which is a --port mos argument.
// If a user has specified --port, then connect to a device automatically.
$.ajax({url: '/getports'}).then(function(data) {
  var r = data.result || {};
  if (!r.IsConnected && r.PortFlag != 'auto') {
    ui.showWizard = false;
    $.ajax({url: '/connect', data: {reconnect: true}}).always(function() {
      checkPorts();
    });
  } else {
    checkPorts();
  }
});

$.ajax({url: '/version-tag'}).then(function(resp) {
  var version_tag = resp.result;
  var url = 'https://mongoose-os.com/downloads/builds-' + version_tag + '.json';
  $.ajax({method: 'GET', url: url}).then(function(json) {
    $('.avail-build').remove();
    var builds = json.builds || [];
    if (builds.length > 0) {
      $.each(builds.reverse(), function(i, v) {
        $('<li class="avail-build"><a href="#">' + v + '</a></li>').insertAfter('.dropdown-builds .avail');
      });
    }
  }).catch(function(e) {
    console.log('hey', e);
  });
});
