#ifndef RemoteConnectionManager_h
#define RemoteConnectionManager_h

#include "Arduino.h"
#include "ConfigurationManager.h"
#include <Ethernet.h>
#include <MQTT.h>

enum class RemoteConnectionErrors 
{
  SUCCESS,
  BROKER_FAILED_CONNECT = -100,
  UNREADABLE_MESSAGE = -200,
  NO_MESSAGES_AVAILABLE = -201,
  HARDWARE_FAILURE = -300,
  ETHERNET_INITIALIZATION_FAILURE,
  MQTT_INITIALIZATION_FAILURE,
};

class RemoteConnectionManager
{
  public:
    RemoteConnectionManager();
    int Init(BrokerConfiguration remConfig, DeviceConfiguration devConfig);
    //contact the remote
    int Connect();
    //return any messages found
    int CheckForMessages(String message);
    //get a readable error msg
    char* GetError(int code);
    //publish a remote message
    int Publish(String message, String topic);
  private:
    BrokerConfiguration _remConfig;
    DeviceConfiguration _devConfig;
    uint8_t _ethernetMac[6];
};

extern RemoteConnectionManager RemoteConnMgr;	//Default class instance

#endif