var Promise = require('bluebird'),
    AWS = require('aws-sdk'),
    base = require('lib/base'),
    helpers = require('lib/helpers'),
    s3 = Promise.promisifyAll(new AWS.S3());

// Exposes the SNS.subscribe API method
function PutObject(event, context) {
  base.Handler.call(this, event, context);
}
PutObject.prototype = Object.create(base.Handler.prototype);
PutObject.prototype.handleCreate = function() {
  var p = this.event.ResourceProperties;
  delete p.ServiceToken;
  return s3.putObjectAsync(p);
}
PutObject.prototype.handleDelete = function(referenceData) {
  var p = this.event.ResourceProperties;
  return Promise.try(function() {
    if (p.Key.endsWith("/")) {
      s3.listObjectsAsync({
        Bucket: p.Bucket,
        Prefix: p.Key
      })
      .then(function(subObjects) {
        return Promise
        .map(
          subObjects.Contents,
          function(item) {
            return s3.deleteObjectAsync({
              Bucket: p.Bucket,
              Key: item.Key
            })
          }
        )
      })
    }
  })
  .then(function() {
    return s3.deleteObjectAsync({
      Bucket: p.Bucket,
      Key: p.Key
    });
  });
}
exports.putObject = function(event, context) {
  handler = new PutObject(event, context);
  handler.handle();
}

// Exposes the S3.putBucketPolicy API method
function PutBucketPolicy(event, context) {
  base.Handler.call(this, event, context);
}
PutBucketPolicy.prototype = Object.create(base.Handler.prototype);
PutBucketPolicy.prototype.handleCreate = function() {
  var p = this.event.ResourceProperties;
  delete p.ServiceToken;
  return s3.putBucketPolicyAsync(p)
    .then(function() {
      return {
        BucketName : p.Bucket
      }
    });
}
PutBucketPolicy.prototype.handleDelete = function(referencedData) {
  return Promise.try(function() {
    if(referencedData) {
      return s3.deleteBucketPolicyAsync({
        Bucket : referencedData.BucketName
      });
    }
  });
}
exports.putBucketPolicy = function(event, context) {
  console.log(JSON.stringify(event));
  handler = new PutBucketPolicy(event, context);
  handler.handle();
}

