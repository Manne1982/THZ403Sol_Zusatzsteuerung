#ifndef GlobalVariabels
#define GlobalVariabels

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <NTPClient.h>

//für Webserver und Ethernet notwendig
#include <EthernetENC.h>
//für Webserver notwendig
#include <ESPAsyncWebServer.h>
//MQTT
#include <PubSubClient.h>

#define BGTDEBUG 1
//Projektvariablen
NWConfig varConfig;
char const EthernetMAC[] = "A0:A1:A2:A3:A4:A5";         //For Ethernet connection (MQTT)
uint8 const mac[6] = {0xA0,0xA1,0xA2,0xA3,0xA4,0xA5}; //For Ethernet connection
bool ESP_Restart = false;                         //Variable for a restart with delay in WWW Request
unsigned long Break_h = 0;
unsigned long Break_10s = 0;
unsigned long Break_60s = 0;
unsigned long Break_s = 0;
//Erstellen Serverelement
AsyncWebServer server(8080);
//Uhrzeit Variablen
UDP * ntpUDP = 0;
NTPClient * timeClient = 0;
//MQTT Variablen
EthernetClient * e_client = 0;
WiFiClient * wifiClient;
PubSubClient * MQTTclient = 0;
unsigned long lastMQTTInit = 0;
String MQTTSubscribeRoot[3] = {"/setOutput/", "/setGeneral/", "/setOutputPWM/"}; //{"/setOutput/", "/General/", "/setOutputPWM/"}
String MQTTSendRoot[7] = {"Input_PWM_Value/", "Input_direct/", "Output_PWM_Value/", "General_Information/", "TempSensors/", "AirQuality/"}; //{"Input_PWM_Value/", "Input_direct/", "Output_PWM_Value/", "General_Information/", "TempSensors/", "AirQuality/"}
//Temperature sensors
const uint8 MaxSensors = 15; 
TSensorArray SensorPort1(9);
TSensorArray SensorPort2(10);
TempSensor TempSensors[MaxSensors]; //Variable array for save Information about DS18B20 sensors.
AirQualitySensor * AirSensValues;
//Port extension
Adafruit_MCP23X17 mcp[2];
int MCPState[2] = {0, 0}; //0 = not connected, 1 = connected, 2 = error
//Output Input configuration
digital_Output OutputsBasicSettings[8];  //Variable array for save Output Information.
digital_Input Inputs;  //Variable to save current states and auxiliary variables of digital inputs
digital_Output_current_Values Output_Values;
ThreeWayValve * ValveHeating;

#endif