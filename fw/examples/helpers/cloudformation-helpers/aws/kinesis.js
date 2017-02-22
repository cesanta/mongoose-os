var Promise = require('bluebird'),
    AWS = require('aws-sdk'),
    base = require('lib/base'),
    kinesis = Promise.promisifyAll(new AWS.Kinesis());

// Exposes the SNS.subscribe API method
function CreateStream(event, context) {
  base.Handler.call(this, event, context);
}
CreateStream.prototype = Object.create(base.Handler.prototype);
CreateStream.prototype.handleCreate = function() {
  var p = this.event.ResourceProperties;
  delete p.ServiceToken;
  p.ShardCount = parseInt(p.ShardCount);
  return kinesis.createStreamAsync(p)
  .then(function() {
    return waitWhileStatus(p.StreamName, "CREATING");
  })
  .then(function(arn) {
    return {
      StreamName: p.StreamName,
      Arn: arn
    }
  });
}
CreateStream.prototype.handleDelete = function(referenceData) {
  var p = this.event.ResourceProperties;
  return kinesis.deleteStreamAsync({StreamName: p.StreamName})
  .then(function() {
    return waitWhileStatus(p.StreamName, "DELETING")
  })
  .catch(function(err) {
    return err;
  });
}
exports.createStream = function(event, context) {
  handler = new CreateStream(event, context);
  handler.handle();
}
// Watch until the given status is no longer the status of the stream.
function waitWhileStatus(streamName, status) {
  return Promise.try(function() {
    var validStatuses = ["CREATING", "DELETING", "ACTIVE", "UPDATING"]
    if (validStatuses.indexOf(status) >= 0) {
      return kinesis.describeStreamAsync({StreamName: streamName})
      .then(function(data) {
        console.log("Current status for [" + streamName +"]: " + data.StreamDescription.StreamStatus);
        if (data.StreamDescription.StreamStatus == status) {
          return Promise.delay(2000)
          .then(function() {
            return waitWhileStatus(streamName, status);
          });
        } else {
          return data.StreamDescription.StreamARN;
        }
      });
    } else {
      throw "status [" + status + "] not one of [" + validStatuses.join(", ") + "]";
    }
  });
}