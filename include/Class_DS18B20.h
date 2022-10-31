#ifndef Class_DS18B20
#define Class_DS18B20

#include <Arduino.h>
#include <OneWire.h>

class TSensor
{
    public:
        TSensor(OneWire * ds, byte * Address);
        TSensor(OneWire * ds);
        ~TSensor();
        bool SensSearch();      //Search new sensors, only if TSensor initiated without address (return value true = OK; false = no device or failure)
        void startConversion(); //Measurement initiate 
        bool loop();            //Have to start periotly to find out if there are a new measured value available if new Value available --> true
        bool NewValueAvailable();   //If a new Value available that was not fetched --> true
        float getTempC();           //Get saved Temperature as degree Celcius
        void setName(String newName);
        String getName();
        String getAddressHEX();
        uint64 getAddressUINT64();
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

class TSensorArray
{
    public:
        TSensorArray(uint8 port);
        ~TSensorArray();
        uint8 GetSensorCount();
        uint8 SensorSearch(); //Delete old List and create a new list with connected Sensors
        void StartConversion(); //Meassurement initiate for all Sensors
        uint8 Loop();
        TSensor * GetSensorIndex(uint8 Index);
        TSensor * GetSensorAddr(uint64 Addr);
        float GetTempIndex(uint8 Index);
        float GetTempAddr(uint64 Addr);
    private:
        uint8 Port;
        uint8 SensorCount;
        uint8 CountNewValues;
        TSensor * SArray[10];
        OneWire * Source;
};

String convertUINT64toHEXstr(uint64 * input); //Function needed in TSensor class


#include "Class_DS18B20.cpp"

#endif
