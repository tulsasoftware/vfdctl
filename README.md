
# Vfdctl Use Case
Vfdctl was built so that when programmatic interaction is needed with a VFD, you do not need to be aware of the specific I/O, registers or even protocol of your device in order to read or write to it. Instead, a user wanting to update a setting needs to send a command with the knowledge of the device name, the value requested and a parameter name that is consistent across every single VFD device. A user wanting the runtime telemetry just needs to specify topics they care to receive updates for in an MQTT client.

With this decoupling, there are numerous tangible benefits. While running Vfdctl, if a VFD is replaced with a different brand or changes to a different protocol it does not force your controllers, HMI or web app to be updated also. So in this scenario if the VFD is not working after you've replaced it, you can safely assume that the issue occurred while performing the setup of the new VFD because you did not have to re-download to all of your devices with potentially unintended changes. During system downtime, minimizing complexity is key to ensuring the quickest recovery.

Another advantage is that you are not tied to a single brand, make or model of VFD. For consistency, most users will opt to use the same VFD when using multiple in their system, but what if that device isn't available for another 20 weeks during a new project? What if this is a 20 year old system and that device is end of life, so there is no drop-in replacement? Imagine if we only needed to change a single config file for our Vfdctl app with the new brand and desired protocol of the new hardware and we didn't need to download to every 20 year old device on the network! This is the role that Vfdctl serves.

This page describes what’s possible to build with the different tools and APIs that are available on the platform, and how to get the access and information that you need to get started.

# How do I get started with Vfdctl?
## Hardware requirements
Across the platform, there are different controller devices capable of various protocols. This section helps to describe what you need to successfully use our APIs.
In addition to the controllers below, you will need tools and cables to communicate with them such as ethernet cables, micro usb cables, power cables and a micro SD card (1GB or greater).
### Supported Controllers
|  Make | Model  | MQTT Support | Modbus Support |
|--|--|--|--|
| Automation Direct | P1AM-100ETH w/ MKR485 | 3.1.1| RTU | 

# API Documentation
### APIs
|API|MQTT Version Supported |Requires Cloud Connectivity? | R/W |
|--|--|--|--|
|Runtime telemetry|3.1.1|No|Read Only|
|Settings Configuration|3.1.1|No|Write

## Get started with the Vfdctl APIs
To get started using the Vfdctl endpoints there are a few steps of setup before the endpoints will be monitored.
### Setup the connected devices
1. Prepare the configuration file
2. Download a copy of the config.txt from the application's repo on GitHub to use as a template
3. Place the config.txt at the root of a micro SD card
4. Setup an MQTT Broker (via Docker)
5. Install Docker
6. Ensure the Docker daemon is running the Docker desktop app
7. Open a terminal / command prompt
8. Run the `command docker run -it --name mqtt --restart unless-stopped -p 1883:1883 eclipse-mosquitto mosquitto -c /mosquitto-no-auth.conf`
9. Find the ip address of your device, this is the IP address of your MQTT Broker
Within config.txt update the broker_url to the IP address of your MQTT Broker
### Network the devices
1. Using a managed switch or router, connect the controller via ethernet
2. On the same network, connect your machine running the MQTT broker
3. Assign a static IP address to the controller on the same subnet as your MQTT Broker
### Set up the Vfdctl controller device
#### P1AM-100ETH
1. Load the image onto the P1AM-100ETH
2. Power up the P1AM-100ETH controller via 24V
3. Clone the git repo to a machine running Arduino IDE
4. Connect to the P1AM-100ETH controller via micro usb on the machine
5. Open the app.ino project in Arduino IDE
6. Select your connected device and download the app to the controller
7. Load the configuration onto the controller
8. Within config.txt update the device_mac to your controller's ethernet MAC address (this can be usually be found on a sticker on the controller or its packaging)
9. Ensure config.txt is located at the root of the micro SD card
10. Insert the micro SD card into the controller
11. Power cycle the controller
# Monitoring Telemetry and Sending Commands
Vfdctl's public communications uses MQTT for flexibly communicating to an IOT ingestion engine. Any MQTT 3.1.1 capable client on the network is able to speak to the Vfdctl app. For ease of use, I recommend installing [MQTT Explorer](http://mqtt-explorer.com/) to browse topics, publish commands and setup basic data traces. For those unfamiliar with MQTT there is reference material shared within the "Important Resources" section.
## Monitoring device telemetry
Telemetry is reported as a read-only stream on a topic beginning with dt (device telemetry). It is up to the topic's consumer to determine the most appropriate way to handle subscribing to topics. 
Telemetry topics follow the pattern below:
dt/[app]/[device]/[parameter-measured]
As an example, the topic below is expected to report the values of the current drawn by VFD 1:
> dt/vfdctl/vfd1/amps

 Some possible examples of determining your subscription are:
> dt/vfdctl/vfd1/+ 

You are creating an engineering dashboard page per device for a web app, you only care about the currently selected "vfd1" device and want to display all messages in a historian table view
dt/vfdtl/vfd1/amps ; you're now on revision 2 of your engineering dashboard page and have added React components with a line chart where you want to display current draw specifically 

> dt/vfdctl/+/amps 

You want to use ai to predict your billing costs, so you create a trend of mean current draw across all devices on the network by setting up a listener in Node Red that every minute will reset its value, sum every incoming value and at the end of the next interval divide by the number of samples received before publishing the mean on a new topic called dt/facility1/vfd/amps and inserting the data into a database ; a conveyor you are driving can bind if it gets too worn, so you implement a Node Red listener to send a text message to a technician that VFD1's conveyor needs maintenance performed before irreversible damage occurs when the value passes an alarm setpoint
## Publishing commands
Commands are requests for action to occur on another client.
Command topics follow the pattern below:

> cmd/[app]/[device]/[parameter-name]/[request-type]

As an example, the topic below contains a request to configure the acceleration time for VFD1:

> cmd/vfdctl/vfd1/acceltime/config

The message body should contain a json object with a "value" key and an integer value:

    {"value":40}

Publishing the sample object to the sample topic is expected to set the acceleration time of VFD1 to 40 seconds.
[Example of reading telemetry and publishing commands](https://drive.google.com/file/d/1uBgdtkvQJD8X0CEoMDGoNPIfBeuOHO-_/view?usp=sharing) 

## Configuring Parameters
### Controller Configuration
The controller's config file contains settings for the controller hardware connection, vfd hardware and MQTT topic mapping. This file is plain text and any notepad or command line software can edit it easily. 
After making edits, see "Setting up the Vfdctl controller device" for load instructions.
[Example of the config.txt, being viewed with VSCode.](https://drive.google.com/file/d/1VchL4qhVxX0zC7FRwbWw4XGCvVEfioZ_/view?usp=sharing)
# Important Resources
### Platform updates
Please make sure to bookmark the following pages, as they will provide you with important details on API outages, updates, and other news relevant to developers on the platform.
Check out our releases to see find the latest or a history of all released binaries
Visit the Tulsa Software Repositories to find related open-source codebases
In case of any document conflicts, assume the Vfdctl readme is most accurate
### MQTT reference material
MQTT is extremely flexible and not as rigidly enforced as communications such as RPC or REST communications. This leaves lots of room for one-off implementations if systems are not intentionally designed. For our communications architecture, we have based our topic and payload structures off of the AWS best practices to ensure ultimate usability.
- Even if not using AWS IoT cloud communications, their [MQTT communication patterns](https://docs.aws.amazon.com/whitepapers/latest/designing-mqtt-topics-aws-iot-core/mqtt-communication-patterns.html) are widely applicable
- A good client GUI like [MQTT Explorer](http://mqtt-explorer.com/) makes troubleshooting your system much easier but any app supporting MQTT 3.1.1 communications will suffice
- Using a [IoT Rules Engine](https://www.mphasis.com/content/dam/mphasis-com/global/en/home/case-study/application-service/ideas-in-action/Digital%20POV%20-%20Using%20Node-RED_Whitepaper.pdf) such as Node Red, AWS IoT Core, etc is essential to utilizing data with minimal coupling and maximal scalability
## Still have questions?
For the questions not covered here or to learn what we're currently working on, visit the [Tulsa Software slack channel](https://join.slack.com/t/tulsasoftware/shared_invite/zt-1gfuxtto0-OKwuAalzEK32Q_V_nONm5Q) to review topics, ask questions, and learn from others.
