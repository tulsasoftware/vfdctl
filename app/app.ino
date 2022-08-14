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
#include <Vector.h>
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

  // Serial.begin(115200);
  //   Serial.println("Initializing modbus ...");
  while (!ModbusRTUServer.begin(1, 9600)) {
    Serial.println(F("Failed to initialize RS485 RTU Server"));
  };
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


    if (ModbusRTUServer.configureCoils(0x00, 10) == 0){
    Serial.println("Success.");
  }else{
    Serial.println("Failed coil configuration.");
  }
  ///only monitor register 999
  if (ModbusRTUServer.configureHoldingRegisters(0x00, 10) == 0){
    Serial.println("Success.");
  }else{
    Serial.println("Failed register configuration.");
  }

  Serial.println("Initializing remote connections ...");
  while (RemoteConnMgr.Init(config.broker, config.device) != 0);
  Serial.println("Success.");
  
}

// long holdRegisterRead(int NodeNum, int Address) {
  
//   long RegisterOut;
  
//   RegisterOut = ModbusRTUServer.holdingRegisterRead(Address);
//   delay(5);
  
//   return RegisterOut;
// }
long torque = 0;
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

  //Receive and mqtt updates
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
  }

  //Scan modbus registers

    Serial.println("Torque Boost Reading...");
    ModbusRTUServer.poll();


    // int tb;
    // tb = holdRegisterRead(2, 999);    // Torque boost parameter
    long tb1 = ModbusRTUServer.holdingRegisterRead(0);
    Serial.print("Torque Boost 1: ");
    Serial.println(tb1);
    long tb2 = ModbusRTUServer.holdingRegisterRead(1);
    Serial.print("Torque Boost 2: ");
    Serial.println(tb2);
    long tb3 = ModbusRTUServer.holdingRegisterRead(2);
    Serial.print("Torque Boost 3: ");
    Serial.println(tb3);

    // if (!ModbusRTUClient.requestFrom(1, HOLDING_REGISTERS, 999, 1)) {
    //   Serial.print("failed to read registers! ");
    //   Serial.println(ModbusRTUClient.lastError());
    // }
    // else {
    //   // If the request succeeds, the sensor sends the readings, that are
    //   // stored in the holding registers. The read() method can be used to
    //   // get the raw temperature and the humidity values.
    //   long rawtemperature = ModbusRTUClient.read();
    //   // short rawhumidity = ModbusRTUClient.read();
    //   Serial.print("Torque: ");
    //   Serial.println(rawtemperature);
  
    //   // // The raw values are multiplied by 10. To get the actual
    //   // // value as a float, divide it by 10.
    //   // float temperature = rawtemperature / 10.0;
    //   // float humidity = rawhumidity / 10.0;
    //   // Serial.print("Max Freq: ");
    //   // Serial.println(humidity);
      
    // }
    
    delay(100);

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


}