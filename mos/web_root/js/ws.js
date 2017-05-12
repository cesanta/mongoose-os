(function($) {

  var reconnect = function() {
    var url = 'ws://' + location.host + '/ws';
    ws = new WebSocket(url);
    ws.onopen = function(ev) {
      // console.log(ev);
      $('#lost-connection').fadeOut(300);
    };
    ws.onclose = function(ev) {
      console.log(ev);
      $('#lost-connection').fadeIn(300);
      setTimeout(reconnect, 1000);
    };
    ws.onmessage = function(ev) {
      var m = JSON.parse(ev.data || '');
      switch (m.cmd) {
        case 'uart':
        case 'stderr':
          $('#device-logs').each(function(i, el) {
            var mustScroll = (el.scrollTop === (el.scrollHeight - el.clientHeight));
            var data = (m.data || '').replace(/</g, '&lt;').replace(/>/g, '&gt;');
            if (m.cmd === 'stderr') data = '<span class="stderr">' + data + '</span>';
            el.innerHTML += data;
            if (mustScroll) el.scrollTop = el.scrollHeight;
          });
          break;
        default:
          break;
      }
    };
    ws.onerror = function(ev) {
      console.log('error', ev);
      ws.close();
    };
  };
  reconnect();

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

  var setDeviceOnlineStatus = function(isOnline) {
    if (isOnline) {
      $('#connect-button .fa').removeClass('devoffline').addClass('devonline');
    } else {
      $('#connect-button .fa').removeClass('devonline').addClass('devoffline');
    }
  };

  var probeDevice = function() {
    $.ajax({
      url: '/call',
      timeout: 1000,
      data: {method: 'Sys.GetInfo', timeout: 1}
    }).then(function(data) {
      $('#devinfo').html(formatDevInfo(data.result));
      $('#found-device-info').fadeIn();
    }).fail(function() {
      $('#found-device-info').fadeOut();
    });
  };

  $(document).on('click', '#connect-button', function() {
    var port = $('#connect-input').val();
    if (!port || port.match(/^\s*$/)) return;
    $.ajax({url: '/connect', data: {port: port, reconnect: true}}).always(function() {
      probeDevice();
    });
  });

  // Repeatedly pull list of serial ports when we're on the first tab
  var initialized = false;
  var portList = '';
  var checkPorts = function() {
    if ($.active) return; // Do not run if there are AJAX requests in flight
    var thisPane = $('.tab-pane.active').attr('id');
    var mustRun = (thisPane == 'tab1') || $('#top_nav')[0];
    if (!mustRun) return;

    $.ajax({url: '/getports', global: false}).then(function(json) {
      $('#dropdown-ports').empty();
      var result = json.result || {};
      var ports = result.Ports || [];
      var port = (result.CurrentPort || '').replace(/^serial:\/\//, '');
      setDeviceOnlineStatus(result.IsConnected);
      $('#wizard-button-next').toggleClass('disabled', !result.IsConnected);
      if (!initialized) {
        $('#connect-input').val(port);
        if (port) probeDevice();
        initialized = true;
      }
      if (ports.length > 0) {
        $.each(ports, function(i, v) {
          $('<li><a href="#">' + v + '</a></li>').appendTo('#dropdown-ports');
        });
        $('#noports-warning').hide();
        var ports = JSON.stringify(ports);
        if (ports != portList) {
          portList = ports;
        }
      } else {
        portList = '';
        if (!$('#connect-input').data('editedManually')) $('#connect-input').val('');
        $('#noports-warning').fadeIn();
        $('#found-device-info').hide();
      }
    });
  };
  checkPorts();
  setInterval(checkPorts, 1000);

})(jQuery);
