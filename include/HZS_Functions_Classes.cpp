#include "HZS_Functions_Classes.h"
//#define BGTDEBUG

//Class functions für AirQualitySensor

AirQualitySensor::AirQualitySensor(uint8 _PortOnOff, uint8 _PortAnalogInput):
SensorState(0),
OnTime_ms(0),
HeatingTime_ms(6000), //10 Minutes
AirSensInitiated(0)
{
  PortOnOff = _PortOnOff;
  PortAnalogInput = _PortAnalogInput;
  pinMode(PortOnOff, OUTPUT);
  digitalWrite(PortOnOff, 0);
  AirSens= new MQ2(_PortAnalogInput);
}
AirQualitySensor::~AirQualitySensor()
{
  digitalWrite(PortOnOff, 0);
  pinMode(PortOnOff, INPUT);
  delete AirSens;
}
byte AirQualitySensor::getSensorState()
{
  return SensorState;
}
void AirQualitySensor::setSensorState(byte newState)
{
  if(newState > 1)
  return;
  if(newState)
    OnTime_ms = millis();
  else
  {
    OnTime_ms = 0;
    AirSensInitiated = 0;
  }
  digitalWrite(PortOnOff, newState);
  SensorState = newState;
}
byte AirQualitySensor::toggleSensorState()
{
  setSensorState((SensorState + 1)%2);
  return SensorState;
}
uint32 AirQualitySensor::getOnTime_s()
{
  if(SensorState)
    return ((millis() - OnTime_ms)/1000);
  else
    return 0;
}
float AirQualitySensor::readLPG()
{
  if(!SensorState || ((millis()-OnTime_ms) < HeatingTime_ms))
    return -1;
  if(!AirSensInitiated)
  {
    AirSens->begin();
    AirSensInitiated = 1;
    return 0;
  }
  else
    return AirSens->readLPG();
  return 0;
}
float AirQualitySensor::readCO()
{
  if(!SensorState || ((millis()-OnTime_ms) < HeatingTime_ms))
    return -1;
  if(!AirSensInitiated)
  {
    AirSens->begin();
    AirSensInitiated = 1;
    return 0;
  }
  else
    return AirSens->readCO();
  return 0;
}
float AirQualitySensor::readSMOKE()
{
  if(!SensorState || ((millis()-OnTime_ms) < HeatingTime_ms))
    return -1;
  if(!AirSensInitiated)
  {
    AirSens->begin();
    AirSensInitiated = 1;
    return 0;
  }
  else
    return AirSens->readSmoke();
  return 0;
}
uint16 AirQualitySensor::getHWValue()
{
  return analogRead(PortAnalogInput);
}

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
          MCP[MCPOutput].pinMode(i+8, OUTPUT); //Switch SSRelais ON in Manu-Mode
          Manu_ONOFF &= ~(1<<i); //No break to switch into Manu Mode
        case 0:
          MCP[MCPOutput].digitalWrite(i, 0);  //Switch Relais for Manu-Mode
          Manu_Auto &= ~(1<<i);
          break;
        case 3: //Switch to Manu-Mode fist if any Input messured to save energy
          *AutoOverSSRelais |= (1<<i);
        case 2:
          MCP[MCPOutput].pinMode(i+8, INPUT);  //Switch SSRelais of 
          MCP[MCPOutput].digitalWrite(i, 1);    //Switch Relais to Auto-Mode
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
    return OutputConfig;
  }
  return 0xFFFF; //Return all ports Auto
}
void SetOutput(int OutputIndex, int Value, digital_Output_current_Values * _Output, Adafruit_MCP23X17 * _MCP, int * MCPStates)
{
  if(MCPStates[MCPOutput] != 1)
    return;
  if((Value < 0) || (Value > 4))
    return;
  switch(Value)
  {
    case 1:
      setRelaisManuAuto(_MCP, OutputIndex, 1, MCPStates);
      setSSR(_MCP, OutputIndex, 1, MCPStates);
      _Output->Outputstates &= (uint16) ~(((uint16) 1<<OutputIndex)+((uint16) 1<<(OutputIndex+8)));
      _Output->PWM_Manu_activ &= ~(1<<OutputIndex);
      break;
    case 0:
      setRelaisManuAuto(_MCP, OutputIndex, 1, MCPStates);
      setSSR(_MCP, OutputIndex, 0, MCPStates);
      _Output->Outputstates &= (uint16) ~(((uint16) 1<<OutputIndex));
      _Output->Outputstates |= (uint16)((uint16)1<<(OutputIndex+8));
      _Output->PWM_Manu_activ &= ~(1<<OutputIndex);
      break;
    case 2:
    case 3:
      setRelaisManuAuto(_MCP, OutputIndex, 0, MCPStates);
      setSSR(_MCP, OutputIndex, 0, MCPStates);
      _Output->Outputstates |= (uint16)(((uint16)1<<OutputIndex)+((uint16)1<<(OutputIndex+8)));
      _Output->PWM_Manu_activ &= ~(1<<OutputIndex);
      break;
    case 4: //Manuel PWM mode, only configurable over MQTT
      setRelaisManuAuto(_MCP, OutputIndex, 1, MCPStates);
      setSSR(_MCP, OutputIndex, 0, MCPStates);
      _Output->Outputstates &= (uint16) ~(((uint16) 1<<OutputIndex));
      _Output->Outputstates |= (uint16)((uint16)1<<(OutputIndex+8));
      _Output->PWM_Manu_activ |= (1<<OutputIndex);
      break;
    default:
      #ifdef BGTDEBUG
        Serial.println("Wrong StartValue for Output Config");
      #endif
      break;
  }

}
bool readDigitalInputs_SetOutputIfAutoSSRMode(int Interrupt, digital_Input * _Inputs, Adafruit_MCP23X17 * _MCP, digital_Output_current_Values* _Output, int * MCPStates)
{
  bool anyChange = false;
  uint8 InputOldValue = _Inputs->StatesHW;
  if((MCPStates[MCPOutput] != 1)||(MCPStates[MCPInput] != 1))
    return false;

  if(!Interrupt) //is there any change of digital inputs
  {
    unsigned long currentTime = millis();
    _Inputs->StatesHW = _MCP[MCPInput].readGPIOA();
    for (int i = 0; i < 8; i++)
    {
      switch(_Inputs->ReadStep[i])
      {
        case 4: //Input was off for more than 10 s
        case 0: //Meassurement not started and start with hight edge (Input inverted)
          if(!(_Inputs->StatesHW & (1<<i)))
          {
            _Inputs->TimeStartpoints[i][0] = currentTime;
            _Inputs->ReadStep[i] = 1;
            _Inputs->OnTimeRatio[i] = 255;
          }
          break;
        case 1: //Meassurement started low time and start with falling edge
          if(_Inputs->StatesHW & (1<<i))
          {
            if((_Inputs->OnTimeRatio[i] == 255)&&(_Inputs->TimeStartpoints[i][0]==0))
              _Inputs->OnTimeRatio[i] = 0;
            _Inputs->TimeStartpoints[i][1] = currentTime;
            _Inputs->ReadStep[i]++;
          }
          break;
        case 2: //cycle finished and write the Ratio value
          if(!(_Inputs->StatesHW & (1<<i)))
          {
                                     //  255 * on time / cycle time
            _Inputs->OnTimeRatio[i] = 255 * (_Inputs->TimeStartpoints[i][1] - _Inputs->TimeStartpoints[i][0]) / (currentTime - _Inputs->TimeStartpoints[i][0]);
            _Inputs->TimeStartpoints[i][0] = currentTime;
            _Inputs->TimeStartpoints[i][1] = 0;
            _Inputs->ReadStep[i] = 1;
          }
          break;
        case 3: //Input was on for more than 10 s
          if(_Inputs->StatesHW & (1<<i))
          {
            _Inputs->OnTimeRatio[i] = 0;
            _Inputs->ReadStep[i]= 0;
          }
          break;
        default:
          break;
      }
      if(_Inputs->OnTimeRatio[i] && (_Output->OutputstatesAutoSSRelais & (1<<i))&&((InputOldValue&(1<<i))!=(_Inputs->StatesHW&(1<<i))))
      {
        SetOutput(i, ((~_Inputs->StatesHW & (1<<i))/(1<<i)), _Output, _MCP, MCPStates);
      }
    }
    anyChange = true;
  }
  else
  {
    unsigned long currentTime = millis();
    for (int i = 0; i < 8; i++)
    {
      if((_Inputs->ReadStep[i] == 3) || (_Inputs->ReadStep[i] == 4))
        continue;
      if(((currentTime - _Inputs->TimeStartpoints[i][0])>10000) && !(_Inputs->StatesHW & (1<<i)))
      {
        _Inputs->OnTimeRatio[i] = 255;
        _Inputs->ReadStep[i] = 3;
        _Inputs->TimeStartpoints[i][0] = 0;
        _Inputs->TimeStartpoints[i][1] = 0;
        anyChange = true;
      }
      if(((currentTime - _Inputs->TimeStartpoints[i][1])>10000) && (_Inputs->StatesHW & (1<<i)))
      {
        _Inputs->OnTimeRatio[i] = 0;
        _Inputs->ReadStep[i] = 4;
        _Inputs->TimeStartpoints[i][0] = 0;
        _Inputs->TimeStartpoints[i][1] = 0;
        anyChange = true;
      }
      if((_Inputs->OnTimeRatio[i]==0)&&(_Output->OutputstatesAutoSSRelais & (1<<i))&&((~_Output->Outputstates) & (uint16)(1<<i))) //if Input ratio on 0 switch off the relais for Manu mode to save energy
      {
        SetOutput(i, 3, _Output, _MCP, MCPStates);
      }
    }
  }
  return anyChange;
}
void setSSR(Adafruit_MCP23X17 * _MCP, uint8 OutputIndex, uint8 On_Off, int * MCPStates)
{
  if((MCPStates[MCPOutput] == 2)||(MCPStates[MCPInput] == 2))
    return;
  if(OutputIndex>7)
    return;
  if(On_Off)
    _MCP[MCPOutput].pinMode(OutputIndex + 8, OUTPUT); //Switch on the SSR (Solid State Relais)
  else
    _MCP[MCPOutput].pinMode(OutputIndex + 8, OUTPUT); //Switch on the SSR (Solid State Relais)
}
void setRelaisManuAuto(Adafruit_MCP23X17 * _MCP, uint8 OutputIndex, uint8 Manu, int * MCPStates)
{
  if(MCPStates[MCPOutput] != 1)
    return;
  if(OutputIndex>7)
    return;
  if(Manu)
    _MCP[MCPOutput].digitalWrite(OutputIndex, 0);
  else
    _MCP[MCPOutput].digitalWrite(OutputIndex, 1);
}
void updateOutputPWM(Adafruit_MCP23X17 * _MCP, digital_Output_current_Values* _Output, int * MCPStates)
{
  if(MCPStates[MCPOutput] != 1)
    return;
  for(int i = 0; i<8; i++)
  {
    if(_Output->PWM_Manu_activ & (1<<i))
    {
      if((millis()%_Output->PWM_CycleTime_ms)<((_Output->PWM_CycleTime_ms*_Output->PWM_Value[i]/255)))
      {
        if(!(_Output->PWM_CurrentState & (1<<i)))
        {
          setSSR(_MCP, i, 1, MCPStates);
          _Output->PWM_CurrentState |= (1<<i);
        }
      }
      else
      {
        if(_Output->PWM_CurrentState & (1<<i))
        {
          setSSR(_MCP, i, 0, MCPStates);
          _Output->PWM_CurrentState &= ~(1<<i);
        }
      }
    }
  }
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
