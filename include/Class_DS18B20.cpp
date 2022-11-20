#include "Class_DS18B20.h"
//#define BGTDEBUG

TSensor::TSensor(OneWire * ds, byte * Address):
TempMin(-55.0),
TempMax(125.0),
Offset(0),
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
Offset(0),
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
    delete[] data;
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
    u_int16_t measurementDelay;      
    if(data[0] != 0)
        sensConfig = (data[4] >> 5) & 0x03;     //Config on Bit 5 and Bit 6

    switch (sensConfig)
    {
    case 0x00:
        measurementDelay = 94 + 50; //Datasheet 93,75ms + 50ms safety
        break;
    case 0x01:
        measurementDelay = 188 + 50; //Datasheet 187,5ms + 50ms safety
        break;
    case 0x02:
        measurementDelay = 375 + 50; //Datasheet 375ms + 50ms safety
        break;    
    default:
        measurementDelay = 750 + 50; //Datasheet 750ms + 50ms safety
        break;
    }
    if(millis() < (StartMeasureTime + measurementDelay)) //Delay time not expired
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
    NewValue = false;
    return (float)raw / 16.0;
}

void TSensor::setName(String NewName)
{
    if(NewName.length() < 15)
    {
        strcpy(Name, NewName.c_str());
    }
}
String TSensor::getName()
{
    return Name;
}
String TSensor::getAddressHEX()
{
/*    char Temp[20] = "";
    for(int i = 0; i < 8; i++)
    {
        sprintf(&Temp[i*2], "%02X", addr[i]);
    }
    Temp[16]=0;
    uint64 * Test = (uint64*) addr;
    Serial.println(*Test);
    return Temp;
*/
    return convertUINT64toHEXstr((uint64*)addr);
}
uint64 TSensor::getAddressUINT64()
{
    uint64 * Temp = (uint64*) addr;
    return *Temp;
}
bool TSensor::NewValueAvailable()
{
    return NewValue;
}
void TSensor::setOffset(float TempOffset)
{
    Offset = TempOffset;
}
float TSensor::getOffset()
{
    return Offset;
}

TSensorArray::TSensorArray(uint8 port)
{
    Port = port;
    SensorCount = 0;
    Source = new OneWire(Port);
    SensorSearch();
}

TSensorArray::~TSensorArray()
{
    if(SensorCount != 0)
    {
        for(int i = 0; i < SensorCount; i++)
        {
            delete SArray[i];
        }
        delete Source;
    }
    
}

uint8 TSensorArray::SensorSearch()
{
    if(SensorCount != 0)
    {
        for(int i = 0; i < SensorCount; i++)
        {
            delete SArray[i];
        }
    }
    for(int i = 0; i < 10; i++)
    {
      SArray[i] = new TSensor(Source);
      if(!SArray[i]->SensSearch())
      {
        delete SArray[i];
        SensorCount = i;
        return SensorCount;
      }
      char Temp[15];
      sprintf(Temp, "P%dS%d", Port, i);
      SArray[i]->setName(Temp);
    }
    return SensorCount;
}
uint8 TSensorArray::GetSensorCount()
{
    return SensorCount;
}
TSensor * TSensorArray::GetSensorIndex(uint8 Index)
{
    if(Index < SensorCount)
        return SArray[Index];
    else
        return 0;
}
TSensor * TSensorArray::GetSensorAddr(uint64 Addr)
{
    for(int i = 0; i < SensorCount; i++)
    {
        if(SArray[i]->getAddressUINT64() == Addr)
            return SArray[i];
    }
    return 0;
}
float TSensorArray::GetTempIndex(uint8 Index)
{
    if(Index < SensorCount)
        return SArray[Index]->getTempC();
    else
        return 999;
}
float TSensorArray::GetTempAddr(uint64 Addr)
{
    for(int i = 0; i < SensorCount; i++)
    {
        if(SArray[i]->getAddressUINT64() == Addr)
            return SArray[i]->getTempC();
    }
    return 999;
}

void TSensorArray::StartConversion()
{
    for(int i = 0; i < SensorCount; i++)
    {
        SArray[i]->startConversion();
    }
}
uint8 TSensorArray::Loop()
{
    CountNewValues = 0;
    for(int i = 0; i < SensorCount; i++)
    {
        if(SArray[i]->loop() || SArray[i]->NewValueAvailable())
            CountNewValues++;
    }
    return CountNewValues;
}

String convertUINT64toHEXstr(uint64 * input)
{
    char Temp[30] = "";
    byte *inputByte = (byte *) input;
    for(int i = 0; i < 8; i++)
    {
        sprintf(&Temp[i*3], "%02X ", inputByte[i]);
    }
    Temp[23]=0;
    return Temp;
}
