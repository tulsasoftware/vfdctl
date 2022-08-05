#include <Arduino.h>
#include <RemoteConnectionManager.h>
#include <ConfigurationManager.h>
#include <ArduinoMqttClient.h>

EthernetClient client;
MqttClient mqttClient(client);

RemoteConnectionManager::RemoteConnectionManager(BrokerConfiguration config, DeviceConfiguration dev)
{
    _remConfig = config;
    _devConfig = dev;
    char* ptr; //start and end pointer for strtol

    //unpack char* mac into byte array
    _ethernetMac[0] = strtol(_devConfig.device_mac, &ptr, HEX );
    for( uint8_t i = 1; i < 6; i++ )
    {
        _ethernetMac[i] = strtol(ptr+1, &ptr, HEX );
    }
}

int RemoteConnectionManager::Init()
{
    Ethernet.init(_devConfig.ethernet_pin);   //CS pin for P1AM-ETH
    Ethernet.begin(_ethernetMac);  // Get IP from DHCP
}

int RemoteConnectionManager::Connect()
{
    if (mqttClient.connected()){
    return 0;
    }
    Serial.print("Connecting to the MQTT broker: ");
    Serial.println(_remConfig.broker_url);
    mqttClient.setUsernamePassword(_remConfig.broker_user, _remConfig.broker_pass);  // Username and Password tokens for Shiftr.io namespace. These can be found in the namespace settings.
    if (!mqttClient.connect(_remConfig.broker_url, _remConfig.broker_port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    return RemoteConnectionErrors::BROKER_FAILED_CONNECT;
    }
    else{
    Serial.println("Connected to the MQTT broker");
    }

    //perform subscriptions
    mqttClient.subscribe("modbus/track");
    return RemoteConnectionErrors::SUCCESS;
}

int RemoteConnectionManager::CheckForMessages(String mqttMessage)
{
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

char* GetError(int code)
{
    char* val;
    switch (code)
    {
        case SUCCESS:
            val = "success";
            break;
        case RemoteConnectionErrors::BROKER_FAILED_CONNECT:
            val = "unable to initialize SD card reader, check your ss pin is correct and if you require a line reset";
            break;
        case RemoteConnectionErrors::UNREADABLE_MESSAGE:
            val = "unable to parse received message from topic";
            break;
        default:
            val = "unrecognized error";
            break;
    }

    return val;
}