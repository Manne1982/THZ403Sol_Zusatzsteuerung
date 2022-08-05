#include <Arduino.h>
#include <OneWire.h>
#include "Klassen_HZS.h"
#include <EthernetENC.h>
//MQTT
#include <PubSubClient.h>

//MQTT Variablen
EthernetClient client;
PubSubClient MQTTclient(client);

unsigned long Break_10s = 0;

bool MQTTinit();
void MQTT_callback(char* topic, byte* payload, unsigned int length);


void setup(void) {
  Serial.begin(9600);
  uint8_t mac[6] = {0xA0,0xA1,0xA2,0xA3,0xA4,0xA5};
  delay(5000);
  if(Ethernet.begin(mac)) //Configure IP address via DHCP
  {
    Serial.println(Ethernet.localIP());
    Serial.println(Ethernet.gatewayIP());
    Serial.println(Ethernet.subnetMask());
  }
  //MQTT
  MQTTinit();

}

void loop(void) {
  
  if(millis()>Break_10s)
  {
    Break_10s = millis() + 10000;
    Serial.println(Ethernet.localIP());
  }
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