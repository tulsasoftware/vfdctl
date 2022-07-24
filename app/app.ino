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
 * 
 */

#include <P1AM.h>
#include <ArduinoMqttClient.h>
#include <Ethernet.h>
#include <ArduinoJson.h>
#include <SD.h>

//TODO: break this into multiple structs for each object being configured
// Our configuration structure.
//
// Never use a JsonDocument to store the configuration!
// A JsonDocument is *not* a permanent storage; it's only a temporary storage
// used during the serialization phase. See:
// https://arduinojson.org/v6/faq/why-must-i-create-a-separate-config-object/
struct Config {
  char device_mac[6];
  char device_name[16];
  char broker_user[32];
  char broker_pass[32];
  char broker_url[64];
  int  broker_port;
};

EthernetClient client;
MqttClient mqttClient(client);

Config config;
const char *filename = "config.json";

byte mac[] = {0x60, 0x52, 0xD0, 0x06, 0x70, 0x27};  // P1AM-ETH have unique MAC IDs on their product label
char broker[64]    = "tulsasoftware.cloud.shiftr.io";  // MQTT Broker URL
int port = 0;
uint8_t lastSentReading = 0; //Stores last Input Reading sent to the broker

void setup() {
  Serial.begin(115200);
  while(!P1.init());  //Wait for module sign-on
  Serial.println("Hello, old friend.");

  // Initialize SD library
  while (!SD.begin()) {
    Serial.println(F("Failed to initialize SD library"));
  }

  // Should load default config if run for the first time
  Serial.println(F("Loading configuration..."));
  loadConfiguration(filename, config);

  // Dump config file
  //Serial.println(F("Printing config file..."));
  //printFile(filename);
  
  Ethernet.init(5);   //CS pin for P1AM-ETH
  Ethernet.begin(mac);  // Get IP from DHCP
  
  mqttClient.setUsernamePassword("tulsasoftware", "M4b8loRf87hgrhWh");  // Username and Password tokens for Shiftr.io namespace. These can be found in the namespace settings.
  Serial.print("Connecting to the MQTT broker: ");
  Serial.println(config.broker_url);
  if (!mqttClient.connect(config.broker_url, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
  }
  else{
    Serial.println("You're connected to the MQTT broker!");
    Serial.println();
  }



  mqttClient.subscribe("InputReading"); //Subscribe to "InputReading" topic
}

// Loads the configuration from a file
void loadConfiguration(const char *filename, Config &config) {

  File root = SD.open("/");
  printDirectory(root, 0);

  Serial.println("Opening config file...");
  // Open file for reading
  File file = SD.open(filename);

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<384> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    Serial.println(F("Failed to read file, using default configuration"));

  // Copy values from the JsonDocument to the Config
  strlcpy(config.broker_url,
          doc["broker_url"] | "none",
          sizeof(config.broker_url));
  strlcpy(config.broker_user,                  // <- destination
        doc["broker_user"] | "",  // <- source
        sizeof(config.broker_user));         // <- destination's capacity
  config.broker_port = doc["port"] | 1883;

  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();
}

void printDirectory(File dir, int numTabs) {

  while (true) {

    File entry =  dir.openNextFile();

    if (! entry) {

      // no more files

      break;

    }

    for (uint8_t i = 0; i < numTabs; i++) {

      Serial.print('\t');

    }

    Serial.print(entry.name());

    if (entry.isDirectory()) {

      Serial.println("/");

      printDirectory(entry, numTabs + 1);

    } else {

      // files have sizes, directories do not

      Serial.print("\t\t");

      Serial.println(entry.size(), DEC);

    }

    entry.close();

  }
}

// Prints the content of a file to the Serial
void printFile(const char *filename) {
  // Open file for reading
  File file = SD.open(filename);
  if (!file) {
    Serial.println(F("Failed to read file"));
    return;
  }

  // Extract each characters by one by one
  while (file.available()) {
    Serial.print((char)file.read());
  }
  Serial.println();

  // Close the file
  file.close();
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
    Serial.println("No new messages");
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
