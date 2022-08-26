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
    //TODO: remove hardcoded mac address
    uint8_t device_mac[6] {0x60, 0x52, 0xD0, 0x06, 0x70, 0x27};
    char device_name[16];
    uint8_t ethernet_pin = 5;
};

struct ModbusParameter
{
    const char* key = "telemetry_registers";
    char name[32];
    char units[16];
    char topic[32];
    int address;
    int value;
    int device_id;
};

struct ModbusConfigParameter
{
    const char* key = "configuration_registers";
    char name[32];
    char units[16];
    char topic[32];
    int address;
    int value;
    int device_id;
};

struct ModbusConfiguration
{
    const char* key = "modbus";
    ModbusParameter registers[50];
    ModbusConfigParameter configuration_registers[50];
};

// Never use a JsonDocument to store the configuration!
// A JsonDocument is *not* a permanent storage; it's only a temporary storage
// used during the serialization phase. See:
// https://arduinojson.org/v6/faq/why-must-i-create-a-separate-config-object/
struct Config 
{
    struct DeviceConfiguration device;
    struct BrokerConfiguration broker;
    struct ModbusConfiguration modbus;
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
        ConfigurationManager();
        //init modules
        int Init(bool resetSsPinMode, int sdCardSsPin);
        //load the configuration from disk
        int Load(char *configFileName, struct Config *config);
        //get a readable error msg
        char* GetError(int code);
    private:
        int _sdCardSsPin;
};

extern ConfigurationManager ConfigMgr;	//Default class instance

#endif


