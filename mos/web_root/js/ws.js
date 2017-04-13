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
        case 'console':
          $('#device-logs').each(function(i, el) {
            var mustScroll = (el.scrollTop === (el.scrollHeight - el.clientHeight));
            el.innerHTML += m.data;
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

})(jQuery);
