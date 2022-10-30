#ifndef Klassen_HZS
#define Klassen_HZS
#include <Arduino.h>

struct NWConfig {
  //Einstellungen NW-Einstellungen WLAN
  uint16 NW_Flags = 1; //See Enum NW_...
  char WLAN_SSID[40] = "HeatingAC";
  char WLAN_Password[70] = "";
  //Einstellungen NW-Einstellungen MQTT
  char MQTT_Server[50] = "192.168.178.5";
  uint16 MQTT_Port = 1883;
  char MQTT_Username[20] = "MQTT_User";
  char MQTT_Password[70] = "12345";
  char MQTT_fprint[70] = "";
  char MQTT_rootpath[100] = "/Heating/Add_Control";
  //Einstellungen NW-Einstellungen Netzwerk
  char NW_NetzName[20] = "Add_Control";
  char NW_IPAddress[17] = "192.168.178.10";
  char NW_IPAddressEthernet[17] = "192.168.178.11";
  char NW_SubMask[17] = "255.255.255.0";
  char NW_Gateway[17] = "192.168.178.1";
  char NW_DNS[17] = "192.168.178.1";
  char NW_NTPServer[55] = "fritz.box";
  char NW_NTPOffset = 0;
};

struct digital_Output{
  char Name[15]="unnamed";
  byte StartValue = 2; //0 = off; 1 = on; 2 = Auto
  byte MCP = 0;     //0 = MCP one Address 38; 1 = MCP two Address 39
  byte MCP_Pin_Auto = 0; //associated Pin in MCP-Modul. (1 - 16) for switch between manuel or automatic
  byte MCP_Pin_OnOff = 0; //associated Pin in MCP-Modul. (1 - 16) for switch in manuel mode on or off
  byte MQTTState = 0; //0 = MQTT control off; 1 = MQTT control on
};

struct TempSensor{
  uint64 Address = 0; //8 Byte Address saved as unsigned int for easier comparison
  char Name[15] = "unnamed"; //Name of Sensor
  byte SensorState = 0; //0 = Sensor not found; 1 = Sensor found; 2 = MQTT enabled for found sensor
};


#include "Klassen_HZS.cpp"

#endif
