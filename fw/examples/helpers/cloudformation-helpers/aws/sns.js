var Promise = require('bluebird'),
    AWS = require('aws-sdk'),
    base = require('lib/base'),
    sns = Promise.promisifyAll(new AWS.SNS());

// Exposes the SNS.subscribe API method
function Subscribe(event, context) {
  base.Handler.call(this, event, context);
}
Subscribe.prototype = Object.create(base.Handler.prototype);
Subscribe.prototype.handleCreate = function() {
  var p = this.event.ResourceProperties;
  return sns.subscribeAsync({
    Endpoint: p.Endpoint,
    Protocol: p.Protocol,
    TopicArn: p.TopicArn
  });
}
Subscribe.prototype.handleDelete = function(referenceData) {
  return Promise.try(function() {
    if (referenceData && referenceData.SubscriptionArn) {
      return sns.unsubscribeAsync({
        SubscriptionArn: referenceData.SubscriptionArn
      });
    }
  });
}
exports.subscribe = function(event, context) {
  handler = new Subscribe(event, context);
  handler.handle();
}
