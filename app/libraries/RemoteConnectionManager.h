/*
  RemoteConnectionManager.h - Library for flashing RemoteConnectionManager code.
  Created by David A. Mellis, November 2, 2007.
  Released into the public domain.
*/
#ifndef RemoteConnectionManager_h
#define RemoteConnectionManager_h

#include "Arduino.h"
#include <Ethernet.h>
#include <ConfigurationManager.h>

enum RemoteConnectionErrors 
{
    SUCCESS,
    BROKER_FAILED_CONNECT = -100,
    UNREADABLE_MESSAGE = -200,
};

class RemoteConnectionManager
{
  public:
    RemoteConnectionManager(BrokerConfiguration remConfig, DeviceConfiguration devConfig);
    int Init();
    //contact the remote
    int Connect();
    //return any messages found
    int CheckForMessages(String message);
    //get a readable error msg
    char* GetError(int code);
  private:
    BrokerConfiguration _remConfig;
    DeviceConfiguration _devConfig;
    uint8_t _ethernetMac[6];
};

#endif