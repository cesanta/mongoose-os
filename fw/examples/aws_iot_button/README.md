# AWS IoT button example

This is an "Internet Button" example: when a button on the device is pressed,
a notification email is sent to the specific email address.

In more detail, this is what happens:

  - User presses the button
  - Device sends a message to the MQTT topic `<device_id>/button_pressed`
  - AWS IoT receives the message and calls AWS Lambda Function
  - AWS Lambda Function publishes a message to the AWS SNS (Simple Notification
    Service)
  - AWS SNS notifies subscribers: in this case, just sends a message to a
    single email address
  - User receives the email

## Build instructions

```bash
# Flash the firmware (you might need to adjust the architecture)
mos flash mos-esp8266

# Get device id (e.g. esp8266_DA84C1), we'll need it later
DEVICE_ID=$(mos config-get device.id)
MY_EMAIL=my@email.com

# Instantiate AWS stack. In the command below, replace <device_id> with the ID
# you obtained above, and <your_email_address> with your actual email address.
#
# I use stack name "my-internet-button" here, but you can use any other name.
aws cloudformation create-stack \
    --stack-name my-internet-button \
    --parameters \
        ParameterKey=TopicName,ParameterValue=$DEVICE_ID/button_pressed \
        ParameterKey=SubscriptionEmail,ParameterValue=$MY_EMAIL \
    --capabilities CAPABILITY_IAM \
    --template-body file://aws_button_template.json

# Wait until the stack creation is completed (it may take a few minutes).
aws cloudformation wait stack-create-complete --stack-name my-internet-button

# Alternatively, you can use the web UI to check the status and read event
# details: https://console.aws.amazon.com/cloudformation/home

# NOTE: During stack creation, AWS will send a Subscription Confirmation email,
# so check your email and confirm the subscription by following a link.

# Put init.js on the device
mos put init.js

# Set wifi configuration
mos wifi WIFI_SSID WIFI_PASSWORD

# Setup device to connect to AWS IoT
mos aws-iot-setup --aws-iot-policy=mos-default

# Attach console
mos console
```

When the device is connected to the AWS IoT, push the button on your device.
In the device's console, you'll see a message like this:

```
Published: yes topic: esp8266_DA84C1/button_pressed message: {"free_ram":26824,"total_ram":44520}
```

Now, check your email. It'll contain a new message:

```
Button pressed: esp8266_DA84C1/button_pressed
```

Now you can go to your AWS dashboard and play with your stack. For example, you
may add more subscriptions to the SNS: other than sending emails, it can also
call some URL, send SMS, etc.

And, of course, you can modify your lambda function to do whatever you want
in response to the button press. Have fun!
