# AWS IoT button example

This is a draft of "AWS IoT Smart Heater" example.

This particular example creates a stack on AWS IoT which consists of the
dynamoDB table and a rule which listens for the messages from the device, and
for each message creates an item in the dynamoDB table.

## Build instructions

```bash
# Flash the firmware (you might need to adjust the architecture)
mos flash mos-esp8266

# Get device id (e.g. esp8266_DA84C1), we'll need it later
mos config-get device.id

# Instantiate AWS stack (replace <device_id> with the ID you obtained above)
#
# In the example command I use stack name "my-internet-button", but you can
# use any other name.
aws cloudformation create-stack \
    --stack-name my-internet-button \
    --parameters ParameterKey=TopicName,ParameterValue=<device_id>/button_pressed \
    --capabilities CAPABILITY_IAM \
    --template-body file://aws_iot_heater_template.json

# Wait until the stack creation is completed (it may take a few minutes).
# You can check the status of stack creation with the following command:
aws cloudformation describe-stack-events --stack-name my-internet-button

# It will return a list of events, the top one is the latest one. When stack
# creation is done, the top event will have the following properties:
#   "ResourceStatus": "CREATE_COMPLETE",
#   "ResourceType": "AWS::CloudFormation::Stack",
# Alternatively, you can use the web UI; e.g. for the eu-west-1 region the URL is:
# https://eu-west-1.console.aws.amazon.com/cloudformation/home?region=eu-west-1#/stacks?filter=active

# Put init.js on the device
mos put init.js

# Set wifi configuration
mos config-set wifi.sta.enable=true wifi.ap.enable=false \
               wifi.sta.ssid=WIFI_SSID wifi.sta.pass=WIFI_PASS

# Setup device to connect to AWS IoT
mos aws-iot-setup --aws-iot-policy=mos-default

# Attach console
mos console

# When the device is connected to the AWS IoT, navigate to the AWS dynamoDB
# web UI (e.g. for the region eu-west-1 the URL is: https://eu-west-1.console.aws.amazon.com/dynamodb/home),
# open the created table (named like "my-internet-button-myDynamoDBTable-XXXXXXXX"),
# and try to press a button on your device. Each button press will result in
# a new item added to the table.
```
