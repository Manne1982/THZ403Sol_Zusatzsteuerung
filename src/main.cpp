#include <Arduino.h>
//Für Temperatur Sensoren
#include <OneWire.h>
//für Uhrzeitabruf notwendig
#include "time.h"
//Allgemeine Bibliotheken
#include <string.h>
#include <ArduinoOTA.h>
//Projektspezifisch
#include "HTML_Var.h"
#include "Class_DS18B20.h"
#include "HZS_Functions_Classes.h"
#include "WiFiFunctions.h"
#include "WebFunctions.h"
#include "GlobalVariabels.h"
#include "MQTT_Functions.h"
#include "ProjectFunctions.h"
//Port extension
#include <Adafruit_MCP23X17.h>

//#define BGTDEBUG 1

void setup(void) {
  wifiClient = new WiFiClient;
  #ifdef BGTDEBUG
    Serial.begin(9600);
  #endif
  uint8 ResetCount = 0;
  ResetCount = ResetVarLesen();
  if(ResetCount > 5)  //plausibility check, otherwise reset of count variable
    ResetCount = 0;
  //ResetCount++;     //If controller restart 5 times in first 5 seconds do not load saved settings (basic setting will setted)
  ResetVarSpeichern(ResetCount);
  delay(5000);
  if (ResetCount < 5) //If controller restart 5 times in first 5 seconds do not load saved settings (basic setting will setted)
  {    
    if(!EinstLaden()) //If failure then standard config will be saved
      EinstSpeichern();
    else
    {
      #ifdef BGTDEBUG
        Serial.println("Einstellung wurde erfolgreich geladen!");
      #endif

    }
  }
  else
  {
    ResetCount = 0;
  }
  ResetVarSpeichern(0);
  TakeoverTSConfig(&SensorPort1, TempSensors, MaxSensors);
  TakeoverTSConfig(&SensorPort2, TempSensors, MaxSensors);
  EinstSpeichern();
  #ifdef BGTDEBUG
    Serial.println("Sensordaten übernommen!");
  #endif
  NetworkInit(); //Initialization of WiFi, Ethernet and MQTT
  #ifdef BGTDEBUG
    Serial.println("Netzwerk initiiert");
  #endif
  //Zeitserver Einstellungen
  if (strlen(varConfig.NW_NTPServer))
    timeClient = new NTPClient(*ntpUDP, (const char *)varConfig.NW_NTPServer);
  else
    timeClient = new NTPClient(*ntpUDP, "fritz.box");
  #ifdef BGTDEBUG
    Serial.println("NTP-Client erstellt");
  #endif
  delay(1000);
  timeClient->begin();
  timeClient->setTimeOffset(varConfig.NW_NTPOffset * 3600);
  #ifdef BGTDEBUG
    Serial.println("NTP-Client initiiert");
  #endif
  //OTA
  ArduinoOTA.setHostname("HeizungOTA");
  ArduinoOTA.setPassword("Heizung!123");
  ArduinoOTA.begin();
  #ifdef BGTDEBUG
    Serial.println("OTA gestartet");
  #endif
  for(int i = 0; i<3; i++)
  {
    //OTA
    ArduinoOTA.handle();
    delay(1000);
  }
  //Webserver
  server.begin();
  server.onNotFound(notFound);
  server.on("/", HTTP_GET, WebserverRoot);
  server.on("/Sensors", HTTP_GET, WebserverSensors);
  server.on("/Output", HTTP_GET, WebserverOutput);
  server.on("/POST", HTTP_POST, WebserverPOST);
  #ifdef BGTDEBUG
    Serial.println("Webserver gestartet");
  #endif
  //MQTT
  MQTTinit();
  #ifdef BGTDEBUG
    Serial.println("MQTT initiiert");
  #endif
  //Output config
  //Port Extension
  Inputs.StatesHW = MCPinit(mcp, MCPState);
  #ifdef BGTDEBUG
    Serial.println("MCP initiiert");
  #endif
  //Output_Values.Outputstates = InitOutputStates(mcp, OutputsBasicSettings, MCPState, &Output_Values.OutputstatesAutoSSRelais); 
  InitOutputStates(mcp, OutputsBasicSettings, MCPState, &Output_Values); 
  #ifdef BGTDEBUG
    Serial.println("Ausgänge initiiert");
  #endif
  MQTT_sendMessage((MQTTSendRoot[3]+"realOutput").c_str(), Output_Values.realValue);

  #ifdef BGTDEBUG
    Serial.println("MQTT send real Output");
  #endif

  MQTT_SendInputStates();
  #ifdef BGTDEBUG
    Serial.println("MQTT_SendInputStates");
  #endif
  ValveHeating = new ThreeWayValve(2, 3);
  //AirSens init
  AirSensValues = new AirQualitySensor(AirSensOnOff, AirSensAnalogPort);
}
void loop(void) {
  //OTA
  ArduinoOTA.handle();
  //MQTT wichtige Funktion
  if(varConfig.NW_Flags & NW_MQTTActive)
  {
    if(((varConfig.NW_Flags & NW_EthernetActive) && Ethernet.linkStatus()== LinkON)||(!(varConfig.NW_Flags & NW_EthernetActive) && WiFi.isConnected()))  //MQTTclient->loop() wichtig damit die Daten im Hintergrund empfangen werden
    {
      if(!MQTTclient->loop())
      {
        MQTTinit();
      }
      #ifdef BGTDEBUG
        Serial.println("MQTTclient loop");
      #endif
    }
    else
    {
      #ifdef BGTDEBUG
        Serial.println("No Link activ");
      #endif
    } 
  } 
  if(millis()>Break_60s)
  {
    Break_60s = millis() + 60000;
    MQTT_sendMessage((MQTTSendRoot[3]+"WLAN-Status").c_str(), WiFi.status());
    MQTT_sendMessage((MQTTSendRoot[3]+"WLAN-Verbindung").c_str(), WiFi.RSSI());
  }

  if(millis()>Break_10s)
  {
    Break_10s = millis() + 10000;
    SensorPort1.StartConversion();
    SensorPort2.StartConversion();
    MQTT_sendMessage((MQTTSendRoot[5]+"SensorState").c_str(), AirSensValues->getSensorState());
    MQTT_sendMessage((MQTTSendRoot[5]+"ADConverter").c_str(), AirSensValues->getHWValue());
    MQTT_sendMessage((MQTTSendRoot[5]+"LPG").c_str(), AirSensValues->readLPG());
    MQTT_sendMessage((MQTTSendRoot[5]+"CO").c_str(), AirSensValues->readCO());
    MQTT_sendMessage((MQTTSendRoot[5]+"Smoke").c_str(), AirSensValues->readSMOKE());
    MQTT_sendMessage((MQTTSendRoot[3]+"VentilHK2").c_str(), ValveHeating->getValvePosition());
    #ifdef BGTDEBUG
      Serial.print(timeClient->getFormattedTime());
      Serial.print(" ");
      Serial.println(Ethernet.localIP());
    #endif
  }
  if(millis()>Break_s)
  {
    Break_s = millis() + 1000;
  }
  if (Break_h < millis())
  {
    Break_h = millis() + 3600000;
    WIFIConnectionCheck(true);
    timeClient->update();
  }
  uint8 oldRealValue = Output_Values.realValue;
  if(oldRealValue != getRealOutput(&Output_Values, &Inputs))
  {
    ValveHeating->updateState(&Output_Values);
    MQTT_sendMessage((MQTTSendRoot[3]+"realOutput").c_str(), Output_Values.realValue);
  }
  //MQTT
  if(varConfig.NW_Flags & NW_MQTTActive)
    MQTTclient->loop();
  //Temperature sensor  
  if(SensorPort1.Loop())
  {
    String SensorPathName = "";
    for(int i = 0; i < SensorPort1.GetSensorCount(); i++)
    {
      if(SensorPort1.GetSensorIndex(i)->NewValueAvailable())
      {
        SensorPathName = MQTTSendRoot[4] + SensorPort1.GetSensorIndex(i)->getName();
        MQTT_sendMessage(SensorPathName.c_str(), SensorPort1.GetSensorIndex(i)->getTempC());
//        MQTT_sendMessage(SensorPort1.GetSensorIndex(i)->getName().c_str(), SensorPort1.GetSensorIndex(i)->getTempC());
      }
    }
  }
  if(SensorPort2.Loop())
  {
    String SensorPathName = "";
    for(int i = 0; i < SensorPort2.GetSensorCount(); i++)
    {
      if(SensorPort2.GetSensorIndex(i)->NewValueAvailable())
      {
        SensorPathName = MQTTSendRoot[4] + SensorPort2.GetSensorIndex(i)->getName();
        MQTT_sendMessage(SensorPathName.c_str(), SensorPort2.GetSensorIndex(i)->getTempC());
      }
    }
  }
  if(Output_Values.PWM_Manu_activ)
  {
    updateOutputPWM(mcp, &Output_Values, MCPState);
  }
  if(readDigitalInputs_SetOutputIfAutoSSRMode(digitalRead(INTPortA), &Inputs, mcp, &Output_Values, MCPState))
  {
    MQTT_SendInputStates();
  }
  //Restart for WWW-Requests
  if(ESP_Restart)
  {
    delay(1000);
    ESP.restart();
  }

}
//---------------------------------------------------------------------
