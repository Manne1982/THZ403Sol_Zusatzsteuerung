#include <Arduino.h>
#include <OneWire.h>
//für Webserver und Ethernet notwendig
#include <EthernetENC.h>
#include <AsyncWebServer_Ethernet.h>
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

//MQTT Variablen
EthernetClient client;
PubSubClient MQTTclient(client);


//Funktionsdefinitionen
void notFound(AsyncWebServerRequest *request);
void EinstSpeichern();
void EinstLaden();
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
unsigned long Break_h = 0;
unsigned long Break_10s = 0;
//unsigned long Break_s = 0;

//Erstellen Serverelement
EthernetServer * pEthernetServer;
AsyncWebServer * server;

//Uhrzeit Variablen
EthernetUDP ntpUDP;
NTPClient *timeClient;


void setup(void) {
  uint8_t mac[6] = {0xA0,0xA1,0xA2,0xA3,0xA4,0xA5};
  Serial.begin(9600);
  delay(3000);
/*  char ResetCount = 0;
  ResetCount = ResetVarLesen();
  if((ResetCount < 0)||(ResetCount > 5))  //Prüfen ob Wert Plausibel, wenn nicht rücksetzen
    ResetCount = 0;
  //ResetCount++;
  ResetVarSpeichern(ResetCount);
  delay(5000);
  if (ResetCount < 5) //Wenn nicht 5 mal in den ersten 5 Sekunden der Startvorgang abgebrochen wurde
    EinstLaden();
  ResetVarSpeichern(0);*/
  
  Ethernet.init(D0);
  if(Ethernet.begin(mac)) //Configure IP address via DHCP
  {
    #ifdef BGTDEBUG
      Serial.println(Ethernet.localIP());
      Serial.println(Ethernet.gatewayIP());
      Serial.println(Ethernet.subnetMask());
    #endif
    //MQTT
    MQTTinit();
  }
  
  server = new AsyncWebServer(80);
  //Zeitserver Einstellungen
  if (strlen(varConfig.NW_NTPServer))
    timeClient = new NTPClient(ntpUDP, (const char *)varConfig.NW_NTPServer);
  else
    timeClient = new NTPClient(ntpUDP, "fritz.box");
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
  //Webserver
  server->onNotFound(notFound);
  server->on("/", HTTP_GET, [](AsyncWebServerRequest *request)
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
              sprintf(Body_neu, html_NWconfig, Un_Checked[(short)varConfig.WLAN_AP_Aktiv].c_str(), varConfig.WLAN_SSID, Un_Checked[(short)varConfig.NW_StatischeIP].c_str(), varConfig.NW_IPAdresse, varConfig.NW_NetzName, varConfig.NW_SubMask, varConfig.NW_Gateway, varConfig.NW_DNS, varConfig.NW_NTPServer, pntSelected[0], pntSelected[1], pntSelected[2], pntSelected[3], pntSelected[4], varConfig.MQTT_Server, varConfig.MQTT_Port, varConfig.MQTT_Username, varConfig.MQTT_rootpath);
              sprintf(HTMLString, "%s%s", Header_neu, Body_neu);
              request->send(200, "text/html", HTMLString);
              delete[] HTMLString;
              delete[] Body_neu;
              delete[] Header_neu;
            });
  server->begin();
}

void loop(void) {
  //OTA
  ArduinoOTA.handle();

  //MQTT wichtige Funktion
  if(Ethernet.linkStatus()== LinkON)  //MQTTclient.loop() wichtig damit die Daten im Hintergrund empfangen werden
  {
    if(!MQTTclient.loop())
    {
      MQTTclient.disconnect();
      MQTTinit();
    }
  }
  else
  {
    #ifdef BGTDEBUG
      Serial.println("No Link activ");
    #endif
  }  
  if(millis()>Break_10s)
  {
    Break_10s = millis() + 10000;
    Serial.print(timeClient->getFormattedTime());
    Serial.print(" ");
    Serial.println(Ethernet.localIP());
  }
  if (Break_h < millis())
  {
    Break_h = millis() + 3600000;
    timeClient->update();
  }
  //MQTT
  MQTTclient.loop();
}

bool MQTTinit()
{
  if(MQTTclient.connected())
    MQTTclient.disconnect();
  IPAddress IPTemp;
  IPTemp.fromString("192.168.63.102");
  MQTTclient.setServer(IPTemp, 1883);
  MQTTclient.setCallback(MQTT_callback);
  unsigned long int StartTime = millis();
  while ((millis() < (StartTime + 5000)&&(!MQTTclient.connect("A0:A1:A2:A3:A4:A5", "mrmqtt", "ReDam")))){
    delay(200);
  }
  if(MQTTclient.connected()){
    Serial.println("MQTTclient connected");
    MQTTclient.subscribe("/Test");
    return true;
  }
  else
    return false;
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
void EinstLaden()
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
    Serial.println("Fehler beim Dateneinlesen");

  delay(200);
  EEPROM.end(); // Free RAM copy of structure
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