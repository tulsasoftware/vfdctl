/*
  ConfigurationManager.h - Library for flashing ConfigurationManager code.
  Created by David A. Mellis, November 2, 2007.
  Released into the public domain.
*/
#ifndef ConfigurationManager_h
#define ConfigurationManager_h

#include "Arduino.h"

// Never use a JsonDocument to store the configuration!
// A JsonDocument is *not* a permanent storage; it's only a temporary storage
// used during the serialization phase. See:
// https://arduinojson.org/v6/faq/why-must-i-create-a-separate-config-object/
struct Config 
{
    DeviceConfiguration device;
    BrokerConfiguration broker;
};

struct BrokerConfiguration
{
    const char* key = "broker";
    char broker_user[32];
    char broker_pass[32];
    char broker_url[64];
    int broker_port;
    int broker_retry_interval_sec;
};

struct DeviceConfiguration
{
    const char* key = "device";
    char device_mac[6];
    char device_name[16];
    int ethernet_pin;
};

enum ConfigurationManagerErrors 
{
    SUCCESS,
    CONFIG_FILE_NOT_FOUND = -10,
    CONFIG_FILE_FAILED_OPEN,
    SD_INIT_FAILED = -20,
};

class ConfigurationManager
{
    public:
        //set the SD card's serial pin - defaults to 22
        ConfigurationManager(int sdCardSsPin);
        //init modules
        int init(bool resetSsPinMode);
        //load the configuration from disk
        int load(char *configFileName, Config config);
        //get a readable error msg
        char* getError(int code);
    private:
        int _sdCardSsPin;
};

#endif


