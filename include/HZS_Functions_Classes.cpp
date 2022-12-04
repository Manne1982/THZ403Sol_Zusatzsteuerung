#include "HZS_Functions_Classes.h"
//#define BGTDEBUG


//Input Output Functions
TempSensor * FindTempSensor(TempSensor * TSArray, uint8 ArrayLen, uint64 Address)
{
  for(int i = 0; i < ArrayLen; i ++)
    if(TSArray[i].Address == Address)
      return &TSArray[i];

  return 0;
}
TempSensor * GetFirstEmptyTS(TempSensor * TSArray, uint8 ArrayLen)
{
  for(int i = 0; i < ArrayLen; i ++)
    if(TSArray[i].Address == 0)
      return &TSArray[i];

  return 0;
}
void DelTSensor(TempSensor * Sensor)
{
  Sensor->Address = 0;
  Sensor->Offset = 0;
  Sensor->Name[0] = 0;
  Sensor->SensorState = 0;
}
void DelTSensor(uint64 Address, TempSensor * TSArray, uint8 ArrayLen)
{
  for(int i = 0; i < ArrayLen; i ++)
  {
    if(TSArray[i].Address != Address)
      continue;

    TSArray[i].Address = 0;
    TSArray[i].Offset = 0;
    TSArray[i].Name[0] = 0;
    TSArray[i].SensorState = 0;
  }
}
void TakeoverTSConfig(TSensorArray * TSensArray, TempSensor * TSArray, uint8 ArrayLen)
{
  TempSensor * Temp = 0;
  for(int i = 0; i < TSensArray->GetSensorCount(); i ++)
  {
    Temp = FindTempSensor(TSArray, ArrayLen, TSensArray->GetSensorIndex(i)->getAddressUINT64());
    if(Temp)
    {
      TSensArray->GetSensorIndex(i)->setName(Temp->Name);
      TSensArray->GetSensorIndex(i)->setOffset(Temp->Offset);
    }
    else
    {
      Temp = GetFirstEmptyTS(TSArray, ArrayLen);
      if(Temp)
      {
        Temp->Address = TSensArray->GetSensorIndex(i)->getAddressUINT64();
        Temp->Offset = TSensArray->GetSensorIndex(i)->getOffset();
        strcpy(Temp->Name, TSensArray->GetSensorIndex(i)->getName().c_str());
      }
    }
  }
}
uint8 FindMissingSensors(TSensorArray * TSensArray1, TSensorArray * TSensArray2, TempSensor * *TSArrayMissing, TempSensor * TSArray, uint8 ArrayLen)
{
  uint8 CounterMissingSens = 0;
  for(int i = 0; i < ArrayLen; i ++)
  {
    if(TSArray[i].Address)
    {
      if(TSensArray1->GetSensorAddr(TSArray[i].Address))
        continue;
      if(TSensArray2->GetSensorAddr(TSArray[i].Address))
        continue;
      TSArrayMissing[CounterMissingSens]= &TSArray[i];
      CounterMissingSens++;
    }
  }
  return CounterMissingSens;
}
int MCPSetup(Adafruit_MCP23X17 * MCP, int MCPAddress)
{
  if (!MCP->begin_I2C(MCPAddress)) {
    return 2;
    #ifdef BGTDEBUG
      Serial.println("MCP 39 (Outputs) not connected!");
    #endif
  }
  else{
    return 1;
    #ifdef BGTDEBUG
      Serial.println("MCP 39 (Outputs) connected!");
    #endif
  }
  return 0;
}
void MCPinit(Adafruit_MCP23X17 * MCP, int * MCPStates)
{
  MCPStates[0] = MCPSetup(&MCP[0], MCPPort0);
  MCPStates[1] = MCPSetup(&MCP[1], MCPPort1);
  for(int i =0; i <16; i++)
  {
    MCP[0].pinMode(i, OUTPUT);
    MCP[1].pinMode(i, INPUT_PULLUP);
    MCP[1].setupInterruptPin(i, CHANGE);
  }
  MCP[0].writeGPIOAB(0xFFFF); //Set all outputs on High to put off the relaises
}

//---------------------------------------------------------------------
uint64 StrToLongInt(String Input)
{
  int Start = -1, Stop = Input.length()-1;
  uint64 Temp = 0, Multi = 1;
  for(unsigned int i = 0; i < Input.length(); i++)
  {
    bool Zahl = false;
    if((Input[i] >='0')&&(Input[i] <= '9'))
      Zahl = true;
    if(Start == -1)
    {
      if(Zahl)
        Start = i;
    }
    else
    {
      if(!Zahl)
      {
        Stop = i-1;
        break;
      }
    }
  }
  for(int i = Stop; i >= Start; i--)
  {
    Temp += (Input[i]-'0')*Multi;
    Multi *=10;
  }
  return Temp;
}
String IntToStr(int _var)
{
    char Temp[20];
    sprintf(Temp,"%d",_var);
    return Temp;
}
String IntToStr(char _var)
{
    char Temp[20];
    sprintf(Temp,"%d",_var);
    return Temp;
}
String IntToStr(long int _var)
{
    char Temp[20];
    sprintf(Temp,"%ld",_var);
    return Temp;
}
String IntToStr(uint32_t _var)
{
    char Temp[20];
    sprintf(Temp,"%u",_var);
    return Temp;
}
String IntToStr(float _var)
{
    char Temp[20];
    sprintf(Temp,"%f",_var);
    return Temp;
}
String IntToStrHex(int _var)
{
    char Temp[20];
    sprintf(Temp,"%x",_var);
    return Temp;
}
