#include "Klassen_HZS.h"
#define BGTDEBUG true

TSensor::TSensor(OneWire * ds, byte * Address):
TempMin(-55.0),
TempMax(125.0),
StartMeasureTime(0),
NewValue(false)
{
    addr = new byte[8];
    SType = new char[10];
    Name = new char[15];
    Source = ds;
    strcpy(Name, "unnamed");
    for(int i = 0; i < 8; i++)
    {
        addr[i]= Address[i];
    }
      // the first ROM byte indicates which chip
    switch (addr[0]) {
        case 0x10:
            strcpy(SType, "DS18S20");  // or old DS1820
            break;
        case 0x28:
            strcpy(SType, "DS18B20");
            break;
        case 0x22:
            strcpy(SType, "DS1822");
            break;
        default:
            strcpy(SType, "Type NA");
            break;
    } 
}
TSensor::TSensor(OneWire * ds):
TempMin(-55.0),
TempMax(125.0),
StartMeasureTime(0),
NewValue(false)
{
    addr = new byte[8];
    SType = new char[10];
    Name = new char[15];
    Source = ds;
    strcpy(Name, "unnamed");
    addr[0] = 0x00;
}
TSensor::~TSensor()
{
    delete[] addr;
    delete[] SType;
    delete[] Name;
}
bool TSensor::SensSearch()
{
    if(addr[0]!= 0x00)
    {
        #ifdef BGTDEBUG
            Serial.println("Only new Object without Address could use for search!");
        #endif
        return false;
    }
    if(!Source->search(addr))
    {
        Source->reset_search();
        delay(250);
        #ifdef BGTDEBUG
            Serial.println("No more addresses.");
        #endif

        return false;
    }
    else
    {
        if (OneWire::crc8(addr, 7) != addr[7]) {
            #ifdef BGTDEBUG
                Serial.print("ROM =");
                for(int i = 0; i < 8; i++) {
                    Serial.write(' ');
                    Serial.print(addr[i], HEX);
                    Serial.println();
                }
                Serial.print("CRC is not valid!");
            #endif
            return false;
        }
    }
    return true;
}
void TSensor::startConversion()
{
    if(addr[0]==0)
    return;
    
    Source->reset();
    Source->select(addr);
    Source->write(0x44, 1);        // start conversion, with parasite power on at the end
    StartMeasureTime = millis();
}