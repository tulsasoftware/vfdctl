#ifndef ConfigurationManager_h
#define ConfigurationManager_h
//enable json comments in deserializer
#define ARDUINOJSON_ENABLE_COMMENTS 1

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SD.h>

/// @brief Comparison modes for two integers
typedef enum eLimitComparison
{
    NONE = 0,
    BETWEEN = 10,
    BETWEEN_OR_EQUAL = 11,
    GREATER_THAN = 20,
    GREATER_THAN_OR_EQUAL = 21,
    LESS_THAN = 30,
    LESS_THAN_OR_EQUAL = 31
};

/// @brief MQTT broker and connection information
struct BrokerConfiguration
{
    const char* key = "broker";
    char broker_user[32];
    char broker_pass[32];
    char broker_url[64];
    int broker_port;
    int broker_retry_interval_sec;
};

/// @brief Controller information
struct DeviceConfiguration
{
    const char* key = "device";
    //TODO: remove hardcoded mac address
    uint8_t device_mac[6] {0x60, 0x52, 0xD0, 0x06, 0x70, 0x27};
    char device_name[16];
    uint8_t ethernet_pin = 5;
};

/// @brief Telemetry (publish-only) register configuration
struct ModbusParameter
{
    const char* key = "telemetry_registers";
    char name[32];
    char units[16];
    char topic[64];
    int address;
    int value;
    int device_id;
};

/// @brief Configuration (command-response) register setup
struct ModbusConfigParameter
{
    const char* key = "configuration_registers";
    char name[32];
    char units[16];
    char topic[64];
    int address;
    int value;
    int device_id;
    int upper_limit;
    int lower_limit;
    eLimitComparison limit_comparison;
};

/// @brief Parent of all types of Modbus registers
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
        ModbusConfigParameter GetParameter(String topic, struct Config *config);
    private:
        int _sdCardSsPin;
};

extern ConfigurationManager ConfigMgr;	//Default class instance

#endif


