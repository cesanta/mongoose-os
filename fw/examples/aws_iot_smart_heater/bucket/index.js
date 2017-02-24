var requests = {};

// Configure Cognito identity pool
AWS.config.region = heaterVars.region;
var credsCognito = new AWS.CognitoIdentityCredentials({
  IdentityPoolId: heaterVars.identityPoolId,
});

var creds = new AWS.Credentials({
  accessKeyId: heaterVars.accessKeyId,
  secretAccessKey: heaterVars.secretAccessKey,
});

var dynamodb = new AWS.DynamoDB({
  region: heaterVars.region,
  credentials: creds,
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

function initClient(requestUrl) {
  var clientId = String(Math.random()).replace('.', '');
  var rpcId = "smart_heater_" + String(Math.random()).replace('.', '');
  var client = new Paho.MQTT.Client(requestUrl, clientId);
  var connectOptions = {
    onSuccess: function () {
      console.log('connected');

      client.subscribe(rpcId + "/rpc");

      setInterval(function() {
        var msgId = Number(String(Math.random()).replace('.', ''));
        console.log('msgId:', msgId);
        requests[msgId] = {
          handler: function(resp) {
            console.log('hey', resp);
            $("#heater_status").text("Heater status: " + (resp.on ? "on" : "off"));
          },
        };
        msgObj = {
          id: msgId,
          src: rpcId,
          dst: heaterVars.deviceId,
          method: 'Heater.GetState',
          args: {},
        };
        message = new Paho.MQTT.Message(JSON.stringify(msgObj));
        message.destinationName = heaterVars.deviceId + '/rpc';
        var sendRes = client.send(message);
        console.log(message, 'result:', sendRes);
      }, 2000);
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
        if (payload.id in requests) {
          requests[payload.id].handler(payload.result);
          delete requests[payload.id];
        }
      }
    }
  };
}

credsCognito.get(function(err) {
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
    credsCognito.accessKeyId,
    credsCognito.secretAccessKey,
    credsCognito.sessionToken
  );

  initClient(requestUrl);
});
