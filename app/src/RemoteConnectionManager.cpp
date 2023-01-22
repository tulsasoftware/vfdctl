#include "RemoteConnectionManager.h"

EthernetClient client;
MQTTClient mqttClient;
bool _initialized = false;

RemoteConnectionManager::RemoteConnectionManager(){
}

RemoteConnectionManager RemoteConnMgr;

int RemoteConnectionManager::Init(BrokerConfiguration config, DeviceConfiguration dev)
{
    if (_initialized)
    {
        return 0;
    }
    _remConfig = config;
    _devConfig = dev;
    _event = nullptr;

    int val;
    // Ethernet.init(_devConfig.ethernet_pin);   //CS pin for P1AM-ETH
    Ethernet.init(5);   //CS pin for P1AM-ETH
    uint8_t mac[6] {
        _devConfig.device_mac.b1,
        _devConfig.device_mac.b2,
        _devConfig.device_mac.b3,
        _devConfig.device_mac.b4,
        _devConfig.device_mac.b5,
        _devConfig.device_mac.b6,
        };
    //val = Ethernet.begin(_devConfig.device_mac, 10);  // Get IP from DHCP
    val = Ethernet.begin(mac);  // Get IP from DHCP
    if (val == 0)
    {
        Serial.print("Ethernet failed to obtain DHCP address. Error code = ");
        Serial.println(val);
        return static_cast<int>(RemoteConnectionErrors::ETHERNET_INITIALIZATION_FAILURE);
    }

    mqttClient.begin(_remConfig.broker_url, client);

    _initialized = true;
    return 0;
}

int RemoteConnectionManager::Connect()
{
    // if (!_initialized){
    //     return static_cast<int>(RemoteConnectionErrors::ETHERNET_INITIALIZATION_FAILURE);
    // }
    //signal that a new cycle has occurred to trigger any queued callback processing
    //this should stay as close to the top of the loop as possible
    mqttClient.loop();

    if (mqttClient.connected())
    {
        return static_cast<int>(RemoteConnectionErrors::SUCCESS);
    }

    //check ethernet link active
    if (Ethernet.linkStatus() != 1){
        Serial.println("Aborting - Ethernet link is down.");
        return static_cast<int>(RemoteConnectionErrors::ETHERNET_INITIALIZATION_FAILURE);
    }
    Serial.print("Connecting to the MQTT broker: ");
    Serial.print(_remConfig.broker_url);
    Serial.print(" , ");
    Serial.println(_remConfig.broker_user);

    // Username and Password tokens for protected broker topics
    if (!mqttClient.connect(_devConfig.device_name, _remConfig.broker_user, _remConfig.broker_pass)) 
    {
        Serial.print("MQTT connection failed! Error code = ");
        Serial.println(mqttClient.returnCode());
        return static_cast<int>(RemoteConnectionErrors::BROKER_FAILED_CONNECT);
    }
    else
    {
        Serial.println("Connected to the MQTT broker");
        //app name on network is vfdctl (vfd control)
        mqttClient.subscribe("cmd/vfdctl/#");
    }

    return static_cast<int>(RemoteConnectionErrors::SUCCESS);
}

void RemoteConnectionManager::RegisterOnMessageReceivedCallback(InputEvent event)
{
    //prevent invalid callback events
    if (event == nullptr){
        return;
    }

    //prevent double subscribing
    if (_event != event){
        _event = event;
        mqttClient.onMessage(event);
    }
}

int RemoteConnectionManager::Publish(String message, String topic)
{
    if (!mqttClient.connected()){
        return -1;
    }
    int pubValue = mqttClient.publish(topic, message);

    if ( pubValue < 0)
    {
        Serial.println("Error publishing to MQTT topic. Code: ");
        Serial.println(pubValue);
        return -2;
    }

    return 0;
}

char* RemoteConnectionManager::GetError(int code)
{
    char* val;
    switch (static_cast<RemoteConnectionErrors>(code))
    {
        case RemoteConnectionErrors::SUCCESS:
            val = "success";
            break;
        case RemoteConnectionErrors::BROKER_FAILED_CONNECT:
            val = "failed to connect to mqtt broker";
            break;
        case RemoteConnectionErrors::UNREADABLE_MESSAGE:
            val = "unable to parse received message from topic";
            break;
        case RemoteConnectionErrors::NO_MESSAGES_AVAILABLE:
            val = "client did not find any available messages";
            break;
        default:
            val = "unrecognized error";
            break;
    }

    return val;
}