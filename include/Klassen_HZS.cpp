#include "Klassen_HZS.h"

TSensor::TSensor(byte * Address)
{
    addr = new byte[8];
    SType = new char[10];
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