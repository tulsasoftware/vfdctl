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
#include "src/ConfigurationManager.h"
#include "src/RemoteConnectionManager.h"

Config config;
char *filename = "config.txt";
String mqttMessage;
uint8_t lastSentReading = 0; //Stores last Input Reading sent to the broker
unsigned long lastMillis = 0; // The time at which the sensors were last read.

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
  errorCode = ConfigMgr.Load(filename, &config);
  if (errorCode != 0){
    Serial.println(ConfigMgr.GetError(errorCode));
    Serial.println(errorCode);
    return;
  }else
  {
    Serial.println("Success.");
    Serial.print("Mac address: ");
    // Serial.println(config.broker.broker_retry_interval_sec);
    char hexCar[2];
    sprintf(hexCar, "%02X", config.device.device_mac[0]);
    Serial.print(hexCar);
    sprintf(hexCar, "%02X", config.device.device_mac[1]);
    Serial.print(hexCar);
    sprintf(hexCar, "%02X", config.device.device_mac[2]);
    Serial.print(hexCar);
    sprintf(hexCar, "%02X", config.device.device_mac[3]);
    Serial.print(hexCar);
    sprintf(hexCar, "%02X", config.device.device_mac[4]);
    Serial.print(hexCar);
    sprintf(hexCar, "%02X", config.device.device_mac[5]);
    Serial.print(hexCar);
    Serial.println("");
  }

  Serial.println("Initializing modbus ...");
  while (!ModbusRTUClient.begin(9600)) {
    Serial.println(F("Failed to initialize RS485 RTU Client"));
  };
  Serial.println("Success.");

  Serial.println("Initializing remote connections ...");
  while (RemoteConnMgr.Init(config.broker, config.device) != 0);
  Serial.println("Success.");
  
}

void loop() {
  //ensure we have a broker connection before continuing
  int connectionErr = RemoteConnMgr.Connect();
  if (connectionErr < 0){
    Serial.println(RemoteConnMgr.GetError(connectionErr));
    delay(config.broker.broker_retry_interval_sec * 1000);
    return;
  }

  // If enough time has elapsed, read again.
  if (millis() - lastMillis > 10000) {
    lastMillis = millis();

  //Receive and update
  mqttMessage = "";
  int msgError = RemoteConnMgr.CheckForMessages(mqttMessage);  //Check for new messages
  if(msgError < 0)
  {
    if (msgError != -201)
    {
      // don't spam the console for not receiving a msg
      Serial.println(RemoteConnMgr.GetError(msgError));
    }
  }else
  {
    Serial.println("New messages:");
    Serial.println(mqttMessage);

    //TODO: take action on incoming requests
    // ModbusRTUClient.holdingRegisterWrite(1, 1080, 400);
  }
    //Scan monitored modbus telemetry registers, mqtt publish values
    for (ModbusParameter param : config.modbus.registers)
    {
      //abort when at the end of the defined registers
      if (param.address == 0){
        break;
      }

      //read
      Serial.print("Scanning next modbus param from configuration ");
      Serial.println("...");
      Serial.println(param.name);
      Serial.println(param.address);

      Serial.print("Beginning read ... ");
      int regValue = ModbusRTUClient.holdingRegisterRead(param.device_id, param.address - 1);
      if (regValue < 0) 
      {
        //move to next register
        Serial.print("failed to read register ");
        Serial.println(ModbusRTUClient.lastError());
      }
      else
      {
        //write the contents to the remote

        //store value in source to preserve last value read
        // config.modbus.registers[i].value = regValue;
        Serial.print("value = ");
        Serial.println(regValue);

        Serial.println("Serializing results...");
        //serialize the contents
        StaticJsonDocument<96> doc;
        String val;
        String topic = "";
        doc["name"] = param.name;
        doc["value"] = regValue;
        doc["units"] = param.units;
        serializeJson(doc, val);
        Serial.print("value: ");
        Serial.println(val);

        Serial.println("Assembling topic...");
        //ex: devices/vfd2/torque
        topic += "devices/vfd";
        topic += String(param.device_id);
        topic += "/";
        topic += param.topic;
        Serial.print("topic: ");
        Serial.println(topic);

        Serial.print("Publishing results...");
        int pubVal = RemoteConnMgr.Publish(val, topic);
        if(pubVal < 0)
        {
          Serial.print("failed to publish to remote. Error: ");
          Serial.println(pubVal);
        }
        Serial.println("done.");
      }

      //takes about 5 counts for RTU transaction to complete
      delay(5);
    }
  }
}