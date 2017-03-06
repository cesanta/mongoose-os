# AWS IoT Smart Heater

This is an "AWS IoT Smart Heater" example: the heater device reports current
temperature, responds to the status requests and to the heater on/off command.

## Build instructions

First of all, you'll need to create Google and/or Facebook OAuth2 Client, so
that users will be able to login into the heater application.

For Google: visit [Google Cloud Console](https://console.cloud.google.com/apis/credentials),
click Create credentials -> OAuth client ID -> Web application, and enter some
name, e.g. "AWS Heater", and click "Create". It will show your client ID and
secret; copy client ID, you'll need it soon. And don't close the tab for now:
when your stack is instantiated, you'll need to get back here and enter the
Authorized JavaScript origin.

For Facebook: visit [Facebook Apps](https://developers.facebook.com/apps), click
"Add a New App", enter some name, like, "My Heater", pick a category, click
"Create App ID". When the app creation is done, you'll see the app dashboard.
Don't close the tab for now: when your stack is instantiated, you'll need to
get back here and enter the Website URL.

```bash
# Flash the firmware (you might need to adjust the architecture)
mos flash mos-esp8266

# Get device id (e.g. esp8266_DA84C1), we'll need it later
mos config-get device.id

# Put init.js on the device
mos put init.js

# Set wifi configuration
mos config-set wifi.sta.enable=true wifi.ap.enable=false \
               wifi.sta.ssid=WIFI_SSID wifi.sta.pass=WIFI_PASS

# Setup device to connect to AWS IoT
mos aws-iot-setup --aws-iot-policy=mos-default

# Attach console
mos console

# Now, in another terminal, instantiate AWS stack.
# First of all, install node_modules for the helpers
npm --prefix ../helpers/cloudformation-helpers install ../helpers/cloudformation-helpers

# We'll also need to create a separate S3 bucket for helper functions:
aws s3 mb s3://my-cf-helpers

# Now, "package" the template. Packaging includes copying source code of the
# helper functions from local machine to the s3 bucket we created above,
# and adjusting the template appropriately. It's all done in one step:
aws cloudformation package \
    --template-file aws_iot_heater_template.yaml \
    --s3-bucket my-cf-helpers \
    --output-template-file packaged_template.yaml

# The command above has created a new template file: packaged-template.yaml.
# Now, instantiate AWS stack (replace <device_id> with the ID you obtained by
# config-get in the beginning. Also, depending on the authentication
# provider(s) you have created apps for, provide client IDs. The command below
# contains parameters for both Google and Facebook; if you don't use some of
# those, just omit the whole parameter)
#
# In the example command I use stack name "my-heater", but you can
# use any other name.
aws cloudformation create-stack \
    --stack-name my-heater \
    --parameters \
        ParameterKey=DeviceID,ParameterValue=<device_id> \
        ParameterKey=GoogleClientID,ParameterValue=<google_client_id> \
        ParameterKey=FacebookClientID,ParameterValue=<facebook_client_id> \
    --capabilities CAPABILITY_IAM \
    --template-body file://packaged_template.yaml

# Wait until the stack creation is completed (it may take a few minutes).
aws cloudformation wait stack-create-complete --stack-name my-heater

# Alternatively, you can use the web UI to check the status and read event
# details: https://console.aws.amazon.com/cloudformation/home

# When the stack is created, get the name of the created S3 bucket:
aws cloudformation describe-stacks --stack-name my-heater

# look for the following:
#  ...
#  {
#      "Description": "Name of the newly created S3 bucket", 
#      "OutputKey": "S3BucketName", 
#      "OutputValue": "<my-s3bucket-name>"
#  },
#  {
#      "Description": "URL of the s3 bucket", 
#      "OutputKey": "S3BucketWebsite", 
#      "OutputValue": "<app-url>"
#  }
#  ...
#
# <my-s3bucket-name> is the name of the bucket, and <app-url> is the URL at
# which your files can be accessed.
#
# Copy the actual value of "<app-url>", and then enter it in the Google and/or
# Facebook app settings: For Google: go back to the Google Console, and add the
# URL as an Authorized JavaScript origin.  For Facebook: go back to the app's
# dashboard, click "Settings" in the sidebar, then click "Add Platform" at the
# bottom, select "Website", and enter Site URL.
#
# Then, copy the actual value of "<my-s3bucket-name>" (from the describe-stacks
# output), and use it to put two files on the S3 bucket:
aws s3 cp bucket/index.html s3://<my-s3bucket-name> --acl public-read
aws s3 cp bucket/index.js s3://<my-s3bucket-name> --acl public-read

# Download two files of Cognito SDK, and also put them on the S3 bucket:
wget https://raw.githubusercontent.com/aws/amazon-cognito-identity-js/master/dist/aws-cognito-sdk.min.js
wget https://raw.githubusercontent.com/aws/amazon-cognito-identity-js/master/dist/amazon-cognito-identity.min.js
aws s3 cp aws-cognito-sdk.min.js s3://<my-s3bucket-name> --acl public-read
aws s3 cp amazon-cognito-identity.min.js s3://<my-s3bucket-name> --acl public-read

# Now, navigate to the index page of your app (<app-url>).
```

You'll see latest graph of the temperature reported from the device, current
heater status (on/off), and the switch. Switching the heater is possible only
for authenticated and authorized users; click "Sign in with Google".

NOTE: if it complains about mismatched redirect URI, just wait a couple of
minutes: the settings in Google Console might need some time to take effect.

If you try to switch the heater status, you'll get the message saying that
you are not authorized to do that. Now, you need to authorize your user to
manage heater.

For that, navigate to the [AWS Cognito console](https://console.aws.amazon.com/cognito/home),
click "Manage Federated Identities", select "identity_pool_for_<device_id>",
click "Edit identity pool", expand "Authentication providers", click on the
"Google+" tab, and in the section "Authenticated role selection" change
"Use default role" to "Choose role with rules". Here, you can use whatever
rule you want. For example, in order to authorize some particular user, you
can specify Claim: "email", match type: "Equals", value: "addr@domain.com",
and pick a role "my-heater-myHeaterAdminRole-XXXXXXXX".

After that, you can sign out from your heater app, sign in back, and switching
the heater should result in the state being changed.
