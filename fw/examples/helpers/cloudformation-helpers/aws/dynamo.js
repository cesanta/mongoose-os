var Promise = require('bluebird'),
    AWS = require('aws-sdk'),
    base = require('lib/base'),
    helpers = require('lib/helpers'),
    dynamoDB = Promise.promisifyAll(new AWS.DynamoDB());

// Exposes the SNS.subscribe API method
function PutItems(event, context) {
  base.Handler.call(this, event, context);
}
PutItems.prototype = Object.create(base.Handler.prototype);
PutItems.prototype.handleCreate = function() {
  var p = this.event.ResourceProperties;
  return dynamoDB.describeTableAsync({
    TableName: p.TableName
  })
  .then(function(tableData) {
    return Promise
    .map(
      p.Items,
      function(item) {
        return dynamoDB.putItemAsync({
          TableName: p.TableName,
          Item: helpers.formatForDynamo(item, true)
        })
        .then(function() {
          var key = {};
          tableData.Table.KeySchema.forEach(function(keyMember) {
            key[keyMember.AttributeName] = item[keyMember.AttributeName]
          });
          return {
            TableName: p.TableName,
            Key: key
          };
        });
      }
    )
    .then(function(itemsInserted) {
      return {
        ItemsInserted: itemsInserted
      }
    });
  });
}
PutItems.prototype.handleDelete = function(referenceData) {
  return Promise.try(function() {
    if (referenceData) {
      return Promise
      .map(
        referenceData.ItemsInserted,
        function(item) {
          return dynamoDB
          .deleteItemAsync({
            TableName: item.TableName,
            Key: helpers.formatForDynamo(item.Key, true)
          })
          .then(function(data) {
            return item.Key;
          });
        }
      )
      .then(function(itemsDeleted) {
        return {
          ItemsDeleted: itemsDeleted
        }
      });
    }
  });
}
exports.putItems = function(event, context) {
  handler = new PutItems(event, context);
  handler.handle();
}
