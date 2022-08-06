#include "ConfigurationManager.h"

int _sdCardSsPin;
ConfigurationManager::ConfigurationManager(){
}

ConfigurationManager ConfigMgr;		//Create Class instance

int ConfigurationManager::Init(bool resetSsPinMode, int sdCardSsPin)
{
    _sdCardSsPin = sdCardSsPin;

    // Initialize SD library
    if (resetSsPinMode){
        Serial.println(F("Resetting SS pinmode"));
        pinMode(_sdCardSsPin, OUTPUT);
    }
    
    Serial.println(F("Beginning to initialize SD library"));
    if (!SD.begin(_sdCardSsPin)) {
        Serial.println(F("Failed to initialize SD library"));
        return static_cast<int>(ConfigurationManagerErrors::SD_INIT_FAILED);
    }

    return static_cast<int>(ConfigurationManagerErrors::SUCCESS);
}

int ConfigurationManager::Load(char* configFileName, Config config)
{
    Serial.print("Opening config file ");
    Serial.println(configFileName);

    // Open file for reading
    File file = SD.open(configFileName);
    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/v6/assistant to compute the capacity.
    StaticJsonDocument<384> doc;

    if (file)
    {
        // Deserialize the JSON document
        DeserializationError error = deserializeJson(doc, file);
        if (error)
        {
            // Close the file (Curiously, File's destructor doesn't close the file)
            Serial.println(F("Failed to read file"));
            Serial.println(error.c_str());
            Serial.println("Create conf.txt and place at root of SD card to configure settings");

            file.close();
            Serial.println("Gracefully closed config file");
            return static_cast<int>(ConfigurationManagerErrors::CONFIG_FILE_NOT_FOUND);
        }

        // Copy values from the JsonDocument to the Config

        //mqtt broker connection
        strlcpy(config.broker.broker_user,                  // <- destination
            doc[config.broker.key]["broker_user"] | "",              // <- source
            sizeof(config.broker.broker_user));         // <- destination's capacity
        strlcpy(config.broker.broker_pass,
            doc[config.broker.key]["broker_pass"] | "",
            sizeof(config.broker.broker_pass));
        strlcpy(config.broker.broker_url,
            doc[config.broker.key]["broker_url"] | "none",
            sizeof(config.broker.broker_url));
        config.broker.broker_port = doc[config.broker.key]["broker_port"] | 1883;
        config.broker.broker_retry_interval_sec = doc[config.broker.key]["broker_retry_interval_sec"] | 5;

        //arduino device settings
        strlcpy(config.device.device_name,
                doc[config.device.key]["device_name"] | "arduino",
                sizeof(config.device.device_name));
        strlcpy(config.device.device_mac,
                doc[config.device.key]["device_mac"] | "",
                sizeof(config.device.device_mac));

        //app settings
    }
    else
    {
        file.close();
        Serial.println("Gracefully closed config file");
        return static_cast<int>(ConfigurationManagerErrors::CONFIG_FILE_FAILED_OPEN);
    }

    file.close();
    Serial.println("Gracefully closed config file");
    return static_cast<int>(ConfigurationManagerErrors::SUCCESS);
}

char* ConfigurationManager::GetError(int code)
{
    char* val;
    switch (static_cast<ConfigurationManagerErrors>(code))
    {
        case ConfigurationManagerErrors::SUCCESS:
            val = "success";
            break;
        case ConfigurationManagerErrors::SD_INIT_FAILED:
            val = "unable to initialize SD card reader, check your ss pin is correct and if you require a line reset";
            break;
        case ConfigurationManagerErrors::CONFIG_FILE_NOT_FOUND:
            val = "unable to load configuration file, check that the file exists and is formatted correctly";
            break;
        case ConfigurationManagerErrors::CONFIG_FILE_FAILED_OPEN:
            val = "unable to open configuration file";
        default:
            val = "unrecognized error";
            break;
    }

    return val;
}