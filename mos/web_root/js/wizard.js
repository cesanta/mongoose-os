(function($) {
  var ws, tabHandlers = {};
  var defaultMqttServer = 'test.mosquitto.org:1883';
  var wssend = function(o) {
    ws.send(JSON.stringify(o));
  };
  $.ajaxSetup({ type: 'POST' });

  PNotify.prototype.options.styling = 'fontawesome';
  PNotify.prototype.options.delay = 5000;

  var getCookie = function(name) {
    var m = (document.cookie || '').match(new RegExp(name + '=([^;"\\s]+)'));
    return m ? m[1] : '';
  };

  $('a[data-toggle="tab"]')
      .on('shown.bs.tab', function() {
        var thisPane = $('.tab-pane.active').attr('id');
        if (thisPane == 'tab4') initTab4();
      });

  var getPolicies = function() {
    var region = $('#input-region').val();
    return $.ajax({url: '/policies', data: {region: region }}).then(function(json) {
      if (!json.result) return;
      $('#dropdown-policies').empty();
      $.each(json.result, function(i, v) {
        $('<li><a href="#">' + v + '</a></li>').appendTo('#dropdown-policies');
      });
    });
  };

  $(document).on('change', '#input-region', function() {
    var v = this.value;
    document.cookie = 'region=' + v;
    getPolicies();
  });

  $(document).on('change', '#input-policy', function() {
    var v = this.value;
    document.cookie = 'policy=' + v;
  });

  var initTab4 = function() {
    var choice = $('#input-cloud').val();
    var isAws = !!choice.match(/AWS/);
    var isMqtt = !!choice.match(/MQTT/);

    $('#input-mqtt').closest('.form-group').toggle(isMqtt);

    $('#input-policy').closest('.form-group').toggle(false);
    $('#input-region').closest('.form-group').toggle(false);
    $('#help-policy').toggle(false);
    $('#input-awskey').closest('.form-group').toggle(false);
    $('#input-awssecret').closest('.form-group').toggle(false);
    $('#help-aws').toggle(false);

    if (isAws) {
      $.ajax({url: '/check-aws-credentials'}).then(function(json) {
        var hasAwsCredentials = json.result;
        if (hasAwsCredentials) {
          $('#input-policy').closest('.form-group').toggle(true);
          $('#input-region').closest('.form-group').toggle(true);
          $('#help-policy').toggle(true);

          $.ajax({url: '/regions'}).then(function(json) {
            if (!json.result) return;
            $('#dropdown-regions').empty();
            $.each(json.result, function(i, v) {
              $('<li><a href="#">' + v + '</a></li>').appendTo('#dropdown-regions');
            });
            var region = $('#input-region').val();
            getPolicies();
          });
        } else {
          $('#input-awskey').closest('.form-group').toggle(true);
          $('#input-awssecret').closest('.form-group').toggle(true);
          $('#help-aws').toggle(true);
        }
      });
    }
  };

  $(document.body).on('click', '.dropdown-menu li', function(ev) {
    var text = $(this).find('a').text();
    $(this).closest('.input-group').find('input').val(text).trigger('change');
  }).on('change', '#input-cloud', initTab4);

  tabHandlers.tab1 = function() {
    var port = $('#input-serial').val();
    return $.ajax({url: '/connect', data: {port: port, reconnect: true}}).done(function(json) {
      new PNotify({ title: 'Success', text: 'Successfully connected to ' + port, type: 'success' });
      document.cookie = 'port=' + port;
    }).fail(function(err) {
      return err;
    });
  };

  tabHandlers.tab2 = function() {
    var firmware = $('#input-firmware').val();
    return $.ajax({url: '/flash', data: {firmware: firmware}}).done(function(json) {
      new PNotify({title: 'Successfully flashed ' + firmware, type: 'success' });
      document.cookie = 'firmware=' + firmware;
      return $.ajax({url: '/call', data: {method: 'Sys.GetInfo'}})
          .done(function(json) {
            var v = json.result;
            new PNotify({
              title: 'Firmware Details',
              text: 'Arch: ' + v.arch + ', ID: ' + v.fw_id + ', MAC: ' + v.mac,
              type: 'success'
            });
          });
    }).fail(function(err) {
      return err;
    });
  };

  tabHandlers.tab3 = function() {
    var ssid = $('#input-ssid').val();
    var pass = $('#input-pass').val();
    return $
        .ajax({url: '/wifi', data: {ssid: ssid, pass: pass}})
        .done(function(json) {
          new PNotify({title: 'Success', text: 'WiFi configured, IP: ' + json.result, type: 'success'});
          document.cookie = 'ssid=' + ssid + '; pass=' + pass;
        });
  };

  tabHandlers.tab4 = function() {
    var choice = $('#input-cloud').val();
    var mqtt = $('#input-mqtt').val();

    if (!!choice.match(/MQTT/)) {
      return $.ajax({url: '/call', data: {method: 'Config.Get'}}).then(function(json) {
        json.result.mqtt.server = mqtt;
        return $.ajax({url: '/call', method: 'POST', data: {method: 'Config.Set', args: JSON.stringify({config:json.result})}});
      }).then(function(json) {
        return $.ajax({url: '/call', data: {method: 'Config.Get'}});
      }).then(function(json) {
        new PNotify({title: 'Success', text: 'Cloud configured, MQTT settings: ' + JSON.stringify(json.result.mqtt, null, '  '), type: 'success'});
        document.cookie = 'mqtt=' + mqtt;
        return this;
      }).then(function(json) {
        return $.ajax({url: '/call', data: {method: 'Config.Save', args: '{"reboot":true}'}});
      });
    } else {
      var policy = $('#input-policy').val();
      var region = $('#input-region').val();
      var key = $('#input-awskey').val();
      var secret = $('#input-awssecret').val();

      if ($('#input-awskey').is(':visible')) {
        return $.ajax({url: '/aws-store-creds', data: {key: key, secret: secret}}).then(function(json) {
          new PNotify({title: 'Saved AWS IoT credentials', type: 'success'});
          initTab4();
          return this.reject();
        });
      } else {
        return $.ajax({url: '/aws-iot-setup', data: {region: region, policy: policy}}).then(function(json) {
          return $.ajax({url: '/call', data: {method: 'Config.Get'}});
        }).then(function(json) {
          new PNotify({title: 'Success', text: 'Cloud configured, MQTT settings: ' + JSON.stringify(json.result.mqtt, null, '  '), type: 'success'});
          return this;
        });
      }
      // console.log($('#input-awskey').is(':visible'), $('#input-region').is(':visible'));
      // return null;
    }
  };

  $(document).on('shown.bs.tab', 'a[data-toggle="tab"]', function(ev) {
    var paneId = $('.tab-pane.active').attr('id');
    var hasPrev = $('.tab-pane.active')[0].hasAttribute('data-prev');
    var hasNext = $('.tab-pane.active')[0].hasAttribute('data-next');
    $('#wizard-button-prev').toggleClass('hidden', !hasPrev);
    $('#wizard-button-next').toggleClass('hidden', !hasNext);
  }).on('click', '#wizard-button-next, #wizard-button-prev', function(ev) {
    var id = ev.target.id;
    var attr = id == 'wizard-button-next' ? 'data-next' : 'data-prev';
    var nextPane = $('.tab-pane.active').attr(attr);
    var thisPane = $('.tab-pane.active').attr('id');
    var t = $(ev.target).addClass('spinner').prop('disabled', true);

    var stopSpinner = function() {
      t.removeClass('spinner disable').prop('disabled', false);
    };
    var stopSpinnerAndSwitchTab = function() {
      stopSpinner();
      $('a[href="' + nextPane + '"]').trigger('click');
    };
    var stopSpinnerAndShowError = function(err) {
      stopSpinner();
      var text = err.responseJSON ? err.responseJSON.error : err.responseText;
      if (text) {
        new PNotify({title: 'Error', text: text, type: 'error'});
      }
    };

    if (id == 'wizard-button-prev') {
      stopSpinnerAndSwitchTab();
    } else if (thisPane in tabHandlers) {
      tabHandlers[thisPane]()
          .done(stopSpinnerAndSwitchTab)
          .fail(stopSpinnerAndShowError);
    }
  });

  $('#input-serial').val(getCookie('port'));
  $('#input-firmware').val(getCookie('firmware'));
  $('#input-ssid').val(getCookie('ssid'));
  $('#input-pass').val(getCookie('pass'));
  $('#input-region').val(getCookie('region'));
  $('#input-policy').val(getCookie('policy'));
  $('#input-mqtt').val(getCookie('mqtt') || defaultMqttServer);

  // Repeatedly pull list of serial ports when we're on the first tab
  setInterval(function() {
    var thisPane = $('.tab-pane.active').attr('id');
    if (thisPane != 'tab1') return;
    $.ajax({url: '/getports'}).then(function(json) {
      $('#dropdown-ports').empty();
      if (json.result && json.result.length > 0) {
        $.each(json.result, function(i, v) {
          $('<li><a href="#">' + v + '</a></li>').appendTo('#dropdown-ports');
        });
        if (!$('#input-serial').val()) {
          $('#input-serial').val(json.result[0]);
        }
        $('#noports-warning').fadeOut();
      } else {
        $('#noports-warning').fadeIn();
      }
    });
  }, 1000);

  // Let the tool know the port we want to use
  $.ajax({url: '/connect', data: {port: getCookie('port')}});

  $.get('https://mongoose-os.com/downloads/builds.json', function(data) {
    if (!data || !data.builds || !data.builds.length) return;
    console.log('JEEY', data);
    $('#dropdown-firmware').empty();
    $.each(data.builds, function(i, v) {
      $('<li><a href="#">https://mongoose-os.com/downloads/' + v + '</a></li>').appendTo('#dropdown-firmware');
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
})(jQuery);
