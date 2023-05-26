
#include "ProjectFunctions.h"
#include "GlobalVariabels.h"
#include <Arduino.h>
#include <EEPROM.h>
#include "WiFiFunctions.h"
#include "HZS_Functions_Classes.h"
#include <WiFiClient.h>


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
    #ifdef BGTDEBUG
      Serial.println("Ethernet initiiert");
    #endif

    if(Ethernet.begin(mac)) //Configure IP address via DHCP
    {
      #ifdef BGTDEBUG
        Serial.println(Ethernet.localIP());
        Serial.println(Ethernet.gatewayIP());
        Serial.println(Ethernet.subnetMask());
      #endif
      e_client = new EthernetClient;
      if(varConfig.NW_Flags & NW_MQTTActive)
      {
        MQTTclient = new PubSubClient(*e_client);
        #ifdef BGTDEBUG
          Serial.println("MQTTclient Variable mit Ethernet-Client erstellt");
        #endif
      }
      ntpUDP = new EthernetUDP;
    }
    else
    {
      #ifdef BGTDEBUG
        Serial.println("Ethernet starten fehlgeschlagen");
      #endif
      if(varConfig.NW_Flags & NW_MQTTActive)
        MQTTclient = new PubSubClient(*wifiClient);
      #ifdef BGTDEBUG
        Serial.println("MQTT client auf WLAN gestartet");
      #endif
      ntpUDP = new WiFiUDP;
      #ifdef BGTDEBUG
        Serial.println("UDP Variable für NTP erstellt");
      #endif
      varConfig.NW_Flags &= ~NW_EthernetActive;
    }
  }
  else
  {
    if(varConfig.NW_Flags & NW_MQTTActive)
      MQTTclient = new PubSubClient(*wifiClient);
    ntpUDP = new WiFiUDP;
  }
}
