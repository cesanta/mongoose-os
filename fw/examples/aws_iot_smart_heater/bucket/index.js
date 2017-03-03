var cognitoUser;
var mqttClientAuth;
var mqttClientUnauth;
var getStateIntervalId;
var heaterOn = false;

// Configure Cognito identity pool
AWS.config.region = heaterVars.region;
AWS.config.credentials = new AWS.CognitoIdentityCredentials({
  IdentityPoolId: heaterVars.identityPoolId,
});

var poolData = {
  UserPoolId: heaterVars.userPoolId,
  ClientId: heaterVars.userPoolClientId,
};
var userPool = new AWSCognito.CognitoIdentityServiceProvider.CognitoUserPool(poolData);

var myAnonymousAccessCreds = new AWS.Credentials({
  accessKeyId: heaterVars.anonUserAccessKeyId,
  secretAccessKey: heaterVars.anonUserSecretAccessKey,
});

var dynamodb = new AWS.DynamoDB({
  region: heaterVars.region,
  credentials: myAnonymousAccessCreds,
});

var timestamp = Math.floor(Date.now() / 1000);

var params = {
  TableName: heaterVars.tableName,
  FilterExpression: '#t > :p',
  ExpressionAttributeNames: {
    "#t": "timestamp",
  },
  ExpressionAttributeValues: {
    ":p": {S: String(timestamp - 600)},
  },
};

dynamodb.scan(params, function (err, awsData) {
  if (err) {
    $("#msg").text(
      "Error:\n" + JSON.stringify(err, null, '  ') + "\n\n" + JSON.stringify(err.stack, null, '  ')
    );
  } else {
    console.log('dynamodb data:', awsData);
    //$("#msg").text(JSON.stringify(awsData, null, '  '));

    var data = [];
    var labels = [];

    for (var i = 0; i < awsData.Items.length; i++) {
      var item = awsData.Items[i];
      data.push(Number(item.payload.M.temp.N));
      labels.push(item.payload.M.temp.N + " (timestamp: " + item.timestamp.S + ")");
    }

    var ctx = document.getElementById("myChart");
    var myChart = new Chart(ctx, {
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
    if (opts.onDisconnected) {
      opts.onDisconnected();
    }
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
    }
  };
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

function signIn(username, password, confirmationCode, eventHandler) {
  eventHandler(EV_SIGN_START);
  eventHandler(EV_STATUS, "Signing in...");

  var userData = {
    Username: username,
    Pool: userPool
  };

  cognitoUser = new AWSCognito.CognitoIdentityServiceProvider.CognitoUser(userData);

  var doSignIn = function() {
    var authenticationData = {
      Username: username,
      Password: password,
    };
    var authenticationDetails = new AWSCognito.CognitoIdentityServiceProvider.AuthenticationDetails(authenticationData);
    cognitoUser.authenticateUser(authenticationDetails, {
      onSuccess: function (result) {
        console.log('result:', result);
        console.log('access token: ' + result.getAccessToken().getJwtToken());

        console.log('You are now logged in.');

        // Add the User's Id Token to the Cognito credentials login map.
        var logins = {};
        logins['cognito-idp.' + heaterVars.region + '.amazonaws.com/' + heaterVars.userPoolId] = result.getIdToken().getJwtToken();

        AWS.config.credentials.params.Logins = logins;
        // expire the credentials so they'll be refreshed on the next request
        AWS.config.credentials.expired = true;

        // call refresh method in order to authenticate user and get new temp credentials
        AWS.config.credentials.refresh((error) => {
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
                eventHandler(EV_SIGNED_IN, {username: username});
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
                  onDisconnected: function(client) {
                    mqttClientAuth = undefined;
                    console.log('reconnecting authenticated...');
                    setTimeout(function() {
                      initClient(requestUrl, initClientAuthOpts);
                    }, 100);

                  },
                };

                initClient(requestUrl, initClientAuthOpts);
              }
            });
          }
        });
      },

      onFailure: function (err) {
        eventHandler(EV_SIGN_DONE);
        eventHandler(EV_STATUS, "Error: " + JSON.stringify(err));
        alert(err);
      },

      newPasswordRequired: function(userAttributes, requiredAttributes) {
        // User was signed up by an admin and must provide new
        // password and required attributes, if any, to complete
        // authentication.

        // the api doesn't accept this field back
        delete userAttributes.email_verified;

        var newPassword = prompt('Enter new password ', '');
        // Get these details and call
        cognitoUser.completeNewPasswordChallenge(newPassword, userAttributes, this);
      },
    });
  }

  if (confirmationCode) {
    cognitoUser.confirmRegistration(confirmationCode, true, function(err, result) {
      if (err) {
        alert(err);
        eventHandler(EV_SIGN_DONE);
        eventHandler(EV_STATUS, "Error: " + JSON.stringify(err));
        return;
      }
      console.log('call result: ' + result);
      doSignIn();
    });
  } else {
    doSignIn();
  }

}

function signUp(username, password, email, eventHandler) {
  eventHandler(EV_SIGN_START);
  eventHandler(EV_STATUS, "Signing up...");

  var attributeList = [
    new AWSCognito.CognitoIdentityServiceProvider.CognitoUserAttribute({
      Name : 'email',
      Value : email
    })
  ];

  userPool.signUp(username, password, attributeList, null, function(err, result) {
    if (err) {
      alert(err);
      eventHandler(EV_SIGN_DONE);
      eventHandler(EV_STATUS, "Error: " + JSON.stringify(err));
      return;
    }
    var cognitoUser = result.user;
    console.log('user', result.user);
    console.log('username is ' + cognitoUser.getUsername());

    openEmailCodeDialog(function(code) {
      signIn(
        cognitoUser.getUsername(), password,
        code,
        eventHandler
      );
    });
  });
}

function signOut(eventHandler) {
  eventHandler(EV_STATUS, "Signing out...");
  mqttClientAuth.disconnect();
  AWS.config.credentials.params.Logins = {};
  AWS.config.credentials.Logins = {};
  AWS.config.credentials.expired = true;
  cognitoUser.signOut();
  AWS.config.credentials.refresh((error) => {
    if (error) {
      // TODO: figure why the error arises
      console.error(error);
      //eventHandler(EV_STATUS, "Error: " + JSON.stringify(error));
      eventHandler(EV_STATUS, "");
    } else {
      eventHandler(EV_STATUS, "");
    }
    eventHandler(EV_SIGNED_OUT);
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
      $("#signup_div").hide();
      $("#signin_div").hide();
      $("#signedin_div").show();
      $("#signedin_username_span").text(data.username);
      break;
    case EV_SIGNED_OUT:
      $("#signup_div").hide();
      $("#signedin_div").hide();
      $("#signin_div").show();
      break;
    case EV_STATUS:
      $("#status").text(data);
      break;
  }
}

$(document).ready(function() {
  $("#signup_link").click(function() {
    $("#signup_div").show();
    $("#signin_div").hide();
  });

  $("#signin_link").click(function() {
    $("#signin_div").show();
    $("#signup_div").hide();
  });

  $("#signin_button").click(function() {
    signIn($("#signin_username").val(), $("#signin_password").val(), undefined, uiEventHandler);
  });

  $("#signup_button").click(function() {
    signUp(
      $("#signup_username").val(),
      $("#signup_password").val(),
      $("#signup_email").val(),
      uiEventHandler
    );
  });

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
