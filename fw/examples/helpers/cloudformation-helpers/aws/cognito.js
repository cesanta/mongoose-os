'use strict';

const https = require("https");
const url = require("url");
const AWS = require('aws-sdk');
const cognitoidentity = new AWS.CognitoIdentity();
const cognitoidentityserviceprovider = new AWS.CognitoIdentityServiceProvider();
const SUCCESS = 'SUCCESS';
const FAILED = 'FAILED';
const UNKNOWN = {
  Error: 'Unknown operation'
};
const requestTypes = [
  'Create',
  'Update',
  'Delete'
];

// Some field are encoded as strings in CloudFormation, e.g. "true" and
// "false", so we need to convert them back to the proper type
function tr(val) {
  switch (typeof val) {
    case "string":
      if (val === "true") {
        return true;
      } else if (val === "false") {
        return false;
      } else {
        return val;
      }
      break;
    case "object":
      if (Array.isArray(val)) {
        for (var i = 0; i < val.length; i++) {
          val[i] = tr(val[i]);
        }
      } else {
        for (var name in val) {
          if (val.hasOwnProperty(name)) {
            val[name] = tr(val[name]);
          }
        }
      }
      return val;
    default:
      return val;
  }
}

/**
 * The Lambda function handler
 * See also: https://docs.aws.amazon.com/AWSCloudFormation/latest/UserGuide/crpg-ref.html
 * 
 * @param {!Object} event
 * @param {!string} event.RequestType
 * @param {!string} event.ServiceToken
 * @param {!string} event.ResponseURL
 * @param {!string} event.StackId
 * @param {!string} event.RequestId
 * @param {!string} event.LogicalResourceId
 * @param {!Object} event.ResourceProperties
 * @param {!Object} context
 * @param {!Requester~requestCallback} callback
 */
exports.handler = (event, context, callback) => {
  console.log('event:', JSON.stringify(event, undefined, "  "));
  console.log('context:', JSON.stringify(context, undefined, "  "));

  function respond(responseStatus, responseData, physicalResourceId) {
    const responseBody = JSON.stringify({
      Status: responseStatus,
      Reason: "See the details in CloudWatch Log Stream: " + context.logStreamName,
      PhysicalResourceId: physicalResourceId || context.logStreamName,
      StackId: event.StackId,
      RequestId: event.RequestId,
      LogicalResourceId: event.LogicalResourceId,
      Data: responseData
    });

    console.log('Response body:\n', responseBody);

    const parsedUrl = url.parse(event.ResponseURL);
    const options = {
      hostname: parsedUrl.hostname,
      port: 443,
      path: parsedUrl.path,
      method: 'PUT',
      headers: {
        'content-type': '',
        'content-length': responseBody.length
      }
    };

    return new Promise((resolve, reject) => {
      const request = https
        .request(options, resolve);

      request.on('error', error => reject(`send(..) failed executing https.request(..): ${error}`));
      request.write(responseBody);
      request.end();
    })
    .then(() => callback(responseStatus === FAILED ? responseStatus : null, responseData))
    .catch(callback);
  }

  if (!~requestTypes.indexOf(event.RequestType))
    return respond(FAILED, UNKNOWN);

  const requestType = event.RequestType;
  let params = requestType === 'Delete' ? {} : event.ResourceProperties.Options;

  switch (event.LogicalResourceId) {
    case 'CognitoIdentityPool':
      if (requestType !== 'Create') {
        params.IdentityPoolId = event.PhysicalResourceId;
      }

      params = tr(params);

      return cognitoidentity
      [`${requestType.toLowerCase()}IdentityPool`](params)
        .promise()
        .then(data => respond(SUCCESS, data, data.IdentityPoolId))
        .catch(err => respond(FAILED, err));

    case 'CognitoUserPool':
      if (requestType !== 'Create') {
        params.UserPoolId = event.PhysicalResourceId;
      }

      params = tr(params);

      console.log("Params for `${requestType.toLowerCase()}UserPool`:", params);
      console.log("Calling...");

      return cognitoidentityserviceprovider
      [`${requestType.toLowerCase()}UserPool`](params)
        .promise()
        .then(data => {
          console.log("success! data:", data);

          var physId = undefined;

          if (requestType === 'Create') {
            // data.UserPool is apparently too large and beats the 4K limit,
            // so in order to prevent an error "Response is too long", we have
            // to delete something SchemaAttributes.
            delete data.UserPool.SchemaAttributes;
            physId = data.UserPool.Id;
          }

          respond(SUCCESS, data, physId);
        })
        .catch(err => {
          console.log("error:", err);
          respond(FAILED, err);
        });

    case 'CognitoUserPoolClient':
      if (requestType !== 'Create') {
        params.ClientId = event.PhysicalResourceId;
        params.UserPoolId = event.ResourceProperties.Options.UserPoolId;
      }
      params = tr(params);

      console.log(`Params for ${requestType.toLowerCase()}UserPoolClient:`, params);
      console.log("Calling...");

      return cognitoidentityserviceprovider
      [`${requestType.toLowerCase()}UserPoolClient`](params)
        .promise()
        .then(data => {
          console.log("success! data:", data);
          var physId = undefined;
          if (requestType === 'Create') {
            physId = data.UserPoolClient.ClientId;
          }
          respond(SUCCESS, data, physId);
        })
        .catch(err => {
          console.log("error:", err);
          respond(FAILED, err);
        });

    case 'CognitoIdentityPoolRoles':
      switch (requestType) {
        case 'Create':
        case 'Update':

          params = tr(params);

          if ("MyRoleMappings" in params) {
            params.RoleMappings = {};
            for (var i = 0; i < params.MyRoleMappings.length; i++) {
              var cur = params.MyRoleMappings[i];
              params.RoleMappings[cur.Key] = cur.Val;
            }
            delete params.MyRoleMappings;
          }

          return cognitoidentity
            .setIdentityPoolRoles(params)
            .promise()
            .then(data => respond(SUCCESS, data))
            .catch(err => respond(FAILED, err));
          case 'Delete':
            return respond(SUCCESS);
      }

    case 'CognitoUserGroup':
      if (requestType !== 'Create') {
        params.GroupName = event.PhysicalResourceId;
        params.UserPoolId = event.ResourceProperties.Options.UserPoolId;
      }
      params = tr(params);

      console.log(`Params for ${requestType.toLowerCase()}Group:`, params);
      console.log("Calling...");

      return cognitoidentityserviceprovider
      [`${requestType.toLowerCase()}Group`](params)
        .promise()
        .then(data => {
          console.log("success! data:", JSON.stringify(data));
          var physId = undefined;
          if (requestType === 'Create') {
            physId = data.Group.GroupName;
          }
          respond(SUCCESS, data, physId);
        })
        .catch(err => {
          console.log("error:", err);
          respond(FAILED, err);
        });

    default:
      return respond(FAILED, UNKNOWN);
  }
};
