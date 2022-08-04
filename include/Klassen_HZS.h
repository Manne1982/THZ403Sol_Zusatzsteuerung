#ifndef Klassen_HZS
#define Klassen_HZS

#include <Arduino.h>
#include <OneWire.h>

class TSensor
{
    public:
        TSensor(OneWire * ds, byte * Address);
        TSensor(OneWire * ds);
        bool SensSearch();
        void startConversion();
        ~TSensor();
    private:
        byte * data;
        byte * addr;
        char * Name;
        char * SType;   //Addr[0] --> 0x10 = DS18S20; 0x28 = DS18B20; 0x22 = DS1822
        float TempMin;
        float TempMax;
        unsigned long StartMeasureTime;
        bool NewValue;
        OneWire * Source;
};



#include "Klassen_HZS.cpp"

#endif
