#include "HZS_Functions_Classes.h"
//#define BGTDEBUG

//Class functions für AirQualitySensor

AirQualitySensor::AirQualitySensor(uint8 _PortOnOff, uint8 _PortAnalogInput):
SensorState(0),
OnTime_ms(0),
HeatingTime_ms(180000), //3 Minutes
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
  if((SensorState == 1)&&(millis()< (OnTime_ms + HeatingTime_ms)))
    return 2;
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
//Three Way Valve Class functions

ThreeWayValve::ThreeWayValve(uint8 _ChannelOpen, uint8 _ChannelClose):
CycleTimeOpen_s(150),
CycleTimeClose_s(150),
ValvePosition(5000),
OnTimeOpen_ms(0),
OnTimeClose_ms(0)
{
  ChannelOpen = (_ChannelOpen < 8)?_ChannelOpen:0;
  ChannelClose = (_ChannelClose < 8)?_ChannelClose:0;
}
ThreeWayValve::~ThreeWayValve()
{}
void ThreeWayValve::setChannelOpen(uint8 _Channel)
{
  ChannelOpen = (_Channel < 8)?_Channel:0;
}
void ThreeWayValve::setChannelClose(uint8 _Channel)
{
  ChannelClose = (_Channel < 8)?_Channel:0;
}
void ThreeWayValve::setCycleTimeOpen(uint16 _Seconds)
{
  CycleTimeOpen_s = _Seconds;
}
void ThreeWayValve::setCycleTimeClose(uint16 _Seconds)
{
  CycleTimeClose_s = _Seconds;
}
uint16 ThreeWayValve::getValvePosition()
{
  uint16 Temp = 0;
  if(OnTimeOpen_ms)
  {
    Temp = ValvePosition + 10000L * (millis() - OnTimeOpen_ms) / (CycleTimeOpen_s * 1000);
    if(Temp > 10000)
      return 10000;
    else 
      return Temp;
  }
  else if(OnTimeClose_ms)
  {
    Temp = ValvePosition - 10000L * (millis() - OnTimeClose_ms) / (CycleTimeClose_s * 1000);
    if(Temp > 10000)
      return 0;
    else
      return Temp;
  }

  return ValvePosition;
}
//Zu alleine Auf beide Kanäle
void ThreeWayValve::updateState(digital_Output_current_Values * _Outputs)
{
  if(!(_Outputs->realValue&((1 << ChannelOpen)|(1 << ChannelClose))) && !(OnTimeClose_ms + OnTimeOpen_ms)) {
    return;}
  
  if((_Outputs->realValue&(1<<ChannelOpen)) && ((OnTimeOpen_ms + OnTimeClose_ms)==0))
  {
    OnTimeOpen_ms = millis();
  }
  else if((_Outputs->realValue&(1<<ChannelClose)) && ((OnTimeOpen_ms + OnTimeClose_ms)==0))
  {
    OnTimeClose_ms = millis();
  }
  else if(OnTimeOpen_ms)
  {
    ValvePosition += 10000L * (millis() - OnTimeOpen_ms) / (CycleTimeOpen_s * 1000);
    if(ValvePosition > 10000)
      ValvePosition = 10000;
    OnTimeOpen_ms = 0;
  }
  else if(OnTimeClose_ms)
  {
    ValvePosition -= 10000L * (millis() - OnTimeClose_ms) / (CycleTimeClose_s * 1000);
    if(ValvePosition > 10000)
      ValvePosition = 0;
    OnTimeClose_ms = 0;
  }
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
  MCP[MCPOutput].writeGPIOAB(0x00FF); //Set all outputs on High to put off the relaises
  for(int i =0; i <16; i++)
  {
    if(i<8)
      MCP[MCPOutput].pinMode(i, OUTPUT);
    else
      MCP[MCPOutput].pinMode(i, INPUT); //Use external Pullup

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
void InitOutputStates(Adafruit_MCP23X17 * MCP, digital_Output * Config, int * MCPStates, digital_Output_current_Values * _Output_Values)
{
  if(MCPStates[MCPOutput] == 1)
  {
    for(int i = 0; i < 8; i++)
    {
      SetOutput(i, Config[i].StartValue, _Output_Values, MCP, MCPStates);
    }
  }
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
      _Output->OutputstatesAutoSSRelais &= ~(1<<OutputIndex);
      break;
    case 0:
      setRelaisManuAuto(_MCP, OutputIndex, 1, MCPStates);
      setSSR(_MCP, OutputIndex, 0, MCPStates);
      _Output->Outputstates &= (uint16) ~(((uint16) 1<<OutputIndex));
      _Output->Outputstates |= (uint16)((uint16)1<<(OutputIndex+8));
      _Output->PWM_Manu_activ &= ~(1<<OutputIndex);
      _Output->OutputstatesAutoSSRelais &= ~(1<<OutputIndex);
      break;
    case 2:
      setRelaisManuAuto(_MCP, OutputIndex, 0, MCPStates);
      setSSR(_MCP, OutputIndex, 0, MCPStates);
      _Output->Outputstates |= (uint16)(((uint16)1<<OutputIndex)+((uint16)1<<(OutputIndex+8)));
      _Output->PWM_Manu_activ &= ~(1<<OutputIndex);
      _Output->OutputstatesAutoSSRelais &= ~(1<<OutputIndex);
      break;
    case 3:
      setRelaisManuAuto(_MCP, OutputIndex, 0, MCPStates);
      setSSR(_MCP, OutputIndex, 0, MCPStates);
      _Output->Outputstates |= (uint16)(((uint16)1<<OutputIndex)+((uint16)1<<(OutputIndex+8)));
      _Output->PWM_Manu_activ &= ~(1<<OutputIndex);
      _Output->OutputstatesAutoSSRelais |= (1<<OutputIndex);
      break;
    case 4: //Manuel PWM mode, only configurable over MQTT
      setRelaisManuAuto(_MCP, OutputIndex, 1, MCPStates);
      setSSR(_MCP, OutputIndex, 0, MCPStates);
      _Output->Outputstates &= (uint16) ~(((uint16) 1<<OutputIndex));
      _Output->Outputstates |= (uint16)((uint16)1<<(OutputIndex+8));
      _Output->PWM_Manu_activ |= (1<<OutputIndex);
      _Output->OutputstatesAutoSSRelais &= ~(1<<OutputIndex);
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

  unsigned long currentTime = millis();
  if(!Interrupt) //is there any change of digital inputs
  {
    _Inputs->StatesHW = _MCP[MCPInput].readGPIOA();
    _Inputs->lastReading = millis();
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
      if(_Inputs->OnTimeRatio[i] && (_Output->OutputstatesAutoSSRelais & (1<<(7-i)))&&((InputOldValue&(1<<i))!=(_Inputs->StatesHW&(1<<i))))
      {
        setRelaisManuAuto(_MCP, 7-i, 1, MCPStates);
        setSSR(_MCP, 7-i, ((~_Inputs->StatesHW & (1<<i))/(1<<i)), MCPStates);
      }
    }
    anyChange = true;
  }
  else
  {
    if((_Inputs->lastReading + 10000) < currentTime)
    {
      _Inputs->StatesHW = _MCP[MCPInput].readGPIOA();
      _Inputs->lastReading = millis();
      anyChange = InputOldValue != _Inputs->StatesHW;
    }
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
      if((_Inputs->OnTimeRatio[i]==0)&&(_Output->OutputstatesAutoSSRelais & (1<<(7-i))) && anyChange) //if Input ratio on 0 switch off the relais for Manu mode to save energy
      {
        SetOutput((7-i), 3, _Output, _MCP, MCPStates);
      }
    }
  }
  return anyChange;
}
void setSSR(Adafruit_MCP23X17 * _MCP, uint8 OutputIndex, uint8 On_Off, int * MCPStates)
{
  if(MCPStates[MCPOutput] != 1)
    return;
  if(OutputIndex>7)
    return;
  if(On_Off)
    _MCP[MCPOutput].pinMode(OutputIndex + 8, OUTPUT); //Switch on the SSR (Solid State Relais)
  else
    _MCP[MCPOutput].pinMode(OutputIndex + 8, INPUT); //Switch off the SSR (Solid State Relais)
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
uint8 getRealOutput(digital_Output_current_Values * _Output_Values, digital_Input * _Inputs)
{ 
  uint8 returnValue = 0;
  for(int i = 0; i < 8; i++)
  {
    if(~_Output_Values->Outputstates & (1<<i))
    {
      if((~_Output_Values->Outputstates & (1<<(i+8)))||(_Output_Values->PWM_CurrentState & (1<<i)))
      {
        returnValue |= (1<<i);
      }
    }
    else
    {
      if(~_Inputs->StatesHW & (1<<(7-i)))
      {
        returnValue |= (1<<i);
      }
    }
  }
  _Output_Values->realValue = returnValue;
  return returnValue;
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
