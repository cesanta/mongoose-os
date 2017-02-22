# cloudformation-helpers
A collection of AWS Lambda funtions that fill in the gaps that existing CloudFormation resources do not cover.

AWS CloudFormation supports Custom Resources (http://docs.aws.amazon.com/AWSCloudFormation/latest/UserGuide/template-custom-resources.html),
which can be used to call AWS Lambda functions. CloudFormation covers much of the AWS API landscape, but
does leave some gaps unsupported. AWS Lambda can contain any sort of logic, including interacting with the
AWS API in ways not covered by CloudFormation. By combining the two, CloudFormation deploys should be able
to approach the full resource support given by the AWS API.

Warning: most of these functions require fairly wide permissions, since they need access to resources in a
general manner - much the same way CloudFormation itself has permission to do almost anything.


## Usage
1. Use https://s3.amazonaws.com/com.gilt.public.backoffice.{your-region-here}/cloudformation_templates/create_cloudformation_helper_functions.template
   to deploy a stack that creates the Lambda functions for you. Remember the stack name.
2. Include the following resources in your CloudFormation template. These will create a) a nested stack that
   looks up the ARNs from the previous step and b) a custom resource that allows your template to read those ARNs.
   
   ```
   "CFHelperStack": {
     "Type": "AWS::CloudFormation::Stack",
     "Properties": {
       "TemplateURL": { "Fn:Join": [ "", ["https://s3.amazonaws.com/com.gilt.public.backoffice", { "Ref": "AWS:Region" }, "/cloudformation_templates/lookup_stack_outputs.template" ] ] }
     }
   },
   "CFHelper": {
     "Type": "Custom::CFHelper",
     "Properties": {
       "ServiceToken": { "Fn::GetAtt" : ["CFHelperStack", "Outputs.LookupStackOutputsArn"] },
       "StackName": "your-helper-stack-name-here"
     },
     "DependsOn": [
       "CFHelperStack"
     ]
   }
   ```
   
   You can either hardcode the stack name of your helper functions, or request it as a parameter.
3. Use the ARNs from the previous step in a custom resource, to call those Lambda functions:

   ```
   "PopulateTable": {
     "Type": "Custom::PopulateTable",
     "Properties": {
       "ServiceToken": { "Fn::GetAtt" : ["CFHelper", "DynamoDBPutItemsFunctionArn"] },
       "TableName": "your-table-name",
       "Items": [
         {
           "key": "foo1",
           "value": {
             "bar": 1.5,
             "baz": "qwerty"
           }
         },
         {
           "key": "foo2",
           "value": false
         }
       ]
     },
     "DependsOn": [
       "CFHelper"
     ]
   }
   ```


## Warning
If you update your helper stack, all client stacks (i.e. other stacks that use the helpers) will be okay
after the new version is deployed. But some helpers rely on stored data to do the delete when the client
stack is deleted. This is stored in a DynamoDB table and will get blown away when you delete-then-recreate
the helper stack. You can get around this by updating the stack instead of tearing it down and recreating
it. But it's probably a good idea to export the helper's DynamoDB table so you have a backup. If you lose
the data, when you tear down client stacks, you may have to manually delete some resources created by the
helpers.


## A note on AWS Regions
At times, AWS can be touchy when it comes to referencing resources across regions. With that in mind, this
project is replicated to all regions - so you simply need to reference the version that lives in the S3
bucket that corresponds to the region you're working in. If there is a new region that is not part of the
ring of replications, please create an [issue](https://github.com/gilt/cloudformation-helpers/issues).


## Steps for adding a new included function

1. Find the file to edit.
   * If a file already exists for the AWS service in the [aws directory](aws), add the code there.
   * If not, create a new file for the AWS service.
2. Implement the function
   * Add a new class that implements base.Handler for the new functionality. In most cases, it will be sufficient
     to pass all parameters (other than ServiceToken through to the AWS client).
   * If the 'delete' can't be implemented using the 'create' parameters, make sure the 'create' returns all
     necessary data needed for the 'delete' (such as unknowable ids); these will be stored in DynamoDB for future
     reference in the 'delete'.
3. Add a new export method that calls the new class.
4. Add two new Resources to the [create_cloudformation_helper_functions.template](create_cloudformation_helper_functions.template):
   * The Role for the new function.
   * The function itself (referencing the new method from #3).
5. Add an output for the function ARN to the template.
6. Add an example template to the [test directory](test/aws).
7. Update the [README](README.md) to explain the new helper function.


## Included functions

### Create a full API in API Gateway

Pass in all of the components of the API endpoints, and the API will be created in API Gateway.

This will delete the entire API when the corresponding stack is deleted.

#### Parameters

##### name
The name of the API, seen in the list of APIs in AWS Console.

##### description
The description of what the API does.

##### endpoints
A JSON array of (see the example template below for a working example):

```javascript
  {
    "resource": {
      "sub-resource": {
        "GET": {
          "authorizationType": "NONE",
          "integration": {
            "type": "MOCK",
            "contentType": "text/html"
          }
        }
      },
      "POST": {
        "authorizationType": "NONE",
        "integration": {
          "type": "AWS",
          "integrationHttpMethod": "POST",
          "uri": "arn:aws:apigateway:us-east-1:lambda:path/2015-03-31/functions/arn:aws:lambda:*:*:function:your-function-name/invocations",
          "contentType": "application/json"
        }
      }
    }
  }
```

1. The properties of the JSON document are either a) the HTTP method or b) a sub-resource.
2. The resources cannot include a '/' - any resources nested in the path should also be nested
   in the JSON document.
3. The parameters of each http method object are the same as putMethod callh ere:
   http://docs.aws.amazon.com/AWSJavaScriptSDK/latest/AWS/APIGateway.html#putMethod-property -
   except that httpMethod, resourceId, and restApiId will be added for you.
4. Each http method object can also optionally specify an "integration" property, which follows
   http://docs.aws.amazon.com/AWSJavaScriptSDK/latest/AWS/APIGateway.html#putIntegration-property with
   the same properties automatically filled in for you.
5. The "integration" property must include a "contentType" property that specifies the response
   Content-Type of the endpoint.

#### Output

##### baseUrl
The base url of the API endpoints. Combine this with the relative paths defined in the config to
put together the full url for the API call.

##### restApiId
The id of the API that is created.


#### Reference Output Name
ApiGatewayCreateApiFunction

#### Example/Test Template
[apiGateway.createApi.template](test/aws/apiGateway.createApi.template)


### Insert items into DynamoDB

Pass in a list of items to be inserted into a DynamoDB table. This is useful to provide a template for the
content of the table, or to populate a config table. There is no data-checking, so it is up to the client
to ensure that the format of the data is correct.

Warning: it is a PUT, so it will overwrite any items that already exist for the table's primary key.

This will delete the items when the corresponding stack is deleted.

#### Parameters

##### TableName
The name of the DynamoDB table to insert into. Must exist at the time of the insert, i.e. will not create if
it does not already exist.

##### Items
A JSON array of items to be inserted, in JSON format (not DynamoDB format).

#### Output
The list of TableName/Key pairs of items created.

#### Reference Output Name
DynamoDBPutItemsFunctionArn

#### Example/Test Template
[dynamo.putItems.template](test/aws/dynamo.putItems.template)


### Create a Kinesis stream

Mirrors the [Kinesis.createStream API method](http://docs.aws.amazon.com/AWSJavaScriptSDK/latest/AWS/Kinesis.html#createStream-property).
Yes, CloudFormation does support creating a Kinesis stream, but it does not allow you to specify the steam name; this
helper does.

This will delete the stream when the corresponding stack is deleted.

#### Parameters

##### ShardCount
The number of shards for the stream. Required; no default.

##### StreamName
The name of the stream. Required.

#### Output
StreamName and Arn (of the stream) - matches the output of the existing CloudFormation task.

#### Reference Output Name
KinesisCreateStreamFunctionArn

#### Example/Test Template
[kinesis.createStream.template](test/aws/kinesis.createStream.template)


### Put CloudWatch Logs Metric Filter

Mirrors the [CloudWatchLogs.putMetricFilter API method](http://docs.aws.amazon.com/AWSJavaScriptSDK/latest/AWS/CloudWatchLogs.html#putMetricFilter-property).

This will delete the metric filter when the corresponding stack is deleted.

#### Parameters
Exactly the same as the documentation above.

#### Reference Output Name
CloudWatchLogsPutMetricFilterFunctionArn

#### Example/Test Template
[cloudWatchLogs.putMetricFilter.template](test/aws/cloudWatchLogs.putMetricFilter.template)


### Put S3 Objects

Mirrors the [S3.putObject API method](http://docs.aws.amazon.com/AWSJavaScriptSDK/latest/AWS/S3.html#putObject-property).

This will delete the objects and any added sub-objects (for "folders") when the corresponding stack is deleted.

#### Parameters

##### Bucket
The S3 bucket to put the object into

##### Key
The name of the object. Can include a "path" for organization in S3.

##### Body
The content of the object being put into S3 (a string).

##### Other
Please see the reference above for further parameters - these are only the most commonly-used ones.

#### Output
The result of the S3.PutObject API method.

#### Reference Output Name
S3PutObjectFunctionArn

#### Example/Test Template
[s3.putObject.template](test/aws/s3.putObject.template)


### Put S3 Bucket Policy

Mirrors the [S3.putBucketPolicy API method](http://docs.aws.amazon.com/AWSJavaScriptSDK/latest/AWS/S3.html#putBucketPolicy-property).

This will replace the existing policy if any is already configured.

#### Parameters

##### Bucket
The S3 bucket to put the policy

##### Policy
The policy to put (it is a string containing a JSON description of the policy. All quotes in the policy must hence be escaped)

#### Reference Output Name
S3PutBucketPolicyFunctionArn

#### Example/Test Template
[s3.putBucketPolicy.template](test/aws/s3.putBucketPolicy.template)

### Subscribe to SNS topics

Mirrors the [SNS.Subscribe API method](http://docs.aws.amazon.com/AWSJavaScriptSDK/latest/AWS/SNS.html#subscribe-property).
To be used when the SNS topic already exists (since CloudFormation allows subscriptions to be created when creating SNS
topics only).

This will delete the subscription when the corresponding stack is deleted.

#### Parameters

##### Endpoint
The endpoint that receives the SNS messages.

##### Protocol
The type of endpoint. Can be one of the following values: application, email, email-json, http, https, lambda, sms, sqs.

##### TopicArn
The SNS topic to subscribe to.

#### Output
The result of the SNS.Subscribe API method, i.e. SubscriptionArn

#### Reference Output Name
SnsSubscribeFunctionArn

#### Example/Test Template
[sns.subscribe.template](test/aws/sns.subscribe.template)


### Create a SES Receipt Rule

Allows to create an SES Receipt Rule inside an existing SES Rule set (active or not).
Mirrors the [SES.CreateReceipRule API method](http://docs.aws.amazon.com/AWSJavaScriptSDK/latest/AWS/SES.html#createReceiptRule-property).
This will delete the rule when the corresponding stack is deleted.

#### Paramters

See the reference above or the example below for full list of parameters. All parameters are directly passed 'as is' except boolean which are converted. 

#### Reference Output Name
SesCreateReceiptRuleFunctionArn

#### Example/Test Template
[ses.createReceiptRule.template](test/aws/ses.createReceiptRule.template)



## Deployment (contributors)
After making changes (i.e. adding a new helper function), please do the following:

1. Upload this zipped repo to the com.gilt.public.backoffice.us-east-1/lambda_functions bucket. To produce the .zip file:

   ```
     zip -r cloudformation-helpers.zip . -x *.git* -x *cloudformation-helpers.zip*
   ```

   Unfortunately we can't use the Github .zip file directly, because it zips the code into a subdirectory named after
   the repo; AWS Lambda then can't find the .js file containing the helper functions because it is not on the top-level.

2. Upload the edited create_cloudformation_helper_functions.template to com.gilt.public.backoffice.us-east-1/cloudformation_templates


## License
Copyright 2016 Gilt Groupe, Inc.

Licensed under the Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0
