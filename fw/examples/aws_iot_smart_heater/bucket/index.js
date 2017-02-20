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
    ":p": {S: String(timestamp - 60)},
  },
};

dynamodb.scan(params, function (err, data) {
  if (err) {
    $("#content").text(
      "Error:\n" + JSON.stringify(err, null, '  ') + "\n\n" + JSON.stringify(err.stack, null, '  ')
    );
  } else {
    $("#content").text(JSON.stringify(data, null, '  '));
  }
});
