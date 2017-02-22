// Implement this class for every new handler.

var Promise = require('bluebird'),
    helpers = require('lib/helpers'),
    response = require('lib/cfn-response'),
    AWS = require('aws-sdk'),
    dynamoDB = Promise.promisifyAll(new AWS.DynamoDB());

exports.Handler = function(event, context) {
  this.event = event;
  this.context = context;
}

exports.Handler.prototype.handle = function() {
  var outer = this;
  Promise.try(function() {
    switch (outer.event.RequestType) {
      case 'Create':
        return outer.handleCreate()
        .then(function(data) {
          return outer.setReferenceData(data)
          .then(function() {
            return data;
          });
        });
      case 'Delete':
        return outer.getReferenceData()
        .then(function(data) {
          return outer.handleDelete(data);
        });
      case 'Update':
        return outer.getReferenceData()
        .then(function(data) {
          return outer.handleUpdate();
        });
      default:
        throw "Unrecognized RequestType [" + outer.event.RequestType + "]";
    }
  })
  .then(function(data) {
    response.send(outer.event, outer.context, response.SUCCESS, data);
  })
  .catch(function(err) {
    outer.error(err);
  });
}

/*
  When implemented, these should all return a Promise, which will then be completed by the handle()
  method above.

  NB: These methods are named 'handle*' because 'delete' is a reserved word in Javascript and
      can't be overridden. To ensure naming parity, they have been named with the 'handle' prefix.
*/
exports.Handler.prototype.handleCreate = function() {
  throw "create method not implemented";
}

exports.Handler.prototype.handleDelete = function(referenceData) {
  throw "delete method not implemented";
}

exports.Handler.prototype.handleUpdate = function(referenceData) {
  var self = this;
  return this.handleDelete(referenceData)
    .then(function() {
      return self.handleCreate();
    });
}

exports.Handler.prototype.error = function(message) {
  console.error(message);
  response.send(this.event, this.context, response.FAILED, { Error: message });
  throw message;
}

exports.Handler.prototype.getStackName = function() {
  var functionName = this.context.functionName;
  // Assume functionName is: stackName-resourceLogicalId-randomString.
  // Per http://docs.aws.amazon.com/AWSCloudFormation/latest/UserGuide/resources-section-structure.html,
  // resourceLogicalId cannot include a '-'; randomString seems to also be alphanumeric.
  // Thus it seems safe to search for the two dashes in order to find the stackName.
  var i = functionName.lastIndexOf("-", functionName.lastIndexOf("-") - 1);
  if (i >= 0)
    return functionName.substr(0, i);
  else
    return functionName;
}

exports.Handler.prototype.getReferenceData = function() {
  return dynamoDB.getItemAsync(
    {
      TableName: this.getStackName() + "-reference",
      Key: helpers.formatForDynamo({
        key: this.event.StackId + this.event.LogicalResourceId
      }, true)
    }
  )
  .then(function(data) {
    data = helpers.formatFromDynamo(data);
    if (data && data.Item && data.Item.value)
      return data.Item.value;
    else
      return null;
  })
}

exports.Handler.prototype.setReferenceData = function(data) {
  return dynamoDB.putItemAsync(
    {
      TableName: this.getStackName() + "-reference",
      Item: helpers.formatForDynamo({
        key: this.event.StackId + this.event.LogicalResourceId,
        value: data
      }, true)
    }
  );
}