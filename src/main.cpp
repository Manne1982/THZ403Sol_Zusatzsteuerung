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
//Allgemeine Bibliotheken
#include <string.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>
//Projektspezifisch
#include "HTML_Var.h"
#include "Klassen_HZS.h"
//Port extension
#include <Adafruit_MCP23X17.h>


#define BGTDEBUG 1

//Funktionsdefinitionen
void WiFi_Start_STA(char *ssid_sta, char *password_sta);
void WiFi_Start_AP();
bool WIFIConnectionCheck(bool with_reconnect);
void notFound(AsyncWebServerRequest *request);
void EinstSpeichern();
bool EinstLaden();
char ResetVarLesen();
void ResetVarSpeichern(char Count);
bool MQTTinit();  //Wenn verbunden Rückgabewert true
void MQTT_callback(char* topic, byte* payload, unsigned int length);
String IntToStr(int _var);
String IntToStr(float _var);
String IntToStrHex(int _var);
String IntToStr(uint32_t _var);

//Projektvariablen
NWConfig varConfig;
char EthernetMAC[] = "A0:A1:A2:A3:A4:A5";         //For Ethernet connection (MQTT)
uint8_t mac[6] = {0xA0,0xA1,0xA2,0xA3,0xA4,0xA5}; //For Ethernet connection
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
//Port extension
Adafruit_MCP23X17 mcp;



void setup(void) {
  wifiClient = new WiFiClient;
  Serial.begin(9600);
  delay(3000);
  uint8_t ResetCount = 0;
  ResetCount = ResetVarLesen();
  if(ResetCount > 5)  //Prüfen ob Wert Plausibel, wenn nicht rücksetzen
    ResetCount = 0;
  //ResetCount++;
  ResetVarSpeichern(ResetCount);
  delay(5000);
  D0
  if (ResetCount < 5) //Wenn nicht 5 mal in den ersten 5 Sekunden der Startvorgang abgebrochen wurde
    if(!EinstLaden()) //If failure than standard config will be saved
      EinstSpeichern();
  ResetVarSpeichern(0);
  //WLAN starten
  if (varConfig.NW_Flags & NW_WiFi_AP)
  {
    WiFi_Start_AP();
  }
  else
  {
    WiFi_Start_STA(varConfig.WLAN_SSID, varConfig.WLAN_Password);
  }
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
    }
    e_client = new EthernetClient;
    if(varConfig.NW_Flags & NW_MQTTActive)
      MQTTclient = new PubSubClient(*e_client);
    ntpUDP = new EthernetUDP;
  }
  else
  {
    if(varConfig.NW_Flags & NW_MQTTActive)
      MQTTclient = new PubSubClient(*wifiClient);
    ntpUDP = new WiFiUDP;
  }
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
//  server = new AsyncWebServer(8080);
  server.begin();
  server.onNotFound(notFound);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              char *Header_neu = new char[(strlen(html_header) + 50)];
              char *Body_neu = new char[(strlen(html_NWconfig)+750)];
              char *HTMLString = new char[(strlen(html_header) + 50)+(strlen(html_NWconfig)+750)];
              //Vorbereitung Datum
              unsigned long epochTime = timeClient->getEpochTime();
              struct tm *ptm = gmtime((time_t *)&epochTime);
              int monthDay = ptm->tm_mday;
              int currentMonth = ptm->tm_mon; // + 1;
              int currentYear = ptm->tm_year; // + 1900;
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
            });
  server.on("/POST", HTTP_POST, [](AsyncWebServerRequest *request)
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
                      if (request->getParam(i)->value().length() <= 60)
                        strcpy(varConfig.WLAN_Password, request->getParam(i)->value().c_str());
                      else
                      {
                        request->send_P(200, "text/html", "Passwort zu lang<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
                        return;
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
                default:
                  char strFailure[50];
                  sprintf(strFailure, "Anweisung unbekannt, Empfangen: %u", *submitBereich);
                  request->send_P(200, "text/html", strFailure);
                  break;
                }
              }
            });
  
  //Port Extension
  if (!mcp.begin_I2C(39)) {
    Serial.println("MCP not connected!");
  }
  else{
    Serial.println("MCP connected!");
  }
  //Only for MCP28017 Test
  // configure all pin for output
  for(int i =0; i <16; i++)
    if(i<8)
    {
      mcp.pinMode(i, INPUT_PULLUP);
      mcp.setupInterruptPin(i, CHANGE);
    }
    else
      mcp.pinMode(i, OUTPUT);
}

void loop(void) {
  //OTA
  ArduinoOTA.handle();
  //Test
  static uint8_t TestCounter = 0; 

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
    Serial.print(timeClient->getFormattedTime());
    Serial.print(" ");
    Serial.println(Ethernet.localIP());
    //Only for MCP28017 Test
    mcp.writeGPIOB(TestCounter);
    TestCounter++;
  }
  if(millis()>Break_s)
  {
    Break_s = millis() + 1000;
    //Only for MCP28017 Test
    Serial.println(mcp.readGPIOA());
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
  //Restart for WWW-Requests
  if(ESP_Restart)
  {
    delay(1000);
    ESP.restart();
  }

}

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
    Serial.println("MQTTclient connected");
    MQTTclient->subscribe("/Test");
    return true;
  }
  else
  {
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
    
    return false;
  }
}
//MQTT-Funktionen
void MQTT_callback(char* topic, byte* payload, unsigned int length)
{
  char payloadTemp[length + 2];
  for (unsigned int i = 0; i < length; i++){
    payloadTemp[i] = (char) payload[i];
  }
  payloadTemp[length] = 0;
  Serial.println(payloadTemp);
}
void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}
//---------------------------------------------------------------------
//Einstellungen laden und Speichern im EEPROM bzw. Flash
void EinstSpeichern()
{
  unsigned long int Checksumme = 0;
  unsigned char *pointer;
  pointer = (unsigned char *)&varConfig;
  for (unsigned int i = 0; i < sizeof(varConfig); i++)
    Checksumme += pointer[i];

  //EEPROM initialisieren
  EEPROM.begin(sizeof(varConfig) + 14);

  EEPROM.put(0, varConfig);
  EEPROM.put(sizeof(varConfig) + 1, Checksumme);

  EEPROM.commit(); // Only needed for ESP8266 to get data written
  EEPROM.end();    // Free RAM copy of structure
}
bool EinstLaden()
{
  NWConfig varConfigTest;
  unsigned long int Checksumme = 0;
  unsigned long int ChecksummeEEPROM = 0;
  unsigned char *pointer;
  pointer = (unsigned char *)&varConfigTest;
  //EEPROM initialisieren
  unsigned int EEPROMSize;
  EEPROMSize = sizeof(varConfig) + 14;
  EEPROM.begin(EEPROMSize);

  EEPROM.get(0, varConfigTest);
  EEPROM.get(sizeof(varConfigTest) + 1, ChecksummeEEPROM);

  for (unsigned int i = 0; i < sizeof(varConfigTest); i++)
    Checksumme += pointer[i];
  if ((Checksumme == ChecksummeEEPROM) && (Checksumme != 0))
  {
    EEPROM.get(0, varConfig);
  }
  else
    return false;

  delay(200);
  EEPROM.end(); // Free RAM copy of structure
  return true;
}
//---------------------------------------------------------------------
//Resetvariable die hochzaehlt bei vorzeitigem Stromverlust um auf Standard-Wert wieder zurueckzustellen.
void ResetVarSpeichern(char Count)
{
  EEPROM.begin(sizeof(varConfig) + 14);

  EEPROM.put(sizeof(varConfig) + 10, Count);

  EEPROM.commit(); // Only needed for ESP8266 to get data written
  EEPROM.end();    // Free RAM copy of structure
}
char ResetVarLesen()
{
  unsigned int EEPROMSize;
  char temp = 0;
  EEPROMSize = sizeof(varConfig) + 14;
  EEPROM.begin(EEPROMSize);
  EEPROM.get(EEPROMSize - 4, temp);
  delay(200);
  EEPROM.end(); // Free RAM copy of structure
  return temp;
}
//---------------------------------------------------------------------
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



/*
OneWire  ds(D2);  // on pin 10 (a 4.7K resistor is necessary)
TSensor * SensorList[10];
u_int8_t SensorListCount = 0;
unsigned long Break_10s = 0;                                //Variable fuer Dinge die alle 1s ausgefuehrt werden

void setup(void) {
  Serial.begin(9600);
  for(int i = 0; i < 10; i++)
  {
    SensorList[i] = new TSensor(&ds);
    if(!SensorList[i]->SensSearch())
    {
      delete SensorList[i];
      SensorListCount = i;
      break;
    }
    char Temp[15];
    sprintf(Temp, "Sensor %d", i);
    SensorList[i]->setName(Temp);
  }
}

void loop(void) {
  if(millis()>Break_10s)
  {
    Break_10s = millis() + 10000;
    for(int i = 0; i < SensorListCount; i++)
    {
      SensorList[i]->startConversion();
    }
  }
  for(int i = 0; i < SensorListCount; i++)
  {
    SensorList[i]->loop();
    if(SensorList[i]->NewValueAvailable())
    {
      Serial.print("Neuer Temp Wert für Sensor ");
      Serial.print(SensorList[i]->getName());
      Serial.print(": ");
      Serial.print(SensorList[i]->getTempC());
      Serial.println(" °C"); 
    }
  }

}
*/