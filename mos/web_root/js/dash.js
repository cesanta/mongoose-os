(function($) {
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
    $('#top_nav').addClass('spinner');
  }).ajaxStop(function() {
    $('#top_nav').removeClass('spinner');
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
    if (connected) {
      loadPage(page);
    } else {
      deferredLoadPage = page;
    }
  });

  $(document).ready(function() {
    $('a[tab]').first().click();
  });

  // Let tool know the port we want to use
  $.ajax({
    url: '/connect',
    success: function() {
      connected = true;
      // If there is a deferred page to load, load it
      if (deferredLoadPage !== undefined) {
        loadPage(deferredLoadPage);
        deferredLoadPage = undefined;
      }
    },
  });

  $('#app_view').resizable({
    handleSelector: ".splitter-horizontal",
    resizeWidth: false
  });

  $('#d1').height($(document.body).height() - 60);
  $('#app_view').height($(d1).height() * 0.75);
  $('#device-logs-panel').height($(d1).height() * 0.25);

  // Instance the tour
  var tour = new Tour({
    steps: [
    {
      element: '#device-logs',
      title: 'See device logs',
      placement: 'top',
      content: 'This panel shows device logs produced by ' +
        'JavaScript code in <code>init.js</code>.'
    },
    {
      element: '.splitter-horizontal',
      placement: 'top',
      title: 'Resize panels',
      reflex: true,
      content: 'You can resize panels by dragging this resize handle.'
    },
    {
      element: '#file-list .is_init',
      title: 'Edit init.js',
      reflex: true,
      content: 'Click on <code>init.js</code> to edit it.'
    },
    {
      element: '#file-textarea',
      title: 'Modify code',
      placement: 'left',
      content: 'Change \'Tock\' to \'Boom\''
    },
    {
      element: '#file-save-button',
      title: 'Save File',
      placement: 'bottom',
      reflex: true,
      content: 'Click "Save selected file" button to save modified file.'
    },
    {
      element: '#reboot-button',
      title: 'Reboot device',
      placement: 'top',
      reflex: true,
      content: 'Click on "Reboot device" button for device to read the new code.'
    },
    {
      element: '#device-logs',
      title: 'See modified message',
      placement: 'top',
      reflex: true,
      content: 'Notice that printed message has changed.'
    },
    {
      element: '[tab=examples]',
      title: 'See JavaScript examples apps',
      placement: 'right',
      reflex: true,
      content: 'Click on examples tab to see a list of examples we have put ' +
      'together to demonstrate the power and simplicity of Mongoose OS.'
    },
    {
      element: '.list-group',
      title: 'Click on button_mqtt.js',
      reflex: true,
      content: 'Click on <code>button_mqtt.js</code>. Click on the orange button to use that example.'
    },
  ]});
  tour.init();
  tour.start();

  $(document).on('click', '#link-tour', function() {
    tour.restart();
  });

})(jQuery);
