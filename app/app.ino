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
#include <ArduinoMqttClient.h>
#include <Ethernet.h>
#include <ArduinoModbus.h>
#include <ConfigurationManager.h>

EthernetClient client;
MqttClient mqttClient(client);

ConfigurationManager configMgr(SDCARD_SS_PIN);
Config &config;
char *filename = "conf.txt";

byte mac[] = {0x60, 0x52, 0xD0, 0x06, 0x70, 0x27};  // P1AM-ETH have unique MAC IDs on their product label
uint8_t lastSentReading = 0; //Stores last Input Reading sent to the broker

void setup() {
  int errorCode = 0;
  Serial.begin(115200);
  while(!P1.init());  //Wait for module sign-on

  //P1AM shares an SS pin with Ethernet - reset mode of pin to guarantee setup
  while(!configMgr.init(true));

  // Should load default config if run for the first time
  Serial.println(F("Loading configuration..."));
  errorCode = configMgr.load(filename, config);
  if (errorCode < 0){
    Serial.println(GetError(errorCode));
    return;
  }

  while (!ModbusRTUClient.begin(9600)) {
    Serial.println(F("Failed to initialize RS485 RTU Client"));
  }
  
  Ethernet.init(5);   //CS pin for P1AM-ETH
  Ethernet.begin(mac);  // Get IP from DHCP

  if (Connect(config) != 0){
    Serial.println(F("Failed to connect"));
  }
  
}

char* GetError(int code){
  if (code == -10){
    return "unable to load configuration file.";
  }
}

int Connect(Config configuration){

  if (mqttClient.connected()){
    return 0;
  }
  Serial.print("Connecting to the MQTT broker: ");
  Serial.println(config.broker_url);
  mqttClient.setUsernamePassword(config.broker_user, config.broker_pass);  // Username and Password tokens for Shiftr.io namespace. These can be found in the namespace settings.
  if (!mqttClient.connect(config.broker_url, config.broker_port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    return -1;
  }
  else{
    Serial.println("Connected to the MQTT broker");
  }

  mqttClient.subscribe("modbus/track");
  return 0;
}

void loop() {
  //ensure we have a broker connection before continuing
  if (Connect(config) != 0){
    Serial.println("Connection error");
    delay(5000);
    return;
  }

  //Receive and  updates
  int mqttValue = checkBroker();  //Check for new messages
  if(mqttValue != -1){  // -1 means we didn't get a new message
    Serial.println("Processed new messages");
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

int checkBroker(){
  String mqttMessage = "";
  int messageValue = 0;

  int messageSize = mqttClient.parseMessage();
  if (messageSize) {
    // we received a message, print out the topic and contents
    Serial.print("Received a message with topic ");
    Serial.println(mqttClient.messageTopic());
    if(mqttClient.messageTopic() == "modbus/track"){
      while (mqttClient.available()){
         mqttMessage +=(char)mqttClient.read(); //Add all message characters to the string
      }     
      Serial.println(mqttMessage);
      
      messageValue =  0;
    }
  }
  else{
    messageValue =  -1; // bad value
  }
  return messageValue;
}
