var Promise = require('bluebird'),
    AWS = require('aws-sdk'),
    base = require('lib/base'),
    apiGateway = Promise.promisifyAll(new AWS.APIGateway());

// Exposes the SNS.subscribe API method
function CreateApi(event, context) {
  base.Handler.call(this, event, context);
}
CreateApi.prototype = Object.create(base.Handler.prototype);
CreateApi.prototype.handleCreate = function() {
  var p = this.event.ResourceProperties;
  var rootObject = this;
  return apiGateway.createRestApiAsync({
    name: p.name,
    description: p.description
  })
  .then(function(apiData) {
    return rootObject.setReferenceData({ restApiId: apiData.id }) // Set this immediately, in case later calls fail
    .then(function() {
      return apiGateway.getResourcesAsync({
        restApiId: apiData.id
      })
      .then(function(resourceData) {
        return setupEndpoints(p.endpoints, resourceData.items[0].id, apiData.id)
        .then(function(endpointsData) {
          return apiGateway.createDeploymentAsync({
            restApiId: apiData.id,
            stageName: p.version
          })
          .then(function(deploymentData) {
            // Total hack: there are limits to the number of API Gateway API calls you can make. So this function
            // will fail if a CloudFormation template includes two or more APIs. Attempting to avoid this by blocking.
            var until = new Date();
            until.setSeconds(until.getSeconds() + 60);
            while (new Date() < until) {
              // Wait...
            }
            // AWS.config.region is a bit of a hack, but I can't figure out how else to dynamically
            // detect the region of the API - seems to be nothing in API Gateway or AWS Lambda context.
            // Could possibly get it from the CloudFormation stack, but that seems wrong.
            return {
              baseUrl: "https://" + apiData.id + ".execute-api." + AWS.config.region + ".amazonaws.com/" + p.version,
              restApiId: apiData.id
            };
          });
        });
      });
    });
  });
}
CreateApi.prototype.handleDelete = function(referenceData) {
  return Promise.try(function() {
    if (referenceData && referenceData.restApiId) {
      // Can simply delete the entire API - don't need to delete each individual component
      return apiGateway.deleteRestApiAsync({
        restApiId: referenceData.restApiId
      });
    }
  })
}
function setupEndpoints(config, parentResourceId, restApiId) {
  return Promise.map(
    Object.keys(config),
    function(key) {
      switch (key.toUpperCase()) {
        case 'GET':
        case 'HEAD':
        case 'DELETE':
        case 'OPTIONS':
        case 'PATCH':
        case 'POST':
        case 'PUT':
          var params = config[key];
          params["httpMethod"] = key.toUpperCase()
          params["resourceId"] = parentResourceId
          params["restApiId"] = restApiId
          params["apiKeyRequired"] = params["apiKeyRequired"] == "true" // Passing through CloudFormation, booleans become strings :(
          var integration = params["integration"]
          delete params.integration
          return apiGateway.putMethodAsync(params)
          .then(function() {
            return Promise.try(function() {
              if (integration) {
                var contentType = integration["contentType"]
                if (!contentType) {
                  throw "Integration config must include response contentType."
                }
                delete integration.contentType
                integration["httpMethod"] = key.toUpperCase()
                integration["resourceId"] = parentResourceId
                integration["restApiId"] = restApiId
                return apiGateway.putIntegrationAsync(integration)
                .then(function(integrationData) {
                  var responseContentTypes = {}
                  responseContentTypes[contentType] = "Empty"
                  return apiGateway.putMethodResponseAsync({
                    httpMethod: key.toUpperCase(),
                    resourceId: parentResourceId,
                    restApiId: restApiId,
                    statusCode: '200',
                    responseModels: responseContentTypes
                  })
                  .then(function(methodResponseData) {
                    responseContentTypes[contentType] = ""
                    return apiGateway.putIntegrationResponseAsync({
                      httpMethod: key.toUpperCase(),
                      resourceId: parentResourceId,
                      restApiId: restApiId,
                      statusCode: '200',
                      responseTemplates: responseContentTypes
                    });
                  });
                });
              }
            });
          });
        default:
          return apiGateway.createResourceAsync({
            parentId: parentResourceId,
            pathPart: key,
            restApiId: restApiId,
          })
          .then(function(resourceData) {
            return setupEndpoints(config[key], resourceData.id, restApiId);
          });
      }
    }
  );
}
exports.createApi = function(event, context) {
  handler = new CreateApi(event, context);
  handler.handle();
}
