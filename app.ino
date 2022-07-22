/*  P1AM Example Series #1 - MQTT Call and Response
 * 
 *  This example uses an MQTT broker to update the outputs of a P1-08TRS
 *  to the inputs of a P1-08SIM. You can easily switch these out for any
 *  discrete input or output module.
 * 
 *  I used shiftr.io for this example. This is a free MQTT broker that provides
 *  nice visualisation and is great for testing. If you want to use a different broker,
 *  just update the broker string to the proper URL and update any login credentials
 *  
 *  Both the sending and receiving portions are included in this code, but these could
 *  easily be separated out into separate P1AM units or web interfaces. 
 *  
 *  With a little bit of modification, you can turn this example into a remote monitoring 
 *  solution or combine it with Modbus TCP and connect an aging PLC to the cloud.
 * 
 * 
 * 
 *  Written by FACTS Engineering
 *  Copyright (c) 2019 FACTS Engineering, LLC
 *  Licensed under the MIT license.
 */

#include <P1AM.h>
#include <ArduinoMqttClient.h>
#include <Ethernet.h>
#include <SD.h>

#define FILE_NAME "config.txt"

EthernetClient client;
MqttClient mqttClient(client);

byte mac[] = {0x60, 0x52, 0xD0, 0x06, 0x70, 0x27};  // P1AM-ETH have unique MAC IDs on their product label
const string url = SD_findString(F("broker_url))
const char broker[]    = "tulsasoftware.cloud.shiftr.io";  // MQTT Broker URL
int port = 1883;
uint8_t lastSentReading = 0; //Stores last Input Reading sent to the broker

File myFile;

void setup() {
  Serial.begin(115200);
  while(!P1.init());  //Wait for module sign-on

  Serial.println("Initializing SD card...");
  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1);
  }

  // open the file read-only. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open(FILE_NAME);

  if (myFile) {
    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }
  Serial.println("Read configuration done.");
  
  Ethernet.init(5);   //CS pin for P1AM-ETH
  Ethernet.begin(mac);  // Get IP from DHCP
  
  mqttClient.setUsernamePassword("tulsasoftware", "M4b8loRf87hgrhWh");  // Username and Password tokens for Shiftr.io namespace. These can be found in the namespace settings.
  Serial.print("Connecting to the MQTT broker: ");
  Serial.println(broker);
  while (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();

  mqttClient.subscribe("InputReading"); //Subscribe to "InputReading" topic
}

void loop() {
  //Sending Updates
  //uint8_t inputReading = P1.readDiscrete(SIM); //Check inputs right now
  uint8_t inputReading = 237;
  if(inputReading != lastSentReading){  // If the state doesn't match the last sent state, update the broker
    mqttClient.beginMessage("InputReading");  //Topic name
    mqttClient.print(inputReading); //Value to send
    mqttClient.endMessage();
    lastSentReading = inputReading; //Update our last sent reading
    Serial.println("Sent " + (String)inputReading + " to broker");
  }

  //Receiving updates
  int mqttValue = checkBroker();  //Check for new messages
  if(mqttValue != -1){  // -1 means we didn't get a new message
    P1.writeDiscrete(mqttValue, TRS); //If we get a new message update the TRS
  }
}

int checkBroker(){
  String mqttMessage = "";
  int messageValue = 0;

  int messageSize = mqttClient.parseMessage();
  if (messageSize) {
    // we received a message, print out the topic and contents
    Serial.print("Received a message with topic ");
    Serial.println(mqttClient.messageTopic());
    if(mqttClient.messageTopic() == "InputReading"){
      while (mqttClient.available()){
         mqttMessage +=(char)mqttClient.read(); //Add all message characters to the string
      }     
      messageValue =  mqttMessage.toInt();  //convert ascii string to integer value
    }
  }
  else{
    messageValue =  -1; //If we didn't receive anything set our value to -1. Since our SIM 
                        //value can't be -1, this makes it easy to filter out later
  }
  return messageValue;
}
