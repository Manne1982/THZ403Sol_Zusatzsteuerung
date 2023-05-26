#include "WiFiFunctions.h"
#include "GlobalVariabels.h"
//Für Wifi-Verbindung notwendig
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
//für Webserver und Ethernet notwendig
#include <EthernetENC.h>
//für Webserver notwendig
#include <ESPAsyncWebServer.h>



//---------------------------------------------------------------------
//Network functions
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
bool WIFIConnectionCheck(bool with_reconnect)
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
