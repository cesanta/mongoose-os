var ws, pageCache = {};
PNotify.prototype.options.styling = 'fontawesome';
PNotify.prototype.options.delay = 5000;
var getCookie = function(name) {
  var m = (document.cookie || '').match(new RegExp(name + '=([^;"\\s]+)'));
  return m ? m[1] : '';
};
$.ajaxSetup({type: 'POST'});

$(document.body)
    .on('click', '.dropdown-menu li', function(ev) {
      var text = $(this).find('a').text();
      $(this)
          .closest('.input-group')
          .find('input')
          .val(text)
          .trigger('change');
    });

$(document).ajaxSend(function(event, xhr, settings) {
  $('#top_spinner').addClass('spinner');
}).ajaxStop(function() {
  $('#top_spinner').removeClass('spinner');
}).ajaxComplete(function(event, xhr) {
  // $('#top_nav').removeClass('spinner');
  if (xhr.status == 200) return;
  var errStr = xhr.responseText || 'connection errStror';
  if (errStr.match(/deadline exceeded/)) {
    errStr = 'Lost connection with your board. Please restart mos tool from terminal';
  }
  new PNotify({ title: 'Server Error', text: 'Reason: ' + errStr, type: 'error' });
});

$(document)
    .on('click', '#reboot-button', function() {
      $.ajax({url: '/call', data: {method: 'Sys.Reboot'}}).done(function() {
        new PNotify({title: 'Device rebooted', type: 'success'});
      });
    });

$(document).on('click', '#clear-logs-button', function() {
  $('#device-logs').empty();
  new PNotify({title: 'Console cleared', type: 'success'});
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
  if (pageCache[page]) {
    doit(pageCache[page]);
  } else {
    $.get('page_' + page + '.html').done(function(html) {
      pageCache[page] = html;
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

$('#d1').height($(document.body).height() - 60);
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
  return editor;
};

$(window).resize(function(ev) {
  if(this.resizeTO) clearTimeout(this.resizeTO);
  this.resizeTO = setTimeout(function() {
    location.reload();
  }, 500);
});
