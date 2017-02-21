var dynamodb = new AWS.DynamoDB({
  region: heaterVars.region,
  credentials: new AWS.Credentials({
    accessKeyId: heaterVars.accessKeyId,
    secretAccessKey: heaterVars.secretAccessKey,
  }),
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
    $("#content").text(
      "Error:\n" + JSON.stringify(err, null, '  ') + "\n\n" + JSON.stringify(err.stack, null, '  ')
    );
  } else {
    //$("#content").text(JSON.stringify(awsData, null, '  '));

    var data = [];
    var labels = [];

    for (var i = 0; i < awsData.Items.length; i++) {
      var item = awsData.Items[i];
      data.push(Number(item.payload.M.temp.N));
      labels.push(item.payload.M.temp.N + " (timestamp: " + item.timestamp.S + ")");
      //labels.push(item.payload.M.temp.N);
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
