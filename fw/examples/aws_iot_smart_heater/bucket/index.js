const GRAPH_UPDATE_PERIOD = 10 /* seconds */;
const GRAPH_PERIOD = 60 * 60 * 24 * 2 /* seconds */;

var heaterOn = false;
var GoogleAuth;
var heaterUser;
var tempChart;
var lastAWSData;
var statusChanging = false;
var getShadowInterval;

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

function getDynamodbQueryParams(timestamp, period) {

  return {
    TableName: heaterVars.tableName,
    KeyConditionExpression: 'deviceid = :d AND #t > :p',
    ExpressionAttributeNames: {
      "#t": "timestamp",
    },
    ExpressionAttributeValues: {
      ":d": {S: heaterVars.deviceId},
      ":p": {S: String(timestamp - period)},
    },
  };
}

function getLabel(awsItem) {
  var date = new Date(Number(awsItem.timestamp.S));
  return awsItem.payload.M.temp.N + " (" + date + ")";
}

dynamodb.query(getDynamodbQueryParams(Math.floor(Date.now() / 1000), GRAPH_PERIOD), function (err, awsData) {
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
      if (typeof item === "object"
          && typeof item.payload === "object"
          && typeof item.payload.M === "object"
          && typeof item.payload.M.temp === "object") {
        data.push(Number(item.payload.M.temp.N));
        labels.push(getLabel(item));
      }
    }

    lastAWSData = awsData;

    var ctx = document.getElementById("tempChart");
    var color = '#f0ad4e';
    tempChart = new Chart(ctx, {
      type: 'line',
      data: {
        labels: labels,
        datasets: [{
          label: "Temperature",
          fill: false,
          lineTension: 0.1,
          backgroundColor: "rgba(75,192,192,0.4)",
          borderColor: color,
          borderCapStyle: "butt",
          borderDash: [],
          borderDashOffset: 0.0,
          borderJoinStyle: 'miter',
          borderWidth: 2,
          pointBorderColor: color,
          pointBackgroundColor: "#fff",
          pointBorderWidth: 1,
          pointHoverRadius: 3,
          pointHoverBackgroundColor: color,
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
  var curTimestamp = Date.now();
  dynamodb.query(getDynamodbQueryParams(Math.floor(curTimestamp / 1000), GRAPH_UPDATE_PERIOD), function (err, awsData) {
    if (err) {
      console.log("error querying dynamodb:", err);
    } else {
      console.log('dynamodb data:', awsData);
      var data = [];
      var labels = [];

      data = tempChart.data.datasets[0].data.splice(awsData.Items.length);
      labels = tempChart.data.labels.splice(awsData.Items.length);

      for (var i = 0; i < awsData.Items.length; i++) {
        var item = awsData.Items[i];
        data.push(Number(item.payload.M.temp.N));
        labels.push(getLabel(item));
      }

      tempChart.data.datasets[0].data = data;
      tempChart.data.labels = labels;
      tempChart.update();

      lastAWSData = awsData;
    }
  });
}, GRAPH_UPDATE_PERIOD * 1000);

function setShadowInterval() {
  if (getShadowInterval !== undefined) {
    clearInterval(getShadowInterval);
  }
  getShadowInterval = setInterval(function() {
    var iotdata = new AWS.IotData({
      endpoint: heaterVars.endpointAddress,
    });
    iotdata.getThingShadow({thingName: heaterVars.deviceId}, function (err, data) {
      if (err) {
        console.log(err, err.stack); // an error occurred
      } else {
        var payload = JSON.parse(data.payload);
        console.log("payload:", payload);
        if ("state" in payload && "reported" in payload.state) {
          var temp;
          if (lastAWSData && lastAWSData.Items && lastAWSData.Items.length > 0) {
            var item = lastAWSData.Items[lastAWSData.Items.length - 1];
            temp = Number(item.payload.M.temp.N);
          }

          var rs = payload.state.reported;
          heaterOn = !!rs.on;
          statusChanging = true;
          $('#status').bootstrapToggle(heaterOn ? 'on' : 'off');
          statusChanging = false;
          if (temp) $('#temp').text(temp.toFixed(1));
          $('#time').text(moment().format('MMM DD HH:mm:ss'));
        }
      }
    });
  }, 1000);
}

setShadowInterval();

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
function gapiLoaded() {
  if (heaterVars.googleOAuthClientId) {
    gapi.load('auth2', function() {
      var auth2 = gapi.auth2.init({
        'client_id': heaterVars.googleOAuthClientId,
      }).then((res) => {
        GoogleAuth = res;
        console.log("google auth initialized", GoogleAuth);

        var el = document.getElementById('google_signin');
        res.attachClickHandler(el, {},
          function(googleUser) {
            el.innerText = "Signed in: " + googleUser.getBasicProfile().getName();
            onSuccess(googleUser);
          }, function(error) {
            console.log(error);
          });
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
});

function signOut() {
  var eventHandler = uiEventHandler;
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

function switchHeater() {
  try {
    var iotdata = new AWS.IotData({
      endpoint: heaterVars.endpointAddress,
      credentials: AWS.config.credentials,
    });

    var updateShadowParams = {
      payload: JSON.stringify({
        state: {
          desired: {
            on: !heaterOn,
          },
        },
      }),
      thingName: heaterVars.deviceId,
    };
    iotdata.updateThingShadow(updateShadowParams, function(err, data) {
      if (err) {
        // an error occurred
        alert(err.message);
        console.log(err, err.stack);
      } else {
        // successful response
        console.log('switch success:', data);
      }
    });
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
      $("#signedout_div").addClass("hidden");
      $("#signedin_div").removeClass("hidden");
      $("#signedin_username_span").text(data.username + " via " + data.provider);
      break;
    case EV_SIGNED_OUT:
      $("#signedout_div").removeClass("hidden");
      $("#signedin_div").addClass("hidden");
      break;
    case EV_STATUS:
      $("#status_msg").text(data);
      break;
  }
}

$(document).ready(function() {
  gapiLoaded();
  var authPresent = false;
  $('#devid').text(heaterVars.deviceId);

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

  $("#status").change(function() {
    if (!statusChanging) {
      setShadowInterval();
      switchHeater();
    }
    return false;
  });
});
