#include "Klassen_HZS.h"
#define BGTDEBUG

TSensor::TSensor(OneWire * ds, byte * Address):
TempMin(-55.0),
TempMax(125.0),
StartMeasureTime(0),
NewValue(false)
{
    data = new byte[11];
    addr = new byte[8];
    SType = new char[10];
    Name = new char[15];
    Source = ds;
    strcpy(Name, "unnamed");
    data[0] = 0;
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
    data = new byte[11];
    addr = new byte[8];
    SType = new char[10];
    Name = new char[15];
    Source = ds;
    strcpy(Name, "unnamed");
    addr[0] = 0x00;
    data[0] = 0;
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
    NewValue = false;
    Source->reset();
    Source->select(addr);
    Source->write(0x44, 1);        // start conversion, with parasite power on at the end
    StartMeasureTime = millis();
}
bool TSensor::loop()
{
    if(StartMeasureTime==0)     //No Measurement started
        return false;
    data[10] = 0;
    byte sensConfig = 0x03;
    u_int16_t meassurementDelay;      
    if(data[0] != 0)
        sensConfig = (data[4] >> 5) & 0x03;     //Config on Bit 5 and Bit 6

    switch (sensConfig)
    {
    case 0x00:
        meassurementDelay = 94 + 50; //Datasheet 93,75ms + 50ms safety
        break;
    case 0x01:
        meassurementDelay = 188 + 50; //Datasheet 187,5ms + 50ms safety
        break;
    case 0x02:
        meassurementDelay = 375 + 50; //Datasheet 375ms + 50ms safety
        break;    
    default:
        meassurementDelay = 750 + 50; //Datasheet 750ms + 50ms safety
        break;
    }
    if(millis() < (StartMeasureTime + meassurementDelay)) //Delay time not expired
        return false;
    
    data[10] = Source->reset();
    Source->select(addr);    
    Source->write(0xBE);         // Read Scratchpad
    for (int i = 0; i < 9; i++) {           // we need 9 bytes
        data[i] = Source->read();
    }
    data[9] = OneWire::crc8(data, 8);
    #ifdef BGTDEBUG
        Serial.println("Data (Byte 8 and 9 = CRC-check): ");
        for (int i = 0; i < 11; i++) {           // we need 9 bytes
            Serial.print(data[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    #endif
    StartMeasureTime = 0;
    NewValue = true;
    return true;
}
float TSensor::getTempC()
{
    // Convert the data to actual temperature
    // because the result is a 16 bit signed integer, it should
    // be stored to an "int16_t" type, which is always 16 bits
    // even when compiled on a 32 bit processor.
    int16_t raw = (data[1] << 8) | data[0];
    if (addr[0] == 0x10) {
        raw = raw << 3; // 9 bit resolution default
        if (data[7] == 0x10) {
            // "count remain" gives full 12 bit resolution
            raw = (raw & 0xFFF0) + 12 - data[6];
        }
    } else {
        byte cfg = (data[4] & 0x60);
        // at lower res, the low bits are undefined, so let's zero them
        if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
        else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
        else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
        //// default is 12 bit resolution, 750 ms conversion time
    }
    #ifdef BGTDEBUG
        Serial.print("  Temperature = ");
        Serial.print(((float)raw / 16.0));
        Serial.print(" Celsius, ");
    #endif

    return (float)raw / 16.0;
}