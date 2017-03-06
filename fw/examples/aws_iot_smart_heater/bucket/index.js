const GRAPH_UPDATE_PERIOD = 10 * 1000;

var mqttClientAuth;
var mqttClientUnauth;
var getStateIntervalId;
var heaterOn = false;

var GoogleAuth;

var heaterUser;

var tempChart;

window.fbAsyncInit = function() {
  if (heaterVars.facebookOAuthClientId) {
    FB.init({
      appId      : heaterVars.facebookOAuthClientId,
      xfbml      : true,
      version    : 'v2.8'
    });
    FB.AppEvents.logPageView();

    facebookCheckLoginState();
  }
};

(function(d, s, id){
  var js, fjs = d.getElementsByTagName(s)[0];
  if (d.getElementById(id)) {return;}
  js = d.createElement(s); js.id = id;
  js.src = "//connect.facebook.net/en_US/sdk.js";
  fjs.parentNode.insertBefore(js, fjs);
}(document, 'script', 'facebook-jssdk'));

function awsCredentialsRefresh(getUsername) {
  // expire the credentials so they'll be refreshed on the next request
  AWS.config.credentials.expired = true;

  // call refresh method in order to authenticate user and get new temp credentials
  AWS.config.credentials.refresh((error) => {
    var eventHandler = uiEventHandler;
    if (error) {
      console.error(error);
      eventHandler(EV_STATUS, "Error: " + JSON.stringify(error));
      eventHandler(EV_SIGN_DONE);
    } else {
      console.log('Successfully logged!');
      console.log('identityId:', AWS.config.credentials.identityId);
      console.log('accessKeyId:', AWS.config.credentials.accessKeyId);

      eventHandler(EV_STATUS, "Attaching IoT policy...");

      //-- Call lambda to attach IoT policy
      var lambda = new AWS.Lambda({
        region: heaterVars.region,
        credentials: myAnonymousAccessCreds,
      });
      lambda.invoke({
        FunctionName: heaterVars.addIotPolicyLambdaName,
        Payload: JSON.stringify({identityId: AWS.config.credentials.identityId}),
      }, function(err, data) {
        if (err) {
          console.log('lambda err:', err, err.stack);
          eventHandler(EV_STATUS, "Error: " + JSON.stringify(err));
          eventHandler(EV_SIGN_DONE);
        } else {
          console.log('lambda ok:', data);
          eventHandler(EV_STATUS, "");
          eventHandler(EV_SIGNED_IN, {
            username: heaterUser.getUsername(),
            provider: heaterUser.getAuthProvider(),
          });
          eventHandler(EV_SIGN_DONE);

          // connect to MQTT
          var requestUrl = SigV4Utils.getSignedUrl(
            'wss',
            'data.iot.' + heaterVars.region + '.amazonaws.com',
            '/mqtt',
            'iotdevicegateway',
            heaterVars.region,
            AWS.config.credentials.accessKeyId,
            AWS.config.credentials.secretAccessKey,
            AWS.config.credentials.sessionToken
          );

          var initClientAuthOpts = {
            onConnected: function(client) {
              mqttClientAuth = client;
            },
            onDisconnected: function(reconnect) {
              mqttClientAuth = undefined;
              if (reconnect) {
                console.log('reconnecting authenticated...');
                setTimeout(function() {
                  initClient(requestUrl, initClientAuthOpts);
                }, 100);
              } else {
                console.log('do not reconnect authenticated');
              }
            },
          };

          initClient(requestUrl, initClientAuthOpts);
        }
      });
    }
  });
}

function facebookCheckLoginState() {
  console.log('facebook checking...');
  FB.getLoginStatus(function(response) {
    console.log('facebook resp:', response);

    // Check if the user logged in successfully.
    if (response.authResponse) {

      console.log('You are now logged in.');

      // Add the Facebook access token to the Cognito credentials login map.
      AWS.config.credentials.params.Logins = {
        'graph.facebook.com': response.authResponse.accessToken
      };
      awsCredentialsRefresh();

      FB.api(
        "/" + encodeURIComponent(response.authResponse.userID),
        function (userResp) {
          if (userResp && !userResp.error) {
            console.log("userResp:", userResp);

            heaterUser = {
              getUsername: () => userResp.name,
              signOut: () => {
                console.log("signing out of facebook")
                return new Promise((resolve, reject) => {
                  FB.logout(() => {
                    resolve();
                  });
                });
              },
              getAuthProvider: () => "Facebook",
            };
          }
        }
      );
    } else {
      console.log('There was a problem logging you in.');
    }
  });
}

// Configure Cognito identity pool
AWS.config.region = heaterVars.region;
AWS.config.credentials = new AWS.CognitoIdentityCredentials({
  IdentityPoolId: heaterVars.identityPoolId,
});

var myAnonymousAccessCreds = new AWS.Credentials({
  accessKeyId: heaterVars.anonUserAccessKeyId,
  secretAccessKey: heaterVars.anonUserSecretAccessKey,
});

var dynamodb = new AWS.DynamoDB({
  region: heaterVars.region,
  credentials: myAnonymousAccessCreds,
});

function getDynamodbQueryParams() {
  var timestamp = Math.floor(Date.now() / 1000);

  return {
    TableName: heaterVars.tableName,
    KeyConditionExpression: 'deviceid = :d AND #t > :p',
    ExpressionAttributeNames: {
      "#t": "timestamp",
    },
    ExpressionAttributeValues: {
      ":d": {S: heaterVars.deviceId},
      ":p": {S: String(timestamp - 600)},
    },
  };
}

dynamodb.query(getDynamodbQueryParams(), function (err, awsData) {
  if (err) {
    $("#msg").text(
      "Error:\n" + JSON.stringify(err, null, '  ') + "\n\n" + JSON.stringify(err.stack, null, '  ')
    );
  } else {
    console.log('dynamodb data:', awsData);

    var data = [];
    var labels = [];

    for (var i = 0; i < awsData.Items.length; i++) {
      var item = awsData.Items[i];
      data.push(Number(item.payload.M.temp.N));
      labels.push(item.payload.M.temp.N + " (timestamp: " + item.timestamp.S + ")");
    }

    var ctx = document.getElementById("tempChart");
    tempChart = new Chart(ctx, {
      type: 'line',
      data: {
        labels: labels,
        datasets: [{
          label: "Temperature",
          fill: false,
          lineTension: 0.1,
          backgroundColor: "rgba(75,192,192,0.4)",
          borderColor: "rgba(75,192,192,1)",
          borderCapStyle: "butt",
          borderDash: [],
          borderDashOffset: 0.0,
          borderJoinStyle: 'miter',
          borderWidth: 2,
          pointBorderColor: "rgba(75,192,192,1)",
          pointBackgroundColor: "#fff",
          pointBorderWidth: 1,
          pointHoverRadius: 3,
          pointHoverBackgroundColor: "rgba(75,192,192,1)",
          pointHoverBorderColor: "rgba(220,220,220,1)",
          pointHoverBorderWidth: 2,
          pointRadius: 0,
          pointHitRadius: 5,
          data: data,
          spanGaps: false,
        }]
      },
      options: {
        scales: {
          xAxes: [{ display: false }],
        },
        legend: { display: false },
        hover: { animationDuration: 0 },
      }
    });
  }
});

setInterval(function() {
  dynamodb.query(getDynamodbQueryParams(), function (err, awsData) {
    if (err) {
      console.log("error querying dynamodb:", err);
    } else {
      console.log('dynamodb data:', awsData);
      var data = [];
      var labels = [];

      for (var i = 0; i < awsData.Items.length; i++) {
        var item = awsData.Items[i];
        data.push(Number(item.payload.M.temp.N));
        labels.push(item.payload.M.temp.N + " (timestamp: " + item.timestamp.S + ")");
      }

      tempChart.data.datasets[0].data = data;
      tempChart.update();
    }
  });
}, GRAPH_UPDATE_PERIOD);

function initClient(requestUrl, opts) {
  var clientId = String(Math.random()).replace('.', '');
  var client = new Paho.MQTT.Client(requestUrl, clientId);
  client.myrequests = {};
  var connectOptions = {
    onSuccess: function () {
      console.log('connected');
      if (opts.onConnected) {
        opts.onConnected(client);
      }
    },
    useSSL: true,
    timeout: 3,
    mqttVersion: 4,
    onFailure: function (err) {
      console.error('connect failed', err);
    }
  };
  client.connect(connectOptions);

  client.onMessageArrived = function (message) {
    if (typeof message === "object" && typeof message.payloadString === "string") {
      console.log("msg arrived: " +  message.payloadString);
      var payload = JSON.parse(message.payloadString);
      if (typeof payload === "object" && typeof payload.id === "number") {
        if (payload.id in client.myrequests) {
          client.myrequests[payload.id].handler(payload.result);
          delete client.myrequests[payload.id];
        }
      }
    }
  };

  client.onConnectionLost = function (resp) {
    console.log('onConnectionLost, resp:', resp);
    var reconnect = false;
    if (resp.errorCode === 8 && resp.errorMessage === "AMQJS0008I Socket closed.") {
      // Error reporting in case of permission violation in MQTT is awful: when
      // the client tries to do something which is not allowed, the connection
      // gets lost with this error message, so we have to handle errors in this
      // ugly way.
      for (key in client.myrequests) {
        if (client.myrequests.hasOwnProperty(key)) {
          alert("Not authorized to publish to " + client.myrequests[key].topic);
          delete client.myrequests[key];
        }
      }
      reconnect = true;
    }
    if (opts.onDisconnected) {
      opts.onDisconnected(reconnect);
    }
  };
}

function onSuccess(googleUser) {
  console.log('Logged in as: ' + googleUser.getBasicProfile().getName());
  console.log('authResponse: ', googleUser.getAuthResponse());
  console.log('authResponse2: ', googleUser.getAuthResponse(true));
  console.log("all:", googleUser);

  gapi.client.load('oauth2', 'v2', function() {
    var request = gapi.client.oauth2.userinfo.get();
    request.execute(
      function(resp) {
        console.log('HEY resp:', resp);
        //serviceUser = resp;

        // Add the User's Id Token to the Cognito credentials login map.
        AWS.config.credentials.params.Logins = {
          'accounts.google.com': googleUser.getAuthResponse().id_token,
        };
        awsCredentialsRefresh();

        heaterUser = {
          getUsername: () => googleUser.getBasicProfile().getName(),
          signOut: () => {
            console.log("signing out of google")
            return GoogleAuth.signOut();
          },
          getAuthProvider: () => "Google",
        };
      }
    );
  });
}
function onFailure(error) {
  console.log(error);
}
function gapiLoaded() {
  if (heaterVars.googleOAuthClientId) {
    gapi.load('auth2', function() {
      gapi.auth2.init({
        'client_id': heaterVars.googleOAuthClientId,
      }).then((_GoogleAuth) => {
        GoogleAuth = _GoogleAuth;
        console.log("google auth initialized", GoogleAuth);
      });
      gapi.signin2.render('google_signin', {
        'scope': 'email',
        'width': 240,
        'height': 50,
        'longtitle': true,
        'theme': 'dark',
        'onsuccess': onSuccess,
        'onfailure': onFailure
      });
    });
  }
}

AWS.config.credentials.params.Logins = {};
AWS.config.credentials.Logins = {};
AWS.config.credentials.expired = true;

// Clearing cached ID is necessary: apparently, the IdentityId is cached
// between page reloading, so if the previous session was authenticated,
// then after the next page refresh get() will try to access the same
// IdentityId for authenticated user, and will fail with "forbidden".
AWS.config.credentials.clearCachedId();
AWS.config.credentials.get(function(err) {
  console.log("credsssss");
  console.log('after3:', AWS.config.credentials.accessKeyId);
  if(err) {
    console.log(err);
    return;
  }
  var requestUrl = SigV4Utils.getSignedUrl(
    'wss',
    'data.iot.' + heaterVars.region + '.amazonaws.com',
    '/mqtt',
    'iotdevicegateway',
    heaterVars.region,
    AWS.config.credentials.accessKeyId,
    AWS.config.credentials.secretAccessKey,
    AWS.config.credentials.sessionToken
  );

  var initClientOpts = {
    onConnected: function(client) {
      mqttClientUnauth = client;
      var rpcId = "smart_heater_" + String(Math.random()).replace('.', '');
      console.log('subscribing to:', rpcId + "/rpc");
      client.subscribe(rpcId + "/rpc");

      getStateIntervalId = setInterval(function() {
        var msgId = Number(String(Math.random()).replace('.', ''));
        var topic = heaterVars.deviceId + '/rpc/Heater.GetState'
        console.log('msgId:', msgId);
        client.myrequests[msgId] = {
          handler: function(resp) {
            console.log('hey', resp);
            heaterOn = !!resp.on;
            $("#heater_status").text("Heater status: " + (resp.on ? "on" : "off"));
          },
          topic: topic,
        };
        msgObj = {
          id: msgId,
          src: rpcId,
          dst: heaterVars.deviceId,
          args: {},
        };
        message = new Paho.MQTT.Message(JSON.stringify(msgObj));
        message.destinationName = topic;
        var sendRes = client.send(message);
        console.log(message, 'result:', sendRes);
      }, 2000);
    },

    onDisconnected: function() {
      console.log("stopping requesting state");
      mqttClientUnauth = undefined;
      clearInterval(getStateIntervalId);

      console.log('reconnecting...');
      setTimeout(function() {
        initClient(requestUrl, initClientOpts);
      }, 100);
    }
  };

  initClient(requestUrl, initClientOpts);
});

function signOut() {
  var eventHandler = uiEventHandler;
  mqttClientAuth.disconnect();
  eventHandler(EV_STATUS, "Signing out...");

  AWS.config.credentials.params.Logins = {};
  AWS.config.credentials.Logins = {};
  AWS.config.credentials.expired = true;
  AWS.config.credentials.clearCachedId();
  AWS.config.credentials.refresh((error) => {

    if (error) {
      // TODO: figure why the error arises
      console.error(error);
      //eventHandler(EV_STATUS, "Error: " + JSON.stringify(error));
    } else {
      heaterUser.signOut()
        .then(() => {
          eventHandler(EV_STATUS, "");
          eventHandler(EV_SIGNED_OUT);
          heaterUser = undefined;
        });
    }

  });
}

function switchHeater(on) {
  var client = mqttClientAuth;

  if (!client) {
    alert('please login first');
    return
  }

  try {
    var rpcId = "smart_heater_" + String(Math.random()).replace('.', '');
    console.log('subscribing to:', rpcId + "/rpc");
    client.subscribe(rpcId + "/rpc");

    var msgId = Number(String(Math.random()).replace('.', ''));
    console.log('msgId:', msgId);
    var topic = heaterVars.deviceId + '/rpc/Heater.SetState';
    client.myrequests[msgId] = {
      handler: function(resp) {
        console.log('heater switch resp:', resp);
        console.log('unsubscribing from:', rpcId + "/rpc");
        try {
          client.unsubscribe(rpcId + "/rpc");
        } catch (e) {
          console.log(e);
        }
      },
      topic: topic,
    };
    msgObj = {
      id: msgId,
      src: rpcId,
      dst: heaterVars.deviceId,
      args: {
        on: !heaterOn,
      },
    };
    message = new Paho.MQTT.Message(JSON.stringify(msgObj));
    message.destinationName = topic;
    $("#heater_status").text("Heater status: ...");
    var sendRes = client.send(message);
    console.log(message, 'result:', sendRes);
  } catch (e) {
    console.log(e);
  }
}

function enableLoginInputs(enabled) {
  $("#signin_username").prop("disabled", !enabled);
  $("#signin_password").prop("disabled", !enabled);
  $("#signup_email")   .prop("disabled", !enabled);
  $("#signup_username").prop("disabled", !enabled);
  $("#signup_password").prop("disabled", !enabled);
}

const EV_SIGN_START = "sign_start";
const EV_SIGN_DONE = "sign_done";
const EV_SIGNED_IN = "signed_in";
const EV_SIGNED_OUT = "signed_out";
const EV_STATUS = "status";

function uiEventHandler(ev, data) {
  switch (ev) {
    case EV_SIGN_START:
      enableLoginInputs(false);
      break;
    case EV_SIGN_DONE:
      enableLoginInputs(true);
      break;
    case EV_SIGNED_IN:
      $("#signedout_div").hide();
      $("#signedin_div").show();
      $("#signedin_username_span").text(data.username + " via " + data.provider);
      break;
    case EV_SIGNED_OUT:
      $("#signedout_div").show();
      $("#signedin_div").hide();
      break;
    case EV_STATUS:
      $("#status").text(data);
      break;
  }
}

$(document).ready(function() {
  var authPresent = false;

  if (heaterVars.googleOAuthClientId) {
    $('#google_signin').removeClass("hidden");
    authPresent = true;
  }

  if (heaterVars.facebookOAuthClientId) {
    $('#facebook_signin').removeClass("hidden");
    authPresent = true;
  }

  if (!authPresent) {
    $('#none_signin').removeClass("hidden");
  }

  $("#signout_button").click(function() {
    signOut(uiEventHandler);
  });

  $("#heater_switch_button").click(function() {
    switchHeater(true);
  });

});

function openEmailCodeDialog(callback) {
  var codeDialog = $("#code_dialog");
  codeDialog.dialog({
    buttons: [
      {
        id: "code-button-ok",
        text: "Ok",
        click: function() {
          callback(codeDialog.find("#email_code").val());
          $(this).dialog("close");
        },
      }
    ],
    autoOpen: false,
    modal: true,
    minWidth: 400,
    maxHeight: 300,
    title: "Enter verification code",
  });
  codeDialog.on("dialogopen", function(event, ui) {
    codeDialog.find("#email_code").val("");
  });

  codeDialog.dialog("open");
}
