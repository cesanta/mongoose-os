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
      addLog(m.data, m.cmd);
    };
    ws.onerror = function(ev) {
      console.log('error', ev);
      ws.close();
    };
  };
  reconnect();
})(jQuery);
