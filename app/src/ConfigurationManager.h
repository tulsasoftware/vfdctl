#ifndef ConfigurationManager_h
#define ConfigurationManager_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SD.h>


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

// Never use a JsonDocument to store the configuration!
// A JsonDocument is *not* a permanent storage; it's only a temporary storage
// used during the serialization phase. See:
// https://arduinojson.org/v6/faq/why-must-i-create-a-separate-config-object/
struct Config 
{
    struct DeviceConfiguration device;
    struct BrokerConfiguration broker;
};

enum class ConfigurationManagerErrors 
{
    SUCCESS,
    CONFIG_FILE_NOT_FOUND = -100,
    CONFIG_FILE_FAILED_OPEN,
    SD_INIT_FAILED = -200,
};

class ConfigurationManager
{
    public:
        //init modules
        int Init(bool resetSsPinMode, int sdCardSsPin);
        //load the configuration from disk
        int Load(char *configFileName, Config config);
        //get a readable error msg
        char* GetError(int code);
    private:
        int _sdCardSsPin;
};

#endif


