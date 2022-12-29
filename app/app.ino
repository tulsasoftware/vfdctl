/*  P1AM Example Series #1 - MQTT Call and Response
 * 
 *  This application monitors a Modbus network and translates them into device status changed events
 *  over MQTT for each of the monitored hardware properties
 *
 *  A config.txt must exist at the root of the SD card with values matching the Config object.
 *  This file should be used for any type of dynamic configuration after deployment.
 * 
 *  I used hivemq as a broker for testing, this is a free MQTT broker that is publicly shared.
 *  If you want to use a different broker, just update the broker URL 
 *  found inside config.txt on the Arduino's SD card
 * 
 */

#include <P1AM.h>
#include <ArduinoModbus.h>
#include <ArduinoJson.h>
#include "src/Message.h"
#include <cppQueue.h>
#include "src/ConfigurationManager.h"
#include "src/RemoteConnectionManager.h"

Config* config = new Config{};
char *filename = "config.txt";
String mqttMessage;
uint8_t lastSentReading = 0; //Stores last Input Reading sent to the broker
unsigned long lastMillis = 0; // The time at which the sensors were last read.
//queue stores cmd messages to be published - messages in queue are overwritten to prevent duplication
// DynamicJsonDocument cmd(512);
//can queue up to 5 commands
Message *cmds[5];
Message *cmd;
cppQueue cmdQ(sizeof(cmds), 20, FIFO, true);

void setup() {
  //initialize values
  int errorCode = 0;

  Serial.begin(115200);
  Serial.println(F("Freshly booted, welcome aboard"));

  while(!P1.init());  //Wait for module sign-on

  Serial.println(F("Initializing SD hardware..."));
  //P1AM shares an SS pin with Ethernet - reset mode of pin to guarantee setup
  while(ConfigMgr.Init(true, SDCARD_SS_PIN) != 0);
  Serial.println("Success.");

  // Should load default config if run for the first time
  Serial.println(F("Loading configuration..."));
  errorCode = ConfigMgr.Load(filename, config);
  Serial.println("Config register 9 comparison:");
  Serial.println(config->modbus.configuration_registers[10].name);
  Serial.println(config->modbus.configuration_registers[10].limit_comparison);

  if (errorCode != 0){
    Serial.println(ConfigMgr.GetError(errorCode));
    Serial.println(errorCode);
    return;
  }else
  {
    Serial.println("Success.");
    Serial.print("Mac address: ");
    // Serial.println(config->broker.broker_retry_interval_sec);
    char hexCar[2];
    sprintf(hexCar, "%02X", config->device.device_mac[0]);
    Serial.print(hexCar);
    sprintf(hexCar, "%02X", config->device.device_mac[1]);
    Serial.print(hexCar);
    sprintf(hexCar, "%02X", config->device.device_mac[2]);
    Serial.print(hexCar);
    sprintf(hexCar, "%02X", config->device.device_mac[3]);
    Serial.print(hexCar);
    sprintf(hexCar, "%02X", config->device.device_mac[4]);
    Serial.print(hexCar);
    sprintf(hexCar, "%02X", config->device.device_mac[5]);
    Serial.print(hexCar);
    Serial.println("");
  }

  Serial.println("Initializing modbus ...");
  while (!ModbusRTUClient.begin(9600)) {
    Serial.println(F("Failed to initialize RS485 RTU Client"));
  };
  Serial.println("Success.");

  Serial.println("Initializing remote connections ...");
  while (RemoteConnMgr.Init(config->broker, config->device) != 0);
  Serial.println("Success.");
  
  Serial.print("Setup message received events ...");
  RemoteConnMgr.RegisterOnMessageReceivedCallback(messageReceived);
  Serial.println("Success.");
}

void messageReceived(String &topic, String &payload) {
  Serial.println("new message!");
  Serial.println("incoming: " + topic + " - " + payload);
  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.

  //TODO: Add into a dictionary<topic, body> and then serialize later to prevent dropping msgs

  Message *msg = new Message();
  //remove 'cmd/' from beginning
  msg->topic = topic;
  msg->body = payload;

  cmdQ.push(&msg);
  Serial.println("message sent to queue");
}

/// @brief 
/// @return 0 = success, -1 = unrecognized command, -2 
int processCommandQueue(){
  if (!cmdQ.isEmpty())
  {
    Serial.println("processing queue");
    Message *cmd = new Message();
    if (cmdQ.pop(&cmd))
    {
      Serial.println("found message ");
      Serial.println(cmd->topic);
      Serial.println(cmd->body);

      if (!cmd->topic.endsWith("/config")){
        Serial.println("unrecognized command was requested, message ignored");
        return -1;
      }

      //check other assumptions are true?

      Serial.println("Deserializing content...");
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, cmd->body);
      Serial.println("Deserializing content... Done.");

      if (error)
      {
        Serial.println("unable to deserialize, message ignored");
        return -5;
      }else
      {
        Serial.print("reading deserialized value: ");
        int val;
        if (doc.containsKey("value")){
          int val = doc["value"];
          Serial.println(val);
        }else{
          Serial.print("property missing in command ; 'value' : 123");
          return -7;
        }

        Serial.println("Looking for parameter...");
        //lookup object from config
        ModbusConfigParameter p = ConfigMgr.GetParameter(cmd->topic, config);

        //ensure requested values are within limits
        Serial.println("Checking value within range for parameter");
        //Serial.println(ConfigMgr.toString(p.limit_comparison));

        int inRange = isWithinRange(p.lower_limit, p.upper_limit, val, p.limit_comparison);
        switch (inRange)
        {
          case 0:
            Serial.println("Requested value not within allowed range");
            return -2;
            break;
          case 1:
            Serial.println("Requested value is within allowed range");
            Serial.println("Writing value to register...");
            Serial.println(p.name);
            Serial.println(p.address);
            Serial.println(val);
            
            //modbus client writes off by 1
            int writeRes;
            writeRes = ModbusRTUClient.holdingRegisterWrite(p.device_id, p.address-1, val);

            if (writeRes > 0){
              return 1;
            }else{
              return -3;
            }
            //TODO: echo the ack if requested
            break;
          default:
            Serial.println("Error determining if requested value is within range");
            return -4;
            break;
        }
      }
    }
  }
  else{
    return 0;
  }
}

/// @brief Judge if a value is within the user's limits
/// @param lower Lower comparison limit
/// @param upper Upper comparison limit
/// @param value Value to compare
/// @param eComparison How to execute comparison
/// @return 0 = false, 1 = true, < 0 = error
int isWithinRange(int lower, int upper, int value, eLimitComparison eComparison){
  int retVal = 0;
  Serial.println("Comparison mode:");
  Serial.println(eComparison);
  switch (eComparison)
  {
    case eLimitComparison::none:
      retVal = 1;
      break;
    case eLimitComparison::between:
      if ((value > lower) && (value < upper)){
        retVal = 1;
      }
      break;
    case eLimitComparison::between_or_equal:
      if ((value >= lower) && (value <= upper)){
        retVal = 1;
      }
    case eLimitComparison::less_than:
      if ((value < upper)){
        retVal = 1;
      }
    case eLimitComparison::less_than_or_equal:
      if ((value <= upper)){
        retVal = 1;
      }
    case eLimitComparison::greater_than:
      if ((value > lower)){
        retVal = 1;
      }
    case eLimitComparison::greater_than_or_equal:
      if ((value < lower)){
        retVal = 1;
      }
    default:
      retVal = -1;
      break;
  }
  return retVal;
}

void loop() {
  //ensure we have a broker connection before continuing
  int connectionErr = RemoteConnMgr.Connect();
  if (connectionErr < 0){
    //flash failed connection led pattern
    Serial.println(RemoteConnMgr.GetError(connectionErr));
    delay(config->broker.broker_retry_interval_sec * 1000);
    setup();
    return;
  }

  //process any incoming commands before reporting telemetry

  //triple flash when beginning to process a command
  if (processCommandQueue() < 0){
    Serial.println("Failed to process a command request");
  }

  // if enough time has elapsed, read again.
  if (millis() - lastMillis > 10000) {
    lastMillis = millis();

    //scan monitored modbus telemetry registers, mqtt publish values
    publishTelemetry();
  }
}

int publishTelemetry(){
  for (size_t i = 0; i < 50; i++)
    {
      // Serial.println("Scanning next modbus param from configuration ...");
      ModbusParameter param = config->modbus.registers[i];

      //abort early if at the end of the defined registers
      if (param.address == 0){
        Serial.println("Values published");
        break;
      }
      // Serial.println(param.name);
      // Serial.println(param.address);

      // Serial.print("Beginning read ... ");
      int regValue = ModbusRTUClient.holdingRegisterRead(param.device_id, param.address - 1);
      if (regValue < 0) 
      {
        //move to next register
        Serial.print("failed to read register ");
        Serial.print(40000 + param.address - 1);
        Serial.print(" ; ");
        Serial.println(ModbusRTUClient.lastError());
      }
      else
      {
        //write the contents to the remote

        //store value in source to preserve last value read
        // config->modbus.registers[i].value = regValue;
        // Serial.print("value = ");
        // Serial.println(regValue);

        // Serial.println("Serializing results...");
        //serialize the contents
        StaticJsonDocument<96> doc;
        String val;
        String topic = "";
        doc["name"] = param.name;
        doc["value"] = regValue;
        doc["units"] = param.units;
        serializeJson(doc, val);
        // Serial.print("value: ");
        // Serial.println(val);

        // Serial.println("Assembling topic...");
        //ex: devices/vfd2/torque
        topic += param.topic;
        // Serial.print("topic: ");
        // Serial.println(topic);

        // Serial.print("Publishing results...");
        int pubVal = RemoteConnMgr.Publish(val, topic);
        if(pubVal < 0)
        {
          Serial.print("failed to publish to remote. Error: ");
          Serial.println(pubVal);
        }
        // Serial.println("done.");
      }

      //takes about 5 counts for RTU transaction to complete
      delay(5);
    }
    return 0;
}