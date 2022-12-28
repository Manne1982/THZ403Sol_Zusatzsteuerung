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
uint8 MCPinit(Adafruit_MCP23X17 * MCP, int * MCPStates)
{
  MCPStates[MCPOutput] = MCPSetup(&MCP[MCPOutput], MCPPort0);
  MCPStates[MCPInput] = MCPSetup(&MCP[MCPInput], MCPPort1);
  MCP[MCPOutput].writeGPIOAB(0xFFFF); //Set all outputs on High to put off the relaises
  for(int i =0; i <16; i++)
  {
    MCP[MCPOutput].pinMode(i, OUTPUT);
    MCP[MCPInput].pinMode(i, INPUT_PULLUP);
    MCP[MCPInput].setupInterruptPin(i, CHANGE);
  }
  pinMode(INTPortA, INPUT); //Interrupt PIN for INTA MCP 1
  pinMode(INTPortB, INPUT); //Interrupt PIN for INTB MCP 1
  if(MCPStates[MCPInput]==1)
    return MCP[MCPInput].readGPIOA();
  else
    return 0;
}
uint16 InitOutputStates(Adafruit_MCP23X17 * MCP, digital_Output * Config, int * MCPStates, uint16 * AutoOverSSRelais)
{
  uint8 Manu_Auto = 0xFF; //all outputs Auto = 0xFF; all outputs Manu = 0x00
  uint8 Manu_ONOFF = 0xFF;  //all outputs Off (Manu) = 0xFF; all outputs On (Manu) = 0x00
  uint16 OutputConfig = 0;
  if(MCPStates[MCPOutput] == 1)
  {
    for(int i = 0; i < 8; i++)
    {
      switch(Config[i].StartValue)
      {
        case 1:
          Manu_ONOFF &= ~(1<<i); //No break to switch into Manu Mode
        case 0:
          Manu_Auto &= ~(1<<i);
          break;
        case 3: //Switch to Manu-Mode fist if any Input messured to save anergy
          *AutoOverSSRelais |= (1<<i);
        case 2:
          break;        
        default:
          #ifdef BGTDEBUG
            Serial.println("Wrong StartValue for Output Config");
          #endif
          break;
      }
    }
    OutputConfig = Manu_ONOFF << 8;
    OutputConfig |= Manu_Auto;
    MCP[MCPOutput].writeGPIOAB(OutputConfig);
    return OutputConfig;
  }
  return 0xFFFF; //Return all ports Auto
}
void SetOutput(int OutputIndex, int Value, uint16 * _OutputStates, Adafruit_MCP23X17 * MCP)
{
  if((Value < 0) || (Value > 3))
    return;
  switch(Value)
  {
    case 1:
      MCP->digitalWrite(OutputIndex, 0);  //Switch to Manu mode
      MCP->digitalWrite(OutputIndex + 8, 0); //Switch on the SSR (Solid State Relais) for Manu On
      *_OutputStates &= (uint16) ~(((uint16) 1<<OutputIndex)+((uint16) 1<<(OutputIndex+8)));
      break;
    case 0:
      MCP->digitalWrite(OutputIndex, 0); //Switch to Manu mode
      MCP->digitalWrite(OutputIndex + 8, 1); //Switch off the SSR for Manu Off
      *_OutputStates &= (uint16) ~(((uint16) 1<<OutputIndex));
      *_OutputStates |= (uint16)((uint16)1<<(OutputIndex+8));
      break;
    case 2:
    case 3:
      MCP->digitalWrite(OutputIndex, 1); //Switch to Auto mode
      MCP->digitalWrite(OutputIndex + 8, 1); //Switch off the SSR for Manu Off
      *_OutputStates |= (uint16)(((uint16)1<<OutputIndex)+((uint16)1<<(OutputIndex+8)));
      break;
    default:
      #ifdef BGTDEBUG
        Serial.println("Wrong StartValue for Output Config");
      #endif
      break;
  }

}
bool readDigitalInputs_SetOutputIfAutoSSRMode(int Interrupt, digital_Input * Inputs, Adafruit_MCP23X17 * MCP, uint8 AutoSSRMode, uint16 * _OutputStates)
{
  bool anyChange = false;
  uint8 InputOldValue = Inputs->StatesHW;
  if(!Interrupt)
  {
    unsigned long currentTime = millis();
    Inputs->StatesHW = MCP[MCPInput].readGPIOA();
    for (int i = 0; i < 8; i++)
    {
      switch(Inputs->ReadStep[i])
      {
        case 4: //Input was off for more than 10 s
        case 0: //Meassurement not started and start with hight edge (Input inverted)
          if(!(Inputs->StatesHW & (1<<i)))
          {
            Inputs->TimeStartpoints[i][0] = currentTime;
            Inputs->ReadStep[i] = 1;
            Inputs->OnTimeRatio[i] = 255;
          }
          break;
        case 1: //Meassurement started low time and start with falling edge
          if(Inputs->StatesHW & (1<<i))
          {
            if((Inputs->OnTimeRatio[i] == 255)&&(Inputs->TimeStartpoints[i][0]==0))
              Inputs->OnTimeRatio[i] = 0;
            Inputs->TimeStartpoints[i][1] = currentTime;
            Inputs->ReadStep[i]++;
          }
          break;
        case 2: //cycle finished and write the Ratio value
          if(!(Inputs->StatesHW & (1<<i)))
          {
                                     //  255 * on time / cycle time
            Inputs->OnTimeRatio[i] = 255 * (Inputs->TimeStartpoints[i][1] - Inputs->TimeStartpoints[i][0]) / (currentTime - Inputs->TimeStartpoints[i][0]);
            Inputs->TimeStartpoints[i][0] = currentTime;
            Inputs->TimeStartpoints[i][1] = 0;
            Inputs->ReadStep[i] = 1;
          }
          break;
        case 3: //Input was on for more than 10 s
          if(Inputs->StatesHW & (1<<i))
          {
            Inputs->OnTimeRatio[i] = 0;
            Inputs->ReadStep[i]= 0;
          }
          break;
        default:
          break;
      }
      if(Inputs->OnTimeRatio[i] && (AutoSSRMode & (1<<i))&&((InputOldValue&(1<<i))!=(Inputs->StatesHW&(1<<i))))
      {
        SetOutput(i, ((~Inputs->StatesHW & (1<<i))/(1<<i)), _OutputStates, &MCP[MCPOutput]);
      }
    }
    anyChange = true;
  }
  else
  {
    unsigned long currentTime = millis();
    for (int i = 0; i < 8; i++)
    {
      if((Inputs->ReadStep[i] == 3) || (Inputs->ReadStep[i] == 4))
        continue;
      if(((currentTime - Inputs->TimeStartpoints[i][0])>10000) && !(Inputs->StatesHW & (1<<i)))
      {
        Inputs->OnTimeRatio[i] = 255;
        Inputs->ReadStep[i] = 3;
        Inputs->TimeStartpoints[i][0] = 0;
        Inputs->TimeStartpoints[i][1] = 0;
        anyChange = true;
      }
      if(((currentTime - Inputs->TimeStartpoints[i][1])>10000) && (Inputs->StatesHW & (1<<i)))
      {
        Inputs->OnTimeRatio[i] = 0;
        Inputs->ReadStep[i] = 4;
        Inputs->TimeStartpoints[i][0] = 0;
        Inputs->TimeStartpoints[i][1] = 0;
        anyChange = true;
      }
      if((Inputs->OnTimeRatio[i]==0)&&(AutoSSRMode & (1<<i))&&((~*_OutputStates) & (uint16)(1<<i))) //if Input ratio on 0 switch off the relais for Manu mode to save energy
      {
        TestVar++;
        SetOutput(i, 3, _OutputStates, &MCP[MCPOutput]);
      }
    }
  }
  return anyChange;
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
String IntToStr(uint32 _var)
{
    char Temp[20];
    sprintf(Temp,"%u",_var);
    return Temp;
}
String IntToStr(uint8 _var)
{
    char Temp[20];
    sprintf(Temp,"%hhu",_var);
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
