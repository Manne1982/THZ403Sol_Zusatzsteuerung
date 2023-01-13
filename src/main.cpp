#include <Arduino.h>
//Für Temperatur Sensoren
#include <OneWire.h>
//Für Wifi-Verbindung notwendig
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
//für Webserver und Ethernet notwendig
#include <EthernetENC.h>
//für Webserver notwendig
#include <ESPAsyncWebServer.h>
//MQTT
#include <PubSubClient.h>
//für Uhrzeitabruf notwendig
#include <NTPClient.h>
#include "time.h"
//Allgemeine Bibliotheken
#include <string.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>
//Projektspezifisch
#include "HTML_Var.h"
#include "Class_DS18B20.h"
#include "HZS_Functions_Classes.h"
//Port extension
#include <Adafruit_MCP23X17.h>

#define BGTDEBUG 0


//Funktionsdefinitionen
//Functions for Ethernet or WiFi connections
void NetworkInit(bool OnlyWiFi = false);
void WiFi_Start_STA(char *ssid_sta, char *password_sta);
void WiFi_Start_AP();
bool WIFIConnectionCheck(bool with_reconnect);
//Webserver functions
void notFound(AsyncWebServerRequest *request);
void WebserverRoot(AsyncWebServerRequest *request); 
void WebserverPOST(AsyncWebServerRequest *request);
void WebserverSensors(AsyncWebServerRequest *request);
void WebserverOutput(AsyncWebServerRequest *request);

//Functions for saving settings
void EinstSpeichern();
bool EinstLaden();
char ResetVarLesen();
void ResetVarSpeichern(char Count);
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

//Projektvariablen
NWConfig varConfig;
uint8 MQTT_NTP_over_Ethernet_activ = 0;
char const EthernetMAC[] = "A0:A1:A2:A3:A4:A5";         //For Ethernet connection (MQTT)
uint8 const mac[6] = {0xA0,0xA1,0xA2,0xA3,0xA4,0xA5}; //For Ethernet connection
bool ESP_Restart = false;                         //Variable for a restart with delay in WWW Request
unsigned long Break_h = 0;
unsigned long Break_10s = 0;
unsigned long Break_s = 0;
//Erstellen Serverelement
AsyncWebServer server(80);
//Uhrzeit Variablen
UDP * ntpUDP = 0;
NTPClient * timeClient = 0;
//MQTT Variablen
EthernetClient * e_client = 0;
WiFiClient * wifiClient;
PubSubClient * MQTTclient = 0;
String SubscribeRoot[2] = {"/setOutput/", "/General/"};
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
  }
  else
  {
    ResetCount = 0;
  }
  ResetVarSpeichern(0);
  TakeoverTSConfig(&SensorPort1, TempSensors, MaxSensors);
  TakeoverTSConfig(&SensorPort2, TempSensors, MaxSensors);
  EinstSpeichern();
  NetworkInit(); //Initialization of WiFi, Ethernet and MQTT
  //Zeitserver Einstellungen
  if (strlen(varConfig.NW_NTPServer))
    timeClient = new NTPClient(*ntpUDP, (const char *)varConfig.NW_NTPServer);
  else
    timeClient = new NTPClient(*ntpUDP, "fritz.box");
  delay(1000);
  timeClient->begin();
  timeClient->setTimeOffset(varConfig.NW_NTPOffset * 3600);
  //OTA
  ArduinoOTA.setHostname("HeizungOTA");
  ArduinoOTA.setPassword("Heizung!123");
  ArduinoOTA.begin();
  for(int i = 0; i<3; i++)
  {
    //OTA
    ArduinoOTA.handle();
    delay(1000);
  }
  //MQTT
  MQTTinit();
  //Webserver
  server.begin();
  server.onNotFound(notFound);
  server.on("/", HTTP_GET, WebserverRoot);
  server.on("/Sensors", HTTP_GET, WebserverSensors);
  server.on("/Output", HTTP_GET, WebserverOutput);
  server.on("/POST", HTTP_POST, WebserverPOST);
  
  //Output config
  //Port Extension
  Inputs.StatesHW = MCPinit(mcp, MCPState);
  MQTT_SendInputStates();
  Output_Values.Outputstates = InitOutputStates(mcp, OutputsBasicSettings, MCPState, &Output_Values.OutputstatesAutoSSRelais); 

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
        MQTTclient->disconnect();
        MQTTinit();
      }
    }
    else
    {
      #ifdef BGTDEBUG
        Serial.println("No Link activ");
      #endif
    } 
  } 
  if(millis()>Break_10s)
  {
    Break_10s = millis() + 10000;
    SensorPort1.StartConversion();
    SensorPort2.StartConversion();
    MQTT_sendMessage("AirQuality/ADConverter", AirSensValues->getHWValue());
    MQTT_sendMessage("AirQuality/LPG", AirSensValues->readLPG());
    MQTT_sendMessage("AirQuality/CO", AirSensValues->readCO());
    MQTT_sendMessage("AirQuality/Smoke", AirSensValues->readSMOKE());
    MQTT_sendMessage("AirQuality/SensorState", AirSensValues->getSensorState());
    
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
        SensorPathName = "TempSensors/" + SensorPort1.GetSensorIndex(i)->getName();
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
        SensorPathName = "TempSensors/" + SensorPort2.GetSensorIndex(i)->getName();
        MQTT_sendMessage(SensorPathName.c_str(), SensorPort2.GetSensorIndex(i)->getTempC());
      }
    }
  }
  if(Output_Values.PWM_Manu_activ)
  {
    updateOutputPWM(mcp, &Output_Values);
  }
  if(readDigitalInputs_SetOutputIfAutoSSRMode(digitalRead(INTPortA), &Inputs, mcp, &Output_Values))
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
//MQTT Functions
bool MQTTinit()
{
  if((varConfig.NW_Flags & NW_MQTTActive)==0)
    return false;
  if(MQTTclient->connected())
    MQTTclient->disconnect();
  IPAddress IPTemp;
  IPTemp.fromString(varConfig.MQTT_Server);
  MQTTclient->setServer(IPTemp, varConfig.MQTT_Port);
  MQTTclient->setCallback(MQTT_callback);
  unsigned long int StartTime = millis();
  while ((millis() < (StartTime + 5000)&&(!MQTTclient->connect((varConfig.NW_Flags & NW_EthernetActive)?EthernetMAC:WiFi.macAddress().c_str() , varConfig.MQTT_Username, varConfig.MQTT_Password)))){
    delay(200);
  }
  if(MQTTclient->connected()){
    #ifdef BGTDEBUG
      Serial.println("MQTTclient connected");
    #endif
    String SubscribeRootTemp = varConfig.MQTT_rootpath;
    String SubscribeTemp = "";
    SubscribeRootTemp += SubscribeRoot[0];
    for(int i = 0; i < 8; i++)
    {
      SubscribeTemp = SubscribeRootTemp + OutputsBasicSettings[i].Name;
      MQTTclient->subscribe(SubscribeTemp.c_str());
    }
    SubscribeRootTemp = varConfig.MQTT_rootpath;
    SubscribeRootTemp += SubscribeRoot[1];
    SubscribeTemp = SubscribeRootTemp + "WLAN_active";
    MQTTclient->subscribe(SubscribeTemp.c_str());
    SubscribeTemp = SubscribeRootTemp + "AirSens_active";
    MQTTclient->subscribe(SubscribeTemp.c_str());
    return true;
  }
  else
  {
    #ifdef BGTDEBUG
      Serial.println("MQTTclient connection failure");
      Serial.print("MAC-Adresse: ");
      Serial.println((varConfig.NW_Flags & NW_EthernetActive)?EthernetMAC:WiFi.macAddress().c_str());
      Serial.print("Benutzername: ");
      Serial.println(varConfig.MQTT_Username);
      Serial.print("Passwort: ");
      Serial.println(varConfig.MQTT_Password);
      Serial.print("Port: ");
      Serial.println(varConfig.MQTT_Port);
      Serial.print("Server: ");
      Serial.println(varConfig.MQTT_Server);
    #endif
    
    return false;
  }
}
void MQTT_callback(char* topic, byte* payload, unsigned int length)
{
  String TempTopic = topic;
  String Value = (char*) payload;
  Value = Value.substring(0, length);
  
  if(TempTopic.substring(strlen(varConfig.MQTT_rootpath), (strlen(varConfig.MQTT_rootpath) + SubscribeRoot[0].length()))==SubscribeRoot[0])
  {
    int OutputIndex = FindOutputName(topic), ValueTemp = 0;
    if(OutputIndex < 0)
    { 
      String TempText = topic, TempPath = "Fehler";
      TempText += " -> ";
      TempText += (char) payload[0];
      MQTT_sendMessage(TempPath.c_str(), ( const uint8 *) TempText.c_str(), TempText.length());
      return;
    }
    ValueTemp = Value.toInt();
    SetOutput(OutputIndex, ValueTemp, &Output_Values, mcp);
    if(ValueTemp!=3)
      Output_Values.OutputstatesAutoSSRelais &= ~(1<<OutputIndex);
    if(ValueTemp==3)
      Output_Values.OutputstatesAutoSSRelais |= (1<<OutputIndex);
  }
  else if(TempTopic.substring(strlen(varConfig.MQTT_rootpath), (strlen(varConfig.MQTT_rootpath) + SubscribeRoot[1].length()))==SubscribeRoot[1])
  {
    if(TempTopic.substring(strlen(varConfig.MQTT_rootpath) + SubscribeRoot[1].length())=="WLAN_active")
    {
      switch(Value.toInt())
      {
        case 0: 
          if(((varConfig.NW_Flags & NW_EthernetActive) && Ethernet.linkStatus()== LinkON))
            WiFi.disconnect();
          break;
        case 1:
          if(!WiFi.isConnected())
            NetworkInit(true);
          break;
        default:
          break;
      }
    }
    else if(TempTopic.substring(strlen(varConfig.MQTT_rootpath) + SubscribeRoot[1].length())=="AirSens_active")
    {
      AirSensValues->setSensorState(Value.toInt());
    }
  }
}
int FindOutputName(const char* Topic)
{
  int pos = strlen(varConfig.MQTT_rootpath) + SubscribeRoot[0].length();
  for(int i = 0; i < 8; i++)
  {
    if(!strcmp(&Topic[pos], OutputsBasicSettings[i].Name))
      return i;
  }
  return -1;
}
void MQTT_SendInputStates()
{
  String Temp = "ratio_";
  MQTT_sendMessage("hwInput/InputPort", (uint8) ~Inputs.StatesHW);
  for(int i = 0; i<8; i++)
  {
    Temp = "ratio/";
    Temp += OutputsBasicSettings[i].Name;
    MQTT_sendMessage(Temp.c_str(), Inputs.OnTimeRatio[i]);
    Temp.replace("ratio", "hwInput");
    MQTT_sendMessage(Temp.c_str(), Inputs.StatesHW&1<<i?0:1);
  }
}
bool MQTT_sendMessage(const char * ValueName, const uint8* MSG, uint8 len)
{
  int lenPath = strlen(varConfig.MQTT_rootpath);
  char strPathVar[lenPath+20];

  sprintf(strPathVar, "%s/%s", varConfig.MQTT_rootpath, ValueName);
  return MQTTclient->publish(strPathVar, MSG, len, true);
  
}
bool MQTT_sendMessage(const char * ValueName, int MSG)
{
  return MQTT_sendMessage(ValueName, (const uint8*) IntToStr(MSG).c_str(), IntToStr(MSG).length());
}
bool MQTT_sendMessage(const char * ValueName, uint8 MSG)
{
  return MQTT_sendMessage(ValueName, (const uint8*) IntToStr(MSG).c_str(), IntToStr(MSG).length());
}
bool MQTT_sendMessage(const char * ValueName, uint32 MSG)
{
  return MQTT_sendMessage(ValueName, (const uint8*) IntToStr(MSG).c_str(), IntToStr(MSG).length());
}
bool MQTT_sendMessage(const char * ValueName, float MSG)
{
  return MQTT_sendMessage(ValueName, (const uint8*) IntToStr(MSG).c_str(), IntToStr(MSG).length());
}
//---------------------------------------------------------------------
//Einstellungen laden und Speichern im EEPROM bzw. Flash
//Settings read and save in EEPROM/Flash
void EinstSpeichern()
{
  uint8 Checksumme = 0;
  uint8 *pointer;
  pointer = (unsigned char *)&varConfig;
  for (unsigned int i = 0; i < sizeof(varConfig); i++)
    Checksumme += pointer[i];

  //EEPROM initialisieren
  EEPROM.begin(sizeof(varConfig) + sizeof(OutputsBasicSettings) + sizeof(TempSensors) + 14);

  EEPROM.put(0, varConfig);
  EEPROM.put(sizeof(varConfig) + 1, Checksumme);
  EEPROM.put(sizeof(varConfig) + 3, OutputsBasicSettings);
  EEPROM.put(sizeof(varConfig) + sizeof(OutputsBasicSettings) + 4, TempSensors);

  EEPROM.commit(); // Only needed for ESP8266 to get data written
  EEPROM.end();    // Free RAM copy of structure
}
bool EinstLaden()
{
  NWConfig varConfigTest;
  uint8 Checksumme = 0;
  uint8 ChecksummeEEPROM = 0;
  uint8 *pointer;
  pointer = (unsigned char *)&varConfigTest;
  //EEPROM initialisieren
  unsigned int EEPROMSize;
  EEPROMSize = sizeof(varConfig) + sizeof(OutputsBasicSettings) + sizeof(TempSensors) + 14;
  EEPROM.begin(EEPROMSize);

  EEPROM.get(0, varConfigTest);
  EEPROM.get(sizeof(varConfigTest) + 1, ChecksummeEEPROM);
  EEPROM.get(sizeof(varConfig) + 3, OutputsBasicSettings);
  EEPROM.get(sizeof(varConfig) + sizeof(OutputsBasicSettings) + 4, TempSensors);
  for (unsigned int i = 0; i < sizeof(varConfigTest); i++)
    Checksumme += pointer[i];
  if ((Checksumme == ChecksummeEEPROM) && (Checksumme != 0))
  {
    EEPROM.get(0, varConfig);
  }
  else
  {
    delay(200);
    EEPROM.end(); // Free RAM copy of structure
    return false;
  }
  delay(200);
  EEPROM.end(); // Free RAM copy of structure
  return true;
}
//---------------------------------------------------------------------
//Resetvariable die hochzaehlt bei vorzeitigem Stromverlust um auf Standard-Wert wieder zurueckzustellen.
void ResetVarSpeichern(char Count)
{
  EEPROM.begin(sizeof(varConfig) + sizeof(OutputsBasicSettings) + sizeof(TempSensors) + 14);

//  EEPROM.put(sizeof(varConfig) + sizeof(OutputsBasicSettings) + sizeof(TempSensors) + 10, Count);
  EEPROM.put(sizeof(varConfig) + 2, Count);

  EEPROM.commit(); // Only needed for ESP8266 to get data written
  EEPROM.end();    // Free RAM copy of structure
}
char ResetVarLesen()
{
  unsigned int EEPROMSize;
  char temp = 0;
  EEPROMSize = sizeof(varConfig) + sizeof(OutputsBasicSettings) + sizeof(TempSensors) + 14;
  EEPROM.begin(EEPROMSize);
//  EEPROM.get(EEPROMSize - 4, temp);
  EEPROM.get(sizeof(varConfig) + 2, temp);
  delay(200);
  EEPROM.end(); // Free RAM copy of structure
  return temp;
}
//---------------------------------------------------------------------
//Network functions
void NetworkInit(bool OnlyWiFi)
{
  //start WLAN
  if (varConfig.NW_Flags & NW_WiFi_AP)
  {
    WiFi_Start_AP();
  }
  else
  {
    WiFi_Start_STA(varConfig.WLAN_SSID, varConfig.WLAN_Password);
  }
  if(OnlyWiFi)
    return;
  if(varConfig.NW_Flags & NW_EthernetActive)  //If Ethernet active than MQTT should connected over Ethernet
  {
    Ethernet.init(D0);
    if(Ethernet.begin(mac)) //Configure IP address via DHCP
    {
      #ifdef BGTDEBUG
        Serial.println(Ethernet.localIP());
        Serial.println(Ethernet.gatewayIP());
        Serial.println(Ethernet.subnetMask());
      #endif
      e_client = new EthernetClient;
      if(varConfig.NW_Flags & NW_MQTTActive)
        MQTTclient = new PubSubClient(*e_client);
      ntpUDP = new EthernetUDP;
      MQTT_NTP_over_Ethernet_activ = 1;
    }
    else
    {
      if(varConfig.NW_Flags & NW_MQTTActive)
        MQTTclient = new PubSubClient(*wifiClient);
      ntpUDP = new WiFiUDP;
    }
  }
  else
  {
    if(varConfig.NW_Flags & NW_MQTTActive)
      MQTTclient = new PubSubClient(*wifiClient);
    ntpUDP = new WiFiUDP;
  }
}
//Wifi Funtkionen
void WiFi_Start_STA(char *ssid_sta, char *password_sta)
{
  unsigned long timeout;
  unsigned int Adresspuffer[4];
  if (varConfig.NW_Flags & NW_StaticIP)
  {
    sscanf(varConfig.NW_IPAddress, "%d.%d.%d.%d", &Adresspuffer[0], &Adresspuffer[1], &Adresspuffer[2], &Adresspuffer[3]);
    IPAddress IP(Adresspuffer[0], Adresspuffer[1], Adresspuffer[2], Adresspuffer[3]);
    sscanf(varConfig.NW_Gateway, "%d.%d.%d.%d", &Adresspuffer[0], &Adresspuffer[1], &Adresspuffer[2], &Adresspuffer[3]);
    IPAddress IPGate(Adresspuffer[0], Adresspuffer[1], Adresspuffer[2], Adresspuffer[3]);
    sscanf(varConfig.NW_SubMask, "%d.%d.%d.%d", &Adresspuffer[0], &Adresspuffer[1], &Adresspuffer[2], &Adresspuffer[3]);
    IPAddress IPSub(Adresspuffer[0], Adresspuffer[1], Adresspuffer[2], Adresspuffer[3]);
    sscanf(varConfig.NW_Gateway, "%d.%d.%d.%d", &Adresspuffer[0], &Adresspuffer[1], &Adresspuffer[2], &Adresspuffer[3]);
    IPAddress IPDNS(Adresspuffer[0], Adresspuffer[1], Adresspuffer[2], Adresspuffer[3]);
    WiFi.config(IP, IPDNS, IPGate, IPSub);
  }
  WiFi.mode(WIFI_STA); // Client
  WiFi.hostname(varConfig.NW_NetzName);
  WiFi.begin(ssid_sta, password_sta);
  timeout = millis() + 12000L;
  while (WiFi.status() != WL_CONNECTED && millis() < timeout)
  {
    delay(10);
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    //    server.begin();
    //    my_WiFi_Mode = WIFI_STA;
#ifdef BGTDEBUG
    Serial.print("Connected IP - Address : ");
    for (int i = 0; i < 3; i++)
    {
      Serial.print(WiFi.localIP()[i]);
      Serial.print(".");
    }
    Serial.println(WiFi.localIP()[3]);
#endif
  }
  else
  {
    WiFi.mode(WIFI_OFF);
#ifdef BGTDEBUG
    Serial.println("WLAN-Connection failed");
#endif
  }
}
void WiFi_Start_AP()
{
  WiFi.mode(WIFI_AP); // Accesspoint
                      //  WiFi.hostname(varConfig.NW_NetzName);

  WiFi.softAP(varConfig.WLAN_SSID, varConfig.WLAN_Password);
  //IPAddress myIP = WiFi.softAPIP();
  //  my_WiFi_Mode = WIFI_AP;
#ifdef BGTDEBUG
  Serial.print("Accesspoint started - Name : ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.print(WiFi.softAPIP());
  Serial.print(" Local IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("WiFi Mode: ");
  Serial.println(WiFi.getMode());
#endif
}
bool WIFIConnectionCheck(bool with_reconnect = true)
{
  if(WiFi.status()!= WL_CONNECTED)
  {
    if(with_reconnect)
    {
      WiFi.reconnect();
    }
    return false;
  }
  return true;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
//WebServer Functions
void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void WebserverRoot(AsyncWebServerRequest *request)
{
  char *Header_neu = new char[(strlen(html_header) + 50)];
  char *Body_neu = new char[(strlen(html_NWconfig)+750)];
  char *HTMLString = new char[(strlen(html_header) + 50)+(strlen(html_NWconfig)+750)];
  //Vorbereitung Datum
  unsigned long long epochTime = timeClient->getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon + 1;
  int currentYear = ptm->tm_year + 1900;

  char *pntSelected[5];
  for (int i = 0; i < 5; i++)
    if (i == (varConfig.NW_NTPOffset + 2))
      pntSelected[i] = (char *)varSelected[1].c_str();
    else
      pntSelected[i] = (char *)varSelected[0].c_str();
  sprintf(Header_neu, html_header, timeClient->getFormattedTime().c_str(), WeekDays[timeClient->getDay()].c_str(), monthDay, currentMonth, currentYear);
  sprintf(Body_neu, html_NWconfig, Un_Checked[varConfig.NW_Flags & NW_WiFi_AP].c_str(), varConfig.WLAN_SSID, 
              Un_Checked[(varConfig.NW_Flags & NW_EthernetActive)/NW_EthernetActive].c_str(), Un_Checked[(varConfig.NW_Flags & NW_StaticIP)/NW_StaticIP].c_str(), varConfig.NW_IPAddress, varConfig.NW_IPAddressEthernet, varConfig.NW_NetzName, varConfig.NW_SubMask, varConfig.NW_Gateway, varConfig.NW_DNS, 
              varConfig.NW_NTPServer, pntSelected[0], pntSelected[1], pntSelected[2], pntSelected[3], pntSelected[4], 
              Un_Checked[(varConfig.NW_Flags & NW_MQTTActive)/NW_MQTTActive].c_str(), varConfig.MQTT_Server, varConfig.MQTT_Port, varConfig.MQTT_Username, varConfig.MQTT_rootpath);
  sprintf(HTMLString, "%s%s", Header_neu, Body_neu);
  request->send(200, "text/html", HTMLString);
  delete[] HTMLString;
  delete[] Body_neu;
  delete[] Header_neu;
}
void WebserverSensors(AsyncWebServerRequest *request)
{
  int countSensors = SensorPort1.GetSensorCount() + SensorPort2.GetSensorCount();
  TempSensor * MissingSensors[MaxSensors];
  uint8 countMissingSensors = FindMissingSensors(&SensorPort1, &SensorPort2, MissingSensors, TempSensors, MaxSensors);
  uint16 StringLen = (strlen(html_header) + 50)+strlen(html_SEconfig1)+(countSensors * (strlen(html_SEconfig2) + 100))+strlen(html_SEconfig3);
  StringLen += ((strlen(html_SEconfig4)+100)*countMissingSensors) + strlen(html_OPSEfooter);
  char *HTMLString = new char[StringLen];
  char *HTMLString2 = new char[StringLen];
  //Vorbereitung Datum 
  unsigned long long epochTime = timeClient->getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon + 1;
  int currentYear = ptm->tm_year + 1900;
  sprintf(HTMLString, html_header, timeClient->getFormattedTime().c_str(), WeekDays[timeClient->getDay()].c_str(), monthDay, currentMonth, currentYear);
  sprintf(HTMLString2, html_SEconfig1, HTMLString, Ein_Aus[AirSensValues->getSensorState()].c_str(), AirSensValues->getOnTime_s(), AirSensValues->readLPG(), AirSensValues->readCO(), 
                  AirSensValues->readSMOKE(), AirSensValues->getHWValue());
  for(int i = 0; i < SensorPort1.GetSensorCount(); i++)
  {
    sprintf(HTMLString, html_SEconfig2, HTMLString2, SensorPort1.GetSensorIndex(i)->getAddressHEX().c_str(), SensorPort1.GetSensorIndex(i)->getTempC(), 1, 
                  SensorPort1.GetSensorIndex(i)->getAddressUINT64(), SensorPort1.GetSensorIndex(i)->getName().c_str(), 1, SensorPort1.GetSensorIndex(i)->getAddressUINT64(), 
                  SensorPort1.GetSensorIndex(i)->getOffset(), 1, SensorPort1.GetSensorIndex(i)->getAddressUINT64(), FindTempSensor(TempSensors, MaxSensors, 
                  SensorPort1.GetSensorIndex(i)->getAddressUINT64())->SensorState);
    strcpy(HTMLString2, HTMLString);
  }
  for(int i = 0; i < SensorPort2.GetSensorCount(); i++)
  {
    sprintf(HTMLString, html_SEconfig2, HTMLString2, SensorPort2.GetSensorIndex(i)->getAddressHEX().c_str(), SensorPort2.GetSensorIndex(i)->getTempC(), 2, 
                  SensorPort2.GetSensorIndex(i)->getAddressUINT64(), SensorPort2.GetSensorIndex(i)->getName().c_str(), 2, SensorPort2.GetSensorIndex(i)->getAddressUINT64(), 
                  SensorPort2.GetSensorIndex(i)->getOffset(), 2, SensorPort2.GetSensorIndex(i)->getAddressUINT64(), FindTempSensor(TempSensors, MaxSensors, 
                  SensorPort2.GetSensorIndex(i)->getAddressUINT64())->SensorState);
    strcpy(HTMLString2, HTMLString);
  }
  sprintf(HTMLString, html_SEconfig3, HTMLString2);
  for(int i = 0; i < countMissingSensors; i++)
  {
    sprintf(HTMLString2, html_SEconfig4, HTMLString, convertUINT64toHEXstr(&MissingSensors[i]->Address).c_str(), MissingSensors[i]->Name, MissingSensors[i]->Offset, 
                  MissingSensors[i]->SensorState, MissingSensors[i]->Address);
    strcpy(HTMLString, HTMLString2);  
  }
  sprintf(HTMLString2, html_OPSEfooter, HTMLString);
  request->send(200, "text/html", HTMLString2);
  delete[] HTMLString;
  delete[] HTMLString2;
}
void WebserverOutput(AsyncWebServerRequest *request)
{
  uint16 StringLen = (strlen(html_header) + 50)+strlen(html_OPconfig1)+(8 * (strlen(html_OPconfig2) + 100)) + strlen(html_OPSEfooter);
  char *HTMLString = new char[StringLen];
  char *HTMLString2 = new char[StringLen];
  //Vorbereitung Datum 
  unsigned long long epochTime = timeClient->getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon + 1;
  int currentYear = ptm->tm_year + 1900;
  int TempCurrentOutputState = 0;
  sprintf(HTMLString, html_header, timeClient->getFormattedTime().c_str(), WeekDays[timeClient->getDay()].c_str(), monthDay, currentMonth, currentYear);
  sprintf(HTMLString2, "%s%s", HTMLString, html_OPconfig1);
  for(int i = 0; i < 8; i++)
  {
    TempCurrentOutputState = (~Output_Values.Outputstates &((uint16) 1<<(i+8)))/((uint16)1<<(i+8)); //Manuel On or Off
    TempCurrentOutputState = (Output_Values.Outputstates &((uint16) 1<<i))?2:TempCurrentOutputState; //Auto or Manuell
    TempCurrentOutputState = Output_Values.OutputstatesAutoSSRelais&((uint8) 1<<i)?3:TempCurrentOutputState; //Auto over Solid state relais
    sprintf(HTMLString, html_OPconfig2, HTMLString2, i, i, OutputsBasicSettings[i].Name, TempCurrentOutputState, Inputs.OnTimeRatio[i], i, OutputsBasicSettings[i].StartValue, i, 
                  Un_Checked[OutputsBasicSettings[i].MQTTState%2].c_str());
    strcpy(HTMLString2, HTMLString);
  }
  sprintf(HTMLString2, html_OPSEfooter, HTMLString);
  request->send(200, "text/html", HTMLString2);
  delete[] HTMLString;
  delete[] HTMLString2;
}
void WebserverPOST(AsyncWebServerRequest *request)
{
  int parameter = request->params();
  unsigned short int *submitBereich;
  if (parameter)
  {
    submitBereich = (unsigned short int *)request->getParam(0)->name().c_str();
    switch (*submitBereich)
    {
    case subwl:
      varConfig.NW_Flags &= ~NW_WiFi_AP;
      if(parameter <= 3)
      for (int i = 0; i < parameter; i++)
      {
        if (request->getParam(i)->name() == "wlAP")
          varConfig.NW_Flags |= NW_WiFi_AP;
        else if (request->getParam(i)->name() == "wlSSID")
        {
          if (request->getParam(i)->value().length() < 40)
            strcpy(varConfig.WLAN_SSID, request->getParam(i)->value().c_str());
          else
          {
            request->send_P(200, "text/html", "SSID zu lang<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
            return;
          }
        }
        else if (request->getParam(i)->name() == "wlPassword")
        {
          if(request->getParam(i)->value()!="xxxxxx")
          {
            if ((request->getParam(i)->value().length() <= 60)&&(request->getParam(i)->value().length() >= 8))
            { 
              strcpy(varConfig.WLAN_Password, request->getParam(i)->value().c_str());
            }
            else
            {
              request->send_P(200, "text/html", "Passwortlaenge muss zwischen 8 und 60 Zeichen liegen<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
              return;
            }
          }
        }
        else
        {
          request->send_P(200, "text/html", "Unbekannter Rueckgabewert<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
          return;
        }
      }
      request->send_P(200, "text/html", "Daten wurden uebernommen und ESP wird neu gestartet!<br><a href=\\Settings\\>Zurueck</a> <meta http-equiv=\"refresh\" content=\"20; URL=\\\">"); //<a href=\>Startseite</a>
      EinstSpeichern();
      ESP_Restart = true;
      break;
    case subnw:
    {
      char tmp_StatischeIP = 0;
      char tmp_EthernetOn = 0;
      String tmp_IPAddress[5];
      String tmp_NTPServer;
      String tmp_NetzName;
      int tmp_NTPOffset;
      if(parameter < 11)
      for (int i = 0; i < parameter; i++)
      {
        if (request->getParam(i)->name() == "nwEthernetOn")
          tmp_EthernetOn = 1;
        else if (request->getParam(i)->name() == "nwSIP")
          tmp_StatischeIP = 1;
        else if (request->getParam(i)->name() == "nwIP")
          tmp_IPAddress[0] = request->getParam(i)->value();
        else if (request->getParam(i)->name() == "nwIPEthernet")
          tmp_IPAddress[1] = request->getParam(i)->value();
        else if (request->getParam(i)->name() == "nwNetzName")
          tmp_NetzName = request->getParam(i)->value();
        else if (request->getParam(i)->name() == "nwSubnet")
          tmp_IPAddress[2] = request->getParam(i)->value();
        else if (request->getParam(i)->name() == "nwGateway")
          tmp_IPAddress[3] = request->getParam(i)->value();
        else if (request->getParam(i)->name() == "nwDNS")
          tmp_IPAddress[4] = request->getParam(i)->value();
        else if (request->getParam(i)->name() == "nwNTPServer")
          tmp_NTPServer = request->getParam(i)->value();
        else if (request->getParam(i)->name() == "nwNTPOffset")
          sscanf(request->getParam(i)->value().c_str(), "%d", &tmp_NTPOffset);
        else
        {
          request->send_P(200, "text/html", "Unbekannter Rueckgabewert<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
          return;
        }
      }
      if (tmp_StatischeIP)
        if ((tmp_IPAddress[0].length() == 0) || (tmp_IPAddress[1].length() == 0))
        {
          request->send_P(200, "text/html", "Bei Statischer IP-Adresse wird eine IP-Adresse und eine Subnet-Mask benoetigt!<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
          return;
        }
      if(tmp_StatischeIP){
        varConfig.NW_Flags |= NW_StaticIP;}
      else{
        varConfig.NW_Flags &= ~NW_StaticIP;}
      if(tmp_EthernetOn){
        varConfig.NW_Flags |= NW_EthernetActive;}
      else{
        varConfig.NW_Flags &= ~NW_EthernetActive;}
      strcpy(varConfig.NW_IPAddress, tmp_IPAddress[0].c_str());
      strcpy(varConfig.NW_IPAddressEthernet, tmp_IPAddress[1].c_str());
      strcpy(varConfig.NW_NetzName, tmp_NetzName.c_str());
      strcpy(varConfig.NW_SubMask, tmp_IPAddress[2].c_str());
      strcpy(varConfig.NW_Gateway, tmp_IPAddress[3].c_str());
      strcpy(varConfig.NW_DNS, tmp_IPAddress[4].c_str());
      strcpy(varConfig.NW_NTPServer, tmp_NTPServer.c_str());
      varConfig.NW_NTPOffset = tmp_NTPOffset;
      request->send_P(200, "text/html", "Daten wurden uebernommen und ESP wird neu gestartet!<br><meta http-equiv=\"refresh\" content=\"20; URL=\\\">"); //<a href=\>Startseite</a>
      EinstSpeichern();
      ESP_Restart = true;
      break;
    }
    case submq:
    {
      char tmp_MQTTOn = 0;
      String Temp[6];
      if(parameter <= 6)
      for (int i = 0; i < parameter; i++)
      {
        if (request->getParam(i)->name() == "mqMQTTOn")
          tmp_MQTTOn = 1;
        else if (request->getParam(i)->name() == "mqServer")
          Temp[0] = request->getParam(i)->value();
        else if (request->getParam(i)->name() == "mqPort")
          Temp[1] = request->getParam(i)->value();
        else if (request->getParam(i)->name() == "mqUser")
          Temp[2] = request->getParam(i)->value();
        else if (request->getParam(i)->name() == "mqPassword")
          Temp[3] = request->getParam(i)->value();
        else if (request->getParam(i)->name() == "mqRootpath")
          Temp[4] = request->getParam(i)->value();
        else if (request->getParam(i)->name() == "mqFPrint")
          Temp[5] = request->getParam(i)->value();
        else
        {
          request->send_P(200, "text/html", "Unbekannter Rueckgabewert<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
          return;
        }
      }
      if(tmp_MQTTOn){
        varConfig.NW_Flags |= NW_MQTTActive;}
      else{
        varConfig.NW_Flags &= ~NW_MQTTActive;}
      if((Temp[0].length()<49)&&(Temp[0].length()>5))
        strcpy(varConfig.MQTT_Server, Temp[0].c_str());
      if((Temp[1].length()<6)&&(Temp[1].length()>1))
        varConfig.MQTT_Port = Temp[1].toInt();
      if((Temp[2].length()<19)&&(Temp[2].length()>5))
        strcpy(varConfig.MQTT_Username, Temp[2].c_str());
      if((Temp[3].length()<=60)&&(Temp[3].length()>=5)&&(Temp[3]!= "xxxxxx"))
        strcpy(varConfig.MQTT_Password, Temp[3].c_str());
      if((Temp[4].length()<95)&&(Temp[4].length()>5))
        strcpy(varConfig.MQTT_rootpath, Temp[4].c_str());
      if((Temp[5].length()<=65)&&(Temp[5].length()>5)&&(Temp[5]!= "xxxxxx"))
        strcpy(varConfig.MQTT_fprint, Temp[5].c_str());
      EinstSpeichern();
      request->send_P(200, "text/html", "MQTT Daten wurden uebernommen, ESP wird neu gestartet!<br><meta http-equiv=\"refresh\" content=\"20; URL=\\\">"); //<a href=\>Startseite</a>
      ESP_Restart = true;
      break;
    }               
    case subPS:
    {
      uint64 Address = 0;
      int Port = 0;
      int Value = 0;
      float TempOffset = 0;
      unsigned int Temp = 0;
      for (int i = 0; i < parameter; i++)
      {
        if(sscanf(request->getParam(i)->name().c_str(), "PS_%d_%d_", &Port, &Value) != 2)
        {          
          request->send_P(200, "text/html", "Unbekannter Rueckgabewert Index 720<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
          return;
        }
        Address = StrToLongInt(request->getParam(i)->name().substring(6));
        switch(Value)
        {
          case 1:
            if(request->getParam(i)->value().length()>15)
            {
              request->send_P(200, "text/html", "Unbekannter Rueckgabewert Index 729<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
              return;
            }
            strcpy(FindTempSensor(TempSensors, MaxSensors, Address)->Name, request->getParam(i)->value().c_str());
            if(Port == 1)
              SensorPort1.GetSensorAddr(Address)->setName(request->getParam(i)->value());
            else
              SensorPort2.GetSensorAddr(Address)->setName(request->getParam(i)->value());
            break;
          case 2:
            if(sscanf(request->getParam(i)->value().c_str(),"%f", &TempOffset)!=1)
            {
              request->send_P(200, "text/html", "Unbekannter Rueckgabewert Index 741<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
              return;
            }
            FindTempSensor(TempSensors, MaxSensors, Address)->Offset = TempOffset;
            if(Port == 1)
              SensorPort1.GetSensorAddr(Address)->setOffset(TempOffset);
            else
              SensorPort2.GetSensorAddr(Address)->setOffset(TempOffset);
            break;
          case 3:
            if(sscanf(request->getParam(i)->value().c_str(),"%u", &Temp)!=1)
            {
              request->send_P(200, "text/html", "Unbekannter Rueckgabewert Index 753<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
              return;
            }
            FindTempSensor(TempSensors, MaxSensors, Address)->SensorState = Temp;
            break;
          default:
            request->send_P(200, "text/html", "Unbekannter Rueckgabewert Index 763<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
            return;
        }
      }
      EinstSpeichern();
      request->send_P(200, "text/html", "Sensordaten wurden uebernommen!<br><meta http-equiv=\"refresh\" content=\"2; URL=/Sensors/\">"); //<a href=\>Startseite</a>
      break;
    }
    case subSD:
    {
      if((parameter > 1)||(request->getParam(0)->name()!="SDelete"))
      {
        request->send_P(200, "text/html", "Unbekannter Rueckgabewert Index 772<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
        return;
      }
      TempSensor * Temp = 0;
      uint64 Address = StrToLongInt(request->getParam(0)->value());
      Temp = FindTempSensor(TempSensors, MaxSensors, Address);
      if(Temp)
      DelTSensor(Temp);
      EinstSpeichern();
      request->send_P(200, "text/html", "Sensor wurde geloescht!<br><meta http-equiv=\"refresh\" content=\"2; URL=/Sensors/\">"); //<a href=\>Startseite</a>
      //ESP_Restart = true;
      break;
    }   
    case subSS:
    {
      SensorPort1.SensorSearch();
      SensorPort2.SensorSearch();
      TakeoverTSConfig(&SensorPort1, TempSensors, MaxSensors);
      TakeoverTSConfig(&SensorPort2, TempSensors, MaxSensors);
      request->send_P(200, "text/html", "Sensoren wurden neu eingelesen!<br><meta http-equiv=\"refresh\" content=\"2; URL=/Sensors/\">"); //<a href=\>Startseite</a>
      break;
    }            
    case subAS:
    {
      if(AirSensValues->toggleSensorState())
      {
        request->send_P(200, "text/html", "Luft-Sensor wurde eingeschaltet!<br><meta http-equiv=\"refresh\" content=\"2; URL=/Sensors/\">");
      }
      else
      {
        request->send_P(200, "text/html", "Luft-Sensor wurde ausgeschaltet!<br><meta http-equiv=\"refresh\" content=\"2; URL=/Sensors/\">");
      }
      break;
    }            
    case subOS:
    {
      int Port = 0;
      int Value = 0;
      for (int i = 0; i < parameter; i++)
      {
        if(sscanf(request->getParam(i)->name().c_str(), "OS_%d_%d_", &Port, &Value) != 2)
        {          
          request->send_P(200, "text/html", "Unbekannter Rueckgabewert Index 808<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
          return;
        }
        OutputsBasicSettings[Port].MQTTState = 0;
        switch(Value)
        {
          case 1:
            if(request->getParam(i)->value().length()>15)
            {
              request->send_P(200, "text/html", "Name zu lang Index 814<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
              return;
            }
            strcpy(OutputsBasicSettings[Port].Name, request->getParam(i)->value().c_str());
            break;
          case 2:
            if((request->getParam(i)->value().toInt() > 3) || (request->getParam(i)->value().toInt() < 0))
            {
              request->send_P(200, "text/html", "Unbekannter Rueckgabewert Index 822<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
              return;
            }
            OutputsBasicSettings[Port].StartValue = request->getParam(i)->value().toInt();
            if(OutputsBasicSettings[Port].StartValue == 3)
            {
              Output_Values.OutputstatesAutoSSRelais |= (1<<Port);
            }
            else
            {
              Output_Values.OutputstatesAutoSSRelais &= ~(1<<Port);
            }
            break;
          case 3:
            if(request->getParam(i)->value()!="on")
            {
              request->send_P(200, "text/html", "Unbekannter Rueckgabewert Index 830<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
              return;
            }
            OutputsBasicSettings[Port].MQTTState = 1;
            break;
          default:
            request->send_P(200, "text/html", "Unbekannter Rueckgabewert Index 836<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
            return;
        }
      }
      EinstSpeichern();
      request->send_P(200, "text/html", "Relaiseinstellungen wurden uebernommen!<br><meta http-equiv=\"refresh\" content=\"2; URL=/Output/\">"); //<a href=\>Startseite</a>
      break;
   }            
    default:
      char strFailure[50];
      sprintf(strFailure, "Anweisung unbekannt, Empfangen: %u", *submitBereich);
      request->send_P(200, "text/html", strFailure);
      break;
    }
  }

}