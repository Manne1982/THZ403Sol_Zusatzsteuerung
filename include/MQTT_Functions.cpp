#include "MQTT_Functions.h"
#include "GlobalVariabels.h"
#include "ProjectFunctions.h"
#include <NTPClient.h>


//MQTT Functions
bool MQTTinit()
{
  if((varConfig.NW_Flags & NW_MQTTActive)==0)
    return false;
  if(millis() < (lastMQTTInit+60000))
    return false;
  else
    lastMQTTInit = millis();

  if(MQTTclient->connected())
    MQTTclient->disconnect();
  IPAddress IPTemp;
  IPTemp.fromString(varConfig.MQTT_Server);
  MQTTclient->setServer(IPTemp, varConfig.MQTT_Port);  
  MQTTclient->setCallback(MQTT_callback);
  unsigned long int StartTime = millis();
  while ((millis() < (StartTime + 5000)&&(!MQTTclient->connect((varConfig.NW_Flags & NW_EthernetActive)?EthernetMAC:WiFi.macAddress().c_str() , varConfig.MQTT_Username, varConfig.MQTT_Password)))){
    delay(200);
  }
  if(MQTTclient->connected()){
    #ifdef BGTDEBUG
      Serial.println("MQTTclient connected");
    #endif
    String SubscribeRootTemp_setOutput = varConfig.MQTT_rootpath;
    String SubscribeRootTemp_setPWMVal = varConfig.MQTT_rootpath;
    String SubscribeTemp = "";
    String SendPathTemp;
    SubscribeRootTemp_setOutput += MQTTSubscribeRoot[0];
    SubscribeRootTemp_setPWMVal += MQTTSubscribeRoot[2];
    for(int i = 0; i < 8; i++)
    {
      SubscribeTemp = SubscribeRootTemp_setOutput + OutputsBasicSettings[i].Name;
      MQTTclient->subscribe(SubscribeTemp.c_str());
      SubscribeTemp = SubscribeRootTemp_setPWMVal + OutputsBasicSettings[i].Name;
      MQTTclient->subscribe(SubscribeTemp.c_str());
      SendPathTemp = MQTTSendRoot[2];
      SendPathTemp += OutputsBasicSettings[i].Name;
      MQTT_sendMessage(SendPathTemp.c_str(), Output_Values.PWM_Value[i]);
    }
    SubscribeRootTemp_setOutput = varConfig.MQTT_rootpath;
    SubscribeRootTemp_setOutput += MQTTSubscribeRoot[1];
    SubscribeTemp = SubscribeRootTemp_setOutput + "WLAN_active";
    MQTTclient->subscribe(SubscribeTemp.c_str());
    SubscribeTemp = SubscribeRootTemp_setOutput + "AirSens_active";
    MQTTclient->subscribe(SubscribeTemp.c_str());
    SubscribeTemp = SubscribeRootTemp_setOutput + "ESP_Restart";
    MQTTclient->subscribe(SubscribeTemp.c_str());
    return true;
  }
  else
  {
    #ifdef BGTDEBUG
      Serial.println("MQTTclient connection failure");
      Serial.print("MAC-Adresse: ");
      Serial.println((varConfig.NW_Flags & NW_EthernetActive)?EthernetMAC:WiFi.macAddress().c_str());
      Serial.print("Benutzername: ");
      Serial.println(varConfig.MQTT_Username);
      Serial.print("Passwort: ");
      Serial.println(varConfig.MQTT_Password);
      Serial.print("Port: ");
      Serial.println(varConfig.MQTT_Port);
      Serial.print("Server: ");
      Serial.println(varConfig.MQTT_Server);
    #endif
    
    return false;
  }
}
void MQTT_callback(char* topic, byte* payload, unsigned int length)
{
  const String TempTopic = topic;
  String Value = (char*) payload;
  Value = Value.substring(0, length);
  uint8 TempPort = 0;
  
  if(TempTopic.substring(strlen(varConfig.MQTT_rootpath), (strlen(varConfig.MQTT_rootpath) + MQTTSubscribeRoot[0].length()))==MQTTSubscribeRoot[0])
  {
    int OutputIndex = FindOutputName(&topic[strlen(varConfig.MQTT_rootpath) + MQTTSubscribeRoot[0].length()]);
    if(OutputIndex < 0)
    { 
      MQTT_SendFailureText("Ausgang nicht gefunden: " + TempTopic.substring(strlen(varConfig.MQTT_rootpath) + MQTTSubscribeRoot[2].length()));
      return;
    }
    SetOutput(OutputIndex, Value.toInt(), &Output_Values, mcp, MCPState);
    TempPort = mcp[MCPOutput].readGPIOA();
    if(TempPort!=(Output_Values.Outputstates&0x00FF))
    {
      String TempText = topic;
      TempText += " -> Differenz Ausgang Port A soll zu ist erkannt: Soll ";
      TempText += IntToStr(Output_Values.Outputstates&0x00FF);
      TempText += ", Ist ";
      TempText += IntToStr(TempPort);
      MQTT_SendFailureText(TempText);
      MCPinit(mcp, MCPState);
      DO_new_Init(mcp, &Output_Values, MCPState);
      //mcp[MCPOutput].writeGPIOAB(Output_Values.Outputstates);
    }
  }
  else if(TempTopic.substring(strlen(varConfig.MQTT_rootpath), (strlen(varConfig.MQTT_rootpath) + MQTTSubscribeRoot[1].length()))==MQTTSubscribeRoot[1])
  {
    if(TempTopic.substring(strlen(varConfig.MQTT_rootpath) + MQTTSubscribeRoot[1].length())=="WLAN_active")
    {
      switch(Value.toInt())
      {
        case 0: 
          if(((varConfig.NW_Flags & NW_EthernetActive) && Ethernet.linkStatus()== LinkON))
            WiFi.disconnect();
          break;
        case 1:
          if(!WiFi.isConnected())
            NetworkInit(true);
          break;
        default:
          break;
      }
    }
    else if(TempTopic.substring(strlen(varConfig.MQTT_rootpath) + MQTTSubscribeRoot[1].length())=="AirSens_active")
    {
      AirSensValues->setSensorState(Value.toInt());
    }
    else if((TempTopic.substring(strlen(varConfig.MQTT_rootpath) + MQTTSubscribeRoot[1].length())=="ESP_Restart")&&(Value.toInt() == 1))
    {
      ESP_Restart = 1;
    }
  }  
  else if(TempTopic.substring(strlen(varConfig.MQTT_rootpath), (strlen(varConfig.MQTT_rootpath) + MQTTSubscribeRoot[2].length()))==MQTTSubscribeRoot[2])
  {
    int OutputIndex = FindOutputName(&topic[strlen(varConfig.MQTT_rootpath) + MQTTSubscribeRoot[2].length()]), ValueTemp = 0;
    if(OutputIndex < 0)
    { 
      MQTT_SendFailureText("Ausgang nicht gefunden: " + TempTopic.substring(strlen(varConfig.MQTT_rootpath) + MQTTSubscribeRoot[2].length()));
      return;
    }
    ValueTemp = Value.toInt();
    if((ValueTemp >= Output_Values.PWM_Min)&&(ValueTemp <= Output_Values.PWM_Max))
    {
      Output_Values.PWM_Value[OutputIndex] = (uint8) ValueTemp;
      MQTT_sendMessage((MQTTSendRoot[2] + OutputsBasicSettings[OutputIndex].Name).c_str(), ValueTemp);
    }    
  }
}
int FindOutputName(const char* Topic)
{
  for(int i = 0; i < 8; i++)
  {
    if(!strcmp(Topic, OutputsBasicSettings[i].Name))
      return i;
  }
  return -1;
}
void MQTT_SendInputStates()
{
  String Temp = MQTTSendRoot[1] + "InputPort";
  if(MQTTclient == 0)
    return;

  if(!MQTTclient->connected())
    return;

  MQTT_sendMessage(Temp.c_str(), (uint8) ~Inputs.StatesHW);
  for(int i = 0; i<8; i++)
  {
    Temp = MQTTSendRoot[0];
    Temp += OutputsBasicSettings[7-i].Name;
    MQTT_sendMessage(Temp.c_str(), Inputs.OnTimeRatio[i]);
    Temp.replace(MQTTSendRoot[0], MQTTSendRoot[1]);
    MQTT_sendMessage(Temp.c_str(), Inputs.StatesHW&1<<i?0:1);
  }
}
void MQTT_SendFailureText(String Text, bool withDate)
{
  char strDate[50] = "";
  if(withDate)
  {
    unsigned long long epochTime = timeClient->getEpochTime();
    struct tm *ptm = gmtime((time_t *)&epochTime);
    int monthDay = ptm->tm_mday;
    int currentMonth = ptm->tm_mon + 1;
    int currentYear = ptm->tm_year + 1900;
    sprintf(strDate, "%s - %02d.%02d.%d -> ", timeClient->getFormattedTime().c_str(), monthDay, currentMonth, currentYear);
  }
  Text = strDate + Text;
  MQTT_sendMessage((MQTTSendRoot[3] + "Fehler").c_str(), (const uint8 *) Text.c_str(), Text.length());
}
bool MQTT_sendMessage(const char * ValueName, const uint8* MSG, uint8 len)
{
  int lenPath = strlen(varConfig.MQTT_rootpath);
  char strPathVar[lenPath+30];
  #ifdef BGTDEBUG
    Serial.println("Vor connected Abfrage in MQTT_SenMessage");
  #endif

  if(MQTTclient == 0)
    return false;

  if(!MQTTclient->connected())
    return false;
  #ifdef BGTDEBUG
    Serial.println("Nach connected Abfrage in MQTT_SenMessage");
  #endif

  sprintf(strPathVar, "%s/%s", varConfig.MQTT_rootpath, ValueName);
  return MQTTclient->publish(strPathVar, MSG, len, true);
  
}
bool MQTT_sendMessage(const char * ValueName, int MSG)
{
  return MQTT_sendMessage(ValueName, (const uint8*) IntToStr(MSG).c_str(), IntToStr(MSG).length());
}
bool MQTT_sendMessage(const char * ValueName, uint8 MSG)
{
  return MQTT_sendMessage(ValueName, (const uint8*) IntToStr(MSG).c_str(), IntToStr(MSG).length());
}
bool MQTT_sendMessage(const char * ValueName, uint32 MSG)
{
  return MQTT_sendMessage(ValueName, (const uint8*) IntToStr(MSG).c_str(), IntToStr(MSG).length());
}
bool MQTT_sendMessage(const char * ValueName, float MSG)
{
  return MQTT_sendMessage(ValueName, (const uint8*) IntToStr(MSG).c_str(), IntToStr(MSG).length());
}
//---------------------------------------------------------------------
