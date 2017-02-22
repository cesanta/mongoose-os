var Promise = require('bluebird'),
    AWS = require('aws-sdk'),
    base = require('lib/base'),
    helpers = require('lib/helpers'),
    ses = Promise.promisifyAll(new AWS.SES());
    
// Exposes the SES.createReceiptRule API method
function CreateReceiptRule(event, context) {
  base.Handler.call(this, event, context);
}
CreateReceiptRule.prototype = Object.create(base.Handler.prototype);
CreateReceiptRule.prototype.handleCreate = function() {
  var p = this.event.ResourceProperties;
  delete p.ServiceToken;
  p.Rule.Enabled = ("true" === p.Rule.Enabled );
  p.Rule.ScanEnabled = ("true" === p.Rule.ScanEnabled );
  return ses.createReceiptRuleAsync(p)
    .then(function() {
      return {
        RuleSetName : p.RuleSetName,
        RuleName : p.Rule.Name
      }
    });
}
CreateReceiptRule.prototype.handleDelete = function(referenceData) {
  return Promise.try(function() {
    if (referenceData) {
      return ses.deleteReceiptRuleAsync({
        RuleSetName : referenceData.RuleSetName,
        RuleName : referenceData.RuleName
      });
    }
  });
}
exports.createReceiptRule = function(event, context) {
  console.log(JSON.stringify(event));
  handler = new CreateReceiptRule(event, context);
  handler.handle();
}
