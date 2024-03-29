#ifndef ConfigurationManager_h
#define ConfigurationManager_h
//enable json comments in deserializer
#define ARDUINOJSON_ENABLE_COMMENTS 1

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SD.h>

/// @brief Comparison modes for two integers
enum eLimitComparison
{
    none = 0,
    between,
    between_or_equal,
    greater_than,
    greater_than_or_equal,
    less_than,
    less_than_or_equal
};

/// @brief MQTT broker and connection information
struct BrokerConfiguration
{
    bool formed = false;
    const char* key = "broker";
    char broker_user[32];
    char broker_pass[32];
    char broker_url[64];
    int broker_port;
    int broker_retry_interval_sec;
};

/// @brief Serial port connection information
struct SerialPortConfiguration
{
    bool formed = false;
    const char* key = "serial_port";
    int baud_rate = 9600;
    int data_bits = 8;
    int parity_bits = 0;
    int stop_bits = 1;
    bool flow_control = false;
};

struct MacAddress
{
    bool formed = false;
    const char* key = "controller_mac_address";
    uint8_t b1;
    uint8_t b2;
    uint8_t b3;
    uint8_t b4;
    uint8_t b5;
    uint8_t b6;
};

/// @brief Controller information
struct DeviceConfiguration
{
    bool formed = false;
    const char* key = "device";
    MacAddress device_mac;
    char device_name[16];
    uint8_t ethernet_pin = 5;
};

/// @brief Telemetry (publish-only) register configuration
struct ModbusParameter
{
    bool formed = false;
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
    bool formed = false;
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
    bool formed = false;
    const char* key = "modbus";
    int offset;
    int telemetry_interval_sec;
    SerialPortConfiguration serial_port;
    ModbusParameter registers[50];
    ModbusConfigParameter configuration_registers[50];
};

// Never use a JsonDocument to store the configuration!
// A JsonDocument is *not* a permanent storage; it's only a temporary storage
// used during the serialization phase. See:
// https://arduinojson.org/v6/faq/why-must-i-create-a-separate-config-object/
struct Config 
{
    bool formed = false;
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
        int Load(char *configFileName, struct Config* config);
        //get a readable error msg
        char* GetError(int code);
        //convert enum to string equivalent
        String toString(eLimitComparison mode);
        //eLimitComparison from(JsonVariantConst mode);
        ModbusConfigParameter GetParameter(String topic, struct Config* config);
    private:
        int _sdCardSsPin;
};

extern ConfigurationManager ConfigMgr;	//Default class instance
#endif


