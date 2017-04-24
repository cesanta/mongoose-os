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
            var data = (m.data || '').replace('<', '&lt;').replace('>', '&gt;');
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

  var portEdited = false;
  $(document).on('keyup paste', '#input-serial', function() {
    portEdited = !!$('#input-serial').val();
  });

  var formatDevInfo = function(json) {
    var ip = json.wifi.sta_ip || json.wifi.ap_ip;
    var id = '', m = json.fw_id.match(/(.+?)-/);
    if (m) id = m[1];
    let html = 'arch:' + json.arch + ', built:' + id + ', ip:';
    if (ip) {
      html += '<a target="_blank" href=http://' + ip + '>' + ip + '</a>';
    } else {
      html += 'n/a';
    }
    return html;
  };

  var setDeviceOnlineStatus = function(isOnline) {
    if (isOnline) {
      $('#devconn-indicator').removeClass('devoffline').addClass('devonline');
    } else {
      $('#devconn-indicator').removeClass('devonline').addClass('devoffline');
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

  // Repeatedly pull list of serial ports when we're on the first tab
  var portList = '';
  setInterval(function() {
    var thisPane = $('.tab-pane.active').attr('id');
    var mustRun = (thisPane == 'tab1') || $('#top_nav')[0];
    if (!mustRun) return;
    $.ajax({url: '/getports', global: false}).then(function(json) {
      $('#dropdown-ports').empty();
      var result = json.result || {};
      var ports = result.Ports || [];
      var port = (result.CurrentPort || '').replace(/^serial:\/\//, '');
      setDeviceOnlineStatus(result.IsConnected);
      if (!$('#input-serial').val()) {
        $('#input-serial').val(port || ports[0] || '');
      }
      if (ports.length > 0) {
        $.each(ports, function(i, v) {
          $('<li><a href="#">' + v + '</a></li>').appendTo('#dropdown-ports');
        });
        $('#noports-warning').fadeOut();
        var ports = JSON.stringify(ports);
        if (ports != portList) {
          portList = ports;
          probeDevice();
        }
      } else {
        if (!portEdited) $('#input-serial').val('');
        $('#noports-warning').fadeIn();
        $('#found-device-info').fadeOut();
      }
    });
  }, 1000);

})(jQuery);
