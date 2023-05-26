#ifndef Klassen_HZS
#define Klassen_HZS
#include <Arduino.h>
#include "Class_DS18B20.h"
#include <Adafruit_MCP23X17.h>
#include "MQ2lib.h"
#define MCPPort0 39
#define MCPPort1 38
#define MCPOutput 0
#define MCPInput 1
#define INTPortA D3
#define INTPortB D4
#define AirSensOnOff D8
#define AirSensAnalogPort A0

enum {NW_WiFi_AP = 0x01, NW_StaticIP = 0x02, NW_EthernetActive = 0x04, NW_MQTTActive = 0x08, NW_MQTTSecure = 0x10}; //Enum for NWConfig


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
  byte StartValue = 2; //0 = off; 1 = on; 2 = Auto; 3 = Auto but Output over Solid State Relais; 4 = Auto but Output over Solid State Relais Manu Relais always on; 5 = PWM
  byte MQTTState = 0; //0 = MQTT control off; 1 = MQTT control on
};

struct digital_Output_current_Values{
  uint16 Outputstates = 0xFFFF; //Variable for the current states of the Output MCP[0]
  uint16 OutputstatesAutoSSRelais = 0x0000;  //Variable for the Auto over SSR-Settings
  uint16 OutputstatesAutoSSRelais_alwaysManu = 0x0000;  //Variable for the Auto over SSR-Settings and Manu Relais always on
  uint8 PWM_Manu_activ = 0x00; 
  uint8 PWM_Value[8] = {25, 25, 25, 25, 25, 25, 25, 25};
  uint16 PWM_CycleTime_ms = 10000;
  uint8 PWM_Min = 25;
  uint8 PWM_Max = 230;
  uint8 PWM_CurrentState = 0;
  uint8 realValue = 0;
};

struct TempSensor{
  uint64 Address = 0; //8 Byte Address saved as unsigned int for easier comparison
  char Name[15] = ""; //Name of Sensor
  byte SensorState = 0; //0 = Sensor not found; 1 = Sensor found; 2 = MQTT enabled for found sensor
  float Offset = 0;
};

class AirQualitySensor{
public:
  AirQualitySensor(uint8 _PortOnOff, uint8 _PortAnalogInput);
  ~AirQualitySensor();
  byte getSensorState();
  void setSensorState(byte newState);
  byte toggleSensorState();
  uint32 getOnTime_s();
  float readLPG();
  float readCO();
  float readSMOKE();
  uint16 getHWValue();  //Get the AD value 
private:
  byte SensorState; 
  unsigned long OnTime_ms;
  uint32 HeatingTime_ms; 
  byte AirSensInitiated;
  byte PortOnOff;
  byte PortAnalogInput;
  MQ2 * AirSens;
};

class ThreeWayValve{
public:
  ThreeWayValve(uint8 _ChannelOpen, uint8 _ChannelClose);
  ~ThreeWayValve();
  void setChannelOpen(uint8 _Channel);
  void setChannelClose(uint8 _Channel);
  void setCycleTimeOpen(uint16 _Seconds);
  void setCycleTimeClose(uint16 _Seconds);
  uint8 getChannelOpen();
  uint8 getChannelClose();
  uint16 getCycleTimeOpen();
  uint16 getCycleTimeClose();
  void setValvePosition(uint16 _Position);
  uint16 getValvePosition();
  void updateState(digital_Output_current_Values * _Outputs);
private:
  uint8 ChannelOpen;              //Channel for open the valve
  uint8 ChannelClose;             //Channel for close the valve
  uint16 CycleTimeOpen_s;         //Time for 
  uint16 CycleTimeClose_s;        //Time for 
  uint16 ValvePosition;           //10.000 = Open --> 0 = Closed
  unsigned long OnTimeOpen_ms;    //millis() at put on the Open-Channel
  unsigned long OnTimeClose_ms;   //millis() at put on the Open-Channel
};


struct digital_Input{
  uint8 StatesHW = 0;  //Variable for the current states of the Inputs MCP[1] Port A
  unsigned long TimeStartpoints[8][2] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  //Starttime High and StartTime Low for calculate the percentage On-Time (Port A)
  uint8 OnTimeRatio[8] = {0, 0, 0, 0, 0, 0, 0, 0};   //Ratio Value of On-Time (max 10 s time slot otherwise 255 or 0)
  uint8 ReadStep[8] = {0, 0, 0, 0, 0, 0, 0, 0}; //Value for save the step of Input reading 0= Reading not started, 1= On-time Start point fixed, 2 = Off-time Start point fixed, 3= Value safed 
  unsigned long lastReading = 0;
};

//Input Output functions
TempSensor * FindTempSensor(TempSensor * TSArray, uint8 ArrayLen, uint64 Address); //Get pointer of varibale with the same address, if nothing found return value 0
TempSensor * GetFirstEmptyTS(TempSensor * TSArray, uint8 ArrayLen); //Get pointer of first unused variable, if no free variable found, return value 0
void DelTSensor(uint64 Address, TempSensor * TSArray, uint8 ArrayLen);//Delete Sensor variables
void DelTSensor(TempSensor * Sensor); //Delete Sensor variables
void TakeoverTSConfig(TSensorArray * TSensArray, TempSensor * TSArray, uint8 ArrayLen); //Takeover saved config into TSensorArray variable and missing sensors from TSensorArray into saved config
uint8 FindMissingSensors(TSensorArray * TSensArray1, TSensorArray * TSensArray2, TempSensor * *TSArrayMissing, TempSensor * TSArray, uint8 ArrayLen); //Function to find missing Sesnors into the config array, return value is the count of missing Sensors
int MCPSetup(Adafruit_MCP23X17 * MCP, int MCPAddress);
uint8 MCPinit(Adafruit_MCP23X17 * MCP, int * MCPStates);
void InitOutputStates(Adafruit_MCP23X17 * MCP, digital_Output * Config, int * MCPStates, digital_Output_current_Values * Output_Values); //MCPStates: 0= Not initiated, 1= connected, 2 = error
//uint16 InitOutputStates(Adafruit_MCP23X17 * MCP, digital_Output * Config, int * MCPStates, uint16 * AutoOverSSRelais); //MCPStates: 0= Not initiated, 1= connected, 2 = error
void SetOutput(int OutputIndex, int Value, digital_Output_current_Values * _Output, Adafruit_MCP23X17 * _MCP, int * MCPStates);
bool readDigitalInputs_SetOutputIfAutoSSRMode(int Interrupt, digital_Input * _Inputs, Adafruit_MCP23X17 * _MCP, digital_Output_current_Values* _Output, int * MCPStates);
void updateOutputPWM(Adafruit_MCP23X17 * _MCP, digital_Output_current_Values* _Output, int * MCPStates);
void DO_new_Init(Adafruit_MCP23X17 * _MCP, digital_Output_current_Values * _Output, int * _MCPStates);
void setRelaisManuAuto(Adafruit_MCP23X17 * _MCP, uint8 OutputIndex, uint8 Manu, int * MCPStates);
void setSSR(Adafruit_MCP23X17 * _MCP, uint8 OutputIndex, uint8 On_Off, int * MCPStates);
uint8 getRealOutput(digital_Output_current_Values * _Output_Values, digital_Input * _Inputs);


//General functions
uint64 StrToLongInt(String Input);
String IntToStr(int _var);
String IntToStr(float _var);
String IntToStrHex(int _var);
String IntToStr(uint32 _var);
String IntToStr(uint8 _var);



#include "HZS_Functions_Classes.cpp"

#endif
