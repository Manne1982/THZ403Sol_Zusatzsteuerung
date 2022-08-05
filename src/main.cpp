#include <Arduino.h>
#include <OneWire.h>
#include "Klassen_HZS.h"
#include <EthernetENC.h>

EthernetClient client;

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
}

void loop(void) {
  Serial.println(Ethernet.localIP());
  delay(5000);
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