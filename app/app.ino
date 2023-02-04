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
unsigned long lastStatusMillis = 0; // The time at which the sensors were last read.
int telemetryFrequency = 15000;
//queue stores cmd messages to be published - messages in queue are overwritten to prevent duplication
// DynamicJsonDocument cmd(512);
//can queue up to 5 commands
cppQueue cmdQ(sizeof(Message), 20, FIFO, true);
int errorCode = 0;

void setup() {
  Serial.begin(115200);
  Serial.println(F("Freshly booted, welcome aboard"));

  //Wait for module sign-on
  while(!P1.init());
  //status led
  pinMode(LED_BUILTIN, OUTPUT);
  //startup sequence beginning flash
  pulseStatus(false, 10);

  Serial.println(F("Initializing SD hardware..."));
  //P1AM shares an SS pin with Ethernet - reset mode of pin to guarantee setup
  // while(ConfigMgr.Init(true, SDCARD_SS_PIN) != 0);
  errorCode = ConfigMgr.Init(true, SDCARD_SS_PIN);
  if (errorCode < 0){
    errorCode = -13;
    return;
  }else{
    Serial.println("Success.");
  }

  // Should load default config if run for the first time
  Serial.println(F("Loading configuration..."));
  errorCode = ConfigMgr.Load(filename, config);
  if (errorCode < 0){
    errorCode = -14;
    return;
  }else{
    Serial.println("Success.");
  }
  Serial.println("Config register 9 comparison:");
  Serial.println(config->modbus.configuration_registers[10].name);
  Serial.println(config->modbus.configuration_registers[10].limit_comparison);

  Serial.println("Success.");
  Serial.print("Mac address: ");
  // Serial.println(config->broker.broker_retry_interval_sec);
  char hexCar[2];
  sprintf(hexCar, "%02X", config->device.device_mac.b1);
  Serial.print(hexCar);
  sprintf(hexCar, "%02X", config->device.device_mac.b2);
  Serial.print(hexCar);
  sprintf(hexCar, "%02X", config->device.device_mac.b3);
  Serial.print(hexCar);
  sprintf(hexCar, "%02X", config->device.device_mac.b4);
  Serial.print(hexCar);
  sprintf(hexCar, "%02X", config->device.device_mac.b5);
  Serial.print(hexCar);
  sprintf(hexCar, "%02X", config->device.device_mac.b6);
  Serial.print(hexCar);
  Serial.println("");

  Serial.println("Initializing modbus ...");
  errorCode = ModbusRTUClient.begin(config->modbus.serial_port.baud_rate);
  if (errorCode < 0){
    errorCode = -15;
    return;
  }else{
    Serial.println("Success.");
  }

  Serial.println("Initializing remote connections ...");
  errorCode = RemoteConnMgr.Init(config->broker, config->device);
  if (errorCode < 0){
    errorCode = -4;
    return;
  }else{
    Serial.println("Success.");
  }

  telemetryFrequency = config->modbus.telemetry_interval_sec * 1000;

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

  Message msg = Message();
  //remove 'cmd/' from beginning
  msg.topic = topic;
  msg.body = payload;

  cmdQ.push(&msg);
  Serial.println("message sent to queue");
  pulseStatus(false, 3);
}

/// @brief 
/// @return 0 = success, -1 = unrecognized command
int processCommandQueue(){
  if (!cmdQ.isEmpty())
  {
    Serial.println("processing queue");
    Message cmd = Message();
    if (cmdQ.pop(&cmd))
    {
      Serial.println("found message ");
      Serial.println(cmd.topic);
      Serial.println(cmd.body);

      //mqtt subscription overlapped with another
      if (!cmd.topic.endsWith("/config")){
        Serial.println("unrecognized command was requested, message ignored");
        return 0;
      }

      //content checking on cmd
      int numSlashes = 0; 
      for(auto x : cmd.topic)
      {
        Serial.println(x);
        if (x == '/' || x == '\\'){
          numSlashes++;
        }
      }
      if (numSlashes != 4){
        Serial.println("requested command topic is not formatted properly");
        return -6;
      }

      //check other assumptions are true?

      Serial.println("Deserializing content...");
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, cmd.body);
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
          return -6;
        }

        Serial.println("Looking for parameter...");
        //lookup object from config
        ModbusConfigParameter p = ConfigMgr.GetParameter(cmd.topic, config);

        if(!p.formed){
          Serial.print("Unable to find a matching command in config. Ignoring message");
          return -6;
        }

        //ensure requested values are within limits
        Serial.println("Checking value within range for parameter");
        //Serial.println(ConfigMgr.toString(p.limit_comparison));

        int inRange = isWithinRange(p.lower_limit, p.upper_limit, val, p.limit_comparison);
        switch (inRange)
        {
          case 0:
            Serial.println("Requested value not within allowed range");
            return -9;
            break;
          case 1:
            Serial.println("Requested value is within allowed range");
            Serial.println("Writing value to register...");
            Serial.println(p.name);
            Serial.println(p.address);
            Serial.println(val);
            
            //modbus client writes off by 1
            int writeRes;
            writeRes = ModbusRTUClient.holdingRegisterWrite(p.device_id, p.address, val);

            if (writeRes > 0){
              return 3;
            }else{
              return -3;
            }
            //TODO: echo the ack if requested
            break;
          default:
            Serial.println("Error determining if requested value is within range");
            return -8;
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
  //status LED freq should always be smaller than telem freq
  int statusLedFrequency = min(10000, telemetryFrequency);

  //give a status update every few seconds or when an error is present
  if (millis() - lastStatusMillis > statusLedFrequency || errorCode != 0) {
    lastStatusMillis = millis();

    //display existing results before updating
    //good running condition gives no error and 0 flashes
    blinkStatus(errorCode < 0, errorCode);

    //if we encountered an error, setup is expected to resolve it
    if (errorCode < 0){
      delay(1000);
      setup();
    }
  }

  //maintain connection / callbacks
  if (RemoteConnMgr.Connect() < 0){
    errorCode = -4;
  }else{
    //process any incoming commands before reporting telemetry
    //if an error condition exists on the controller, allow it to reset before popping queue
    errorCode = processCommandQueue();
  }

  // if enough time has elapsed, publish telemetry again.
  if (millis() - lastMillis > telemetryFrequency) {
    lastMillis = millis();

    if(errorCode >= 0){
      errorCode = publishTelemetry();
    }
  }
}

int publishTelemetry(){
  Serial.print(" publishing..");
  for (size_t i = 0; i < 50; i++)
    {
      // Serial.println("Scanning next modbus param from configuration ...");
      ModbusParameter param = config->modbus.registers[i];

      //abort early if at the end of the defined registers
      if (param.formed == false){
        break;
      }
      // Serial.println(param.name);
      // Serial.println(param.address);

      // Serial.print("Beginning read ... ");
      int regValue = ModbusRTUClient.holdingRegisterRead(param.device_id, param.address);
      if (regValue < 0) 
      {
        //move to next register
        Serial.print("failed to read register ");
        Serial.print(40000 + param.address);
        Serial.print(" ; ");
        Serial.println(ModbusRTUClient.lastError());
        return -7;
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
          return -7;
        }
        // Serial.println("done.");
      }

      //takes about 5 counts for RTU transaction to complete
      delay(5);
    }
    return 2;
}

void blinkStatus(bool isError, int errorCode){
  //accept positive or negative codes
  int num = abs(errorCode);
  //track the start state to return it when complete
  bool startState = isError;
  //errors blink slower for troubleshooting
  int duration;
  if (isError){
    duration = 1000;
  }else{
    duration = 300;
  }

  for (size_t i = 0; i < num*2; i++)
  {
    //pull the led opposite of current state, wait and toggle it
    //takes two cycles for a single "blink"
    digitalWrite(LED_BUILTIN, !isError);
    delay(duration);
    isError = !isError;
  }
  
  //return led to original state
  digitalWrite(LED_BUILTIN, startState);
}

void pulseStatus(bool isError, int errorCode){
  //accept positive or negative codes
  int num = abs(errorCode);
  //track the start state to return it when complete
  bool startState = isError;
  //errors blink slower for troubleshooting
  int duration;
  duration = 100;

  for (size_t i = 0; i < num*2; i++)
  {
    //pull the led opposite of current state, wait and toggle it
    //takes two cycles for a single "blink"
    digitalWrite(LED_BUILTIN, !isError);
    delay(duration);
    isError = !isError;
  }
  
  //return led to original state
  digitalWrite(LED_BUILTIN, startState);
}

void setStatus(bool isError){
  digitalWrite(LED_BUILTIN, isError);
}