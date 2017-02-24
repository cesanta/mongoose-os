'use strict';

const https = require("https");
const url = require("url");
const AWS = require('aws-sdk');
const cognitoidentity = new AWS.CognitoIdentity();
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
  console.log(event, context);

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
  const params = requestType === 'Delete' ? {} : event.ResourceProperties.Options;

  switch (event.LogicalResourceId) {
    case 'CognitoIdentityPool':
      if (requestType !== 'Create')
        params.IdentityPoolId = event.PhysicalResourceId;

      // CloudFormation passes the value as a string, so it must be converted to a valid javascript object
      if (requestType !== 'Delete') {
        [
          'AllowUnauthenticatedIdentities',
          'CognitoIdentityProviders',
          'OpenIdConnectProviderARNs',
          'SupportedLoginProviders'
        ].forEach(param => {
          if (params[param])
            params[param] = JSON.parse(params[param]);
        });
      }

      return cognitoidentity
      [`${requestType.toLowerCase()}IdentityPool`](params)
        .promise()
        .then(data => respond(SUCCESS, data, data.IdentityPoolId))
        .catch(err => respond(FAILED, err));
      case 'CognitoIdentityPoolRoles':
        switch (requestType) {
          case 'Create':
          case 'Update':
            return cognitoidentity
              .setIdentityPoolRoles(params)
              .promise()
              .then(data => respond(SUCCESS, data))
              .catch(err => respond(FAILED, err));
            case 'Delete':
              return respond(SUCCESS);
        }
      default:
        return respond(FAILED, UNKNOWN);
  }
};
