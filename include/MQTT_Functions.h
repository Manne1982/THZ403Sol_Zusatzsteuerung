#ifndef MQTT_Functions
#define MQTT_Functions

#include <Arduino.h>
#include <PubSubClient.h>


//MQTT functions
bool MQTTinit();  //Wenn verbunden Rückgabewert true
void MQTT_callback(char* topic, byte* payload, unsigned int length);
void MQTT_SendInputStates();
int FindOutputName(const char* Topic);
bool MQTT_sendMessage(const char * ValueName, const uint8* MSG, uint8 len);
bool MQTT_sendMessage(const char * ValueName, int MSG);
bool MQTT_sendMessage(const char * ValueName, uint8 MSG);
bool MQTT_sendMessage(const char * ValueName, uint32 MSG);
bool MQTT_sendMessage(const char * ValueName, float MSG);
void MQTT_SendFailureText(String Text, bool withDate = false);


/*
    MQTT
    -4 : MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time
    -3 : MQTT_CONNECTION_LOST - the network connection was broken
    -2 : MQTT_CONNECT_FAILED - the network connection failed
    -1 : MQTT_DISCONNECTED - the client is disconnected cleanly
    0 : MQTT_CONNECTED - the client is connected
    1 : MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT
    2 : MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier
    3 : MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection
    4 : MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected
    5 : MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect

    WiFi
    WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
    WL_IDLE_STATUS      = 0,
    WL_NO_SSID_AVAIL    = 1,
    WL_SCAN_COMPLETED   = 2,
    WL_CONNECTED        = 3,
    WL_CONNECT_FAILED   = 4,
    WL_CONNECTION_LOST  = 5,
    WL_DISCONNECTED     = 6
  */

#include "MQTT_Functions.cpp"
#endif