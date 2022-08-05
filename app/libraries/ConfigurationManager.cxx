#include <Arduino.h>
#include <ConfigurationManager.h>
#include <ArduinoJson.h>
#include <SD.h>

ConfigurationManager::ConfigurationManager(int sdCardSsPin)
{
    _sdCardSsPin = sdCardSsPin;
}

int ConfigurationManager::init(bool resetSsPinMode){

    // Initialize SD library
    if (resetSsPinMode){
        pinMode(_sdCardSsPin, OUTPUT);
    }
    
    if (!SD.begin(_sdCardSsPin)) {
        Serial.println(F("Failed to initialize SD library"));
        return SD_INIT_FAILED;
    }

    return SUCCESS;
}

int ConfigurationManager::load(char* configFileName, Config config)
{
    Serial.println("Opening config file...");
    // Open file for reading
    File file = SD.open(configFileName);
    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/v6/assistant to compute the capacity.
    StaticJsonDocument<384> doc;

    if (file){
        // Deserialize the JSON document
        DeserializationError error = deserializeJson(doc, file);
        if (error){
            // Close the file (Curiously, File's destructor doesn't close the file)
            Serial.println(F("Failed to read file"));
            Serial.println(error.c_str());
            Serial.println("Create conf.txt and place at root of SD card to configure settings");

            file.close();
            Serial.println("Gracefully closed config file");
            return CONFIG_FILE_NOT_FOUND;
        }

        // Copy values from the JsonDocument to the Config

        //mqtt broker connection
        strlcpy(config.broker_user,                  // <- destination
            doc["broker_user"] | "",              // <- source
            sizeof(config.broker_user));         // <- destination's capacity
        strlcpy(config.broker_pass,
            doc["broker_pass"] | "",
            sizeof(config.broker_pass));
        strlcpy(config.broker_url,
                doc["broker_url"] | "none",
                sizeof(config.broker_url));
        config.broker_port = doc["broker_port"] | 1883;

        //arduino device settings
        strlcpy(config.device_name,
                doc["device_name"] | "arduino",
                sizeof(config.device_name));
        strlcpy(config.device_mac,
                doc["device_eth_mac"] | "",
                sizeof(config.device_mac));

        //app settings
    }

    file.close();
    Serial.println("Gracefully closed config file");
    return SUCCESS;
}