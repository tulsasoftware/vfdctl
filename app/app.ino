/*  P1AM Example Series #1 - MQTT Call and Response
 * 
 *  This application monitors a Modbus network and translates them into device status changed events
 *  over MQTT for each of the monitored hardware properties
 *
 *  A config.json must exist at the root of the SD card with values matching the Config object.
 *  This file should be used for any type of dynamic configuration after deployment.
 * 
 *  I used shiftr.io for this example. This is a free MQTT broker that provides
 *  nice visualisation and is great for testing. If you want to use a different broker,
 *  just update the broker string to the proper URL and update any login credentials
 * 
 */

#include <P1AM.h>
#include <ArduinoModbus.h>
#include "src/ConfigurationManager.h"
#include "src/RemoteConnectionManager.h"

Config config;
char *filename = "config.txt";
String mqttMessage;
byte mac[] = {0x60, 0x52, 0xD0, 0x06, 0x70, 0x27};  // P1AM-ETH have unique MAC IDs on their product label
uint8_t lastSentReading = 0; //Stores last Input Reading sent to the broker

void setup() {
  //initialize values
  int errorCode = 0;

  Serial.begin(115200);
  Serial.println(F("Freshly booted, welcome aboard"));

  while(!P1.init());  //Wait for module sign-on

  Serial.println(F("Initializing SD hardware..."));
  //P1AM shares an SS pin with Ethernet - reset mode of pin to guarantee setup
  while(ConfigMgr.Init(true, SDCARD_SS_PIN) != 0);

  // Should load default config if run for the first time
  Serial.println(F("Loading configuration..."));
  errorCode = ConfigMgr.Load(filename, &config);
  if (errorCode != 0){
    Serial.println(ConfigMgr.GetError(errorCode));
    Serial.println(errorCode);
    return;
  }

  Serial.println("Initializing modbus ...");
  while (!ModbusRTUClient.begin(9600)) {
    Serial.println(F("Failed to initialize RS485 RTU Client"));
  };
  
  Serial.println("Connecting to remote ...");
  if (RemoteConnMgr.Init(config.broker, config.device) < 0){
    Serial.println(F("Failed to connect"));
  }
  
}

void loop() {
  //ensure we have a broker connection before continuing
  int connectionErr = RemoteConnMgr.Connect();
  if (connectionErr < 0){
    Serial.println(ConfigMgr.GetError(connectionErr));
    delay(config.broker.broker_retry_interval_sec);
    return;
  }

  //Receive and updates
  mqttMessage = "";
  int msgError = RemoteConnMgr.CheckForMessages(mqttMessage);  //Check for new messages
  if(msgError < 0)
  {
    Serial.println(ConfigMgr.GetError(msgError));
    return;
  }else
  {
    Serial.println("New messages:");
    Serial.println(mqttMessage);
  }
  
  //Scan modbus registers

  //Compare modbus registers to existing values if delta

  //Publish modbus values for tracked items
  //Sending Updates
  //uint8_t inputReading = P1.readDiscrete(SIM); //Check inputs right now
  // uint8_t inputReading = 237;
  // if(inputReading != lastSentReading){  // If the state doesn't match the last sent state, update the broker
  //   mqttClient.beginMessage("InputReading");  //Topic name
  //   mqttClient.print(inputReading); //Value to send
  //   mqttClient.endMessage();
  //   lastSentReading = inputReading; //Update our last sent reading
  //   Serial.println("Sent " + (String)inputReading + " to broker");
  // }

}