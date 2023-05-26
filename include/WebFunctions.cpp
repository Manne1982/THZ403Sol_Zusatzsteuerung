#include "WebFunctions.h"
#include "GlobalVariabels.h"
#include "MQTT_Functions.h"
#include "HTML_Var.h"
#include "ProjectFunctions.h"


void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void WebserverRoot(AsyncWebServerRequest *request)
{
  char *Header_neu = new char[(strlen(html_header) + 50)];
  char *Body_neu = new char[(strlen(html_NWconfig)+750)];
  char *HTMLString = new char[(strlen(html_header) + 50)+(strlen(html_NWconfig)+750)];
  //Vorbereitung Datum
  unsigned long long epochTime = timeClient->getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon + 1;
  int currentYear = ptm->tm_year + 1900;

  char *pntSelected[5];
  for (int i = 0; i < 5; i++)
    if (i == (varConfig.NW_NTPOffset + 2))
      pntSelected[i] = (char *)varSelected[1].c_str();
    else
      pntSelected[i] = (char *)varSelected[0].c_str();
  sprintf(Header_neu, html_header, timeClient->getFormattedTime().c_str(), WeekDays[timeClient->getDay()].c_str(), monthDay, currentMonth, currentYear, Output_Values.realValue, Output_Values.Outputstates);
  sprintf(Body_neu, html_NWconfig, Un_Checked[varConfig.NW_Flags & NW_WiFi_AP].c_str(), varConfig.WLAN_SSID, 
              Un_Checked[(varConfig.NW_Flags & NW_EthernetActive)/NW_EthernetActive].c_str(), Un_Checked[(varConfig.NW_Flags & NW_StaticIP)/NW_StaticIP].c_str(), varConfig.NW_IPAddress, varConfig.NW_IPAddressEthernet, varConfig.NW_NetzName, varConfig.NW_SubMask, varConfig.NW_Gateway, varConfig.NW_DNS, 
              varConfig.NW_NTPServer, pntSelected[0], pntSelected[1], pntSelected[2], pntSelected[3], pntSelected[4], 
              Un_Checked[(varConfig.NW_Flags & NW_MQTTActive)/NW_MQTTActive].c_str(), varConfig.MQTT_Server, varConfig.MQTT_Port, varConfig.MQTT_Username, varConfig.MQTT_rootpath, Un_Checked[(varConfig.NW_Flags & NW_MQTTSecure)/NW_MQTTSecure].c_str());
  sprintf(HTMLString, "%s%s", Header_neu, Body_neu);
//  request->send(200, "text/html", HTMLString);

  AsyncBasicResponse *response = new AsyncBasicResponse(200, "text/html", HTMLString);
  request->send(response);

  delete[] HTMLString;
  delete[] Body_neu;
  delete[] Header_neu;
}
void WebserverSensors(AsyncWebServerRequest *request)
{
  int countSensors = SensorPort1.GetSensorCount() + SensorPort2.GetSensorCount();
  TempSensor * MissingSensors[MaxSensors];
  uint8 countMissingSensors = FindMissingSensors(&SensorPort1, &SensorPort2, MissingSensors, TempSensors, MaxSensors);
  uint16 StringLen = (strlen(html_header) + 50)+strlen(html_SEconfig1)+(countSensors * (strlen(html_SEconfig2) + 120))+strlen(html_SEconfig3);
  StringLen += ((strlen(html_SEconfig4)+100)*countMissingSensors) + strlen(html_OPSEfooter);
  char *HTMLString = new char[StringLen];
  char *HTMLString2 = new char[StringLen];
  //Vorbereitung Datum 
  unsigned long long epochTime = timeClient->getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon + 1;
  int currentYear = ptm->tm_year + 1900;
  sprintf(HTMLString, html_header, timeClient->getFormattedTime().c_str(), WeekDays[timeClient->getDay()].c_str(), monthDay, currentMonth, currentYear, Output_Values.realValue, Output_Values.Outputstates);
  sprintf(HTMLString2, html_SEconfig1, HTMLString, Ein_Aus[AirSensValues->getSensorState()].c_str(), AirSensValues->getOnTime_s(), AirSensValues->readLPG(), AirSensValues->readCO(), 
                  AirSensValues->readSMOKE(), AirSensValues->getHWValue());
  for(int i = 0; i < SensorPort1.GetSensorCount(); i++)
  {
    sprintf(HTMLString, html_SEconfig2, HTMLString2, SensorPort1.GetSensorIndex(i)->getAddressHEX().c_str(), SensorPort1.GetSensorIndex(i)->getTempC(), 1, 
                  SensorPort1.GetSensorIndex(i)->getAddressUINT64(), SensorPort1.GetSensorIndex(i)->getName().c_str(), 1, SensorPort1.GetSensorIndex(i)->getAddressUINT64(), 
                  SensorPort1.GetSensorIndex(i)->getOffset(), 1, SensorPort1.GetSensorIndex(i)->getAddressUINT64(), 
                  FindTempSensor(TempSensors, MaxSensors, SensorPort1.GetSensorIndex(i)->getAddressUINT64())->SensorState);
    strcpy(HTMLString2, HTMLString);
  }
  for(int i = 0; i < (SensorPort2.GetSensorCount()); i++)
  {
    sprintf(HTMLString, html_SEconfig2, HTMLString2, SensorPort2.GetSensorIndex(i)->getAddressHEX().c_str(), SensorPort2.GetSensorIndex(i)->getTempC(), 2, 
                  SensorPort2.GetSensorIndex(i)->getAddressUINT64(), SensorPort2.GetSensorIndex(i)->getName().c_str(), 2, SensorPort2.GetSensorIndex(i)->getAddressUINT64(), 
                  SensorPort2.GetSensorIndex(i)->getOffset(), 2, SensorPort2.GetSensorIndex(i)->getAddressUINT64(),
                  FindTempSensor(TempSensors, MaxSensors, SensorPort2.GetSensorIndex(i)->getAddressUINT64())->SensorState);
    strcpy(HTMLString2, HTMLString);
  }
  sprintf(HTMLString, html_SEconfig3, HTMLString2);
  for(int i = 0; i < countMissingSensors; i++)
  {
    sprintf(HTMLString2, html_SEconfig4, HTMLString, convertUINT64toHEXstr(&MissingSensors[i]->Address).c_str(), MissingSensors[i]->Name, MissingSensors[i]->Offset, 
                  MissingSensors[i]->SensorState, MissingSensors[i]->Address);
    strcpy(HTMLString, HTMLString2);  
  }
  sprintf(HTMLString2, html_OPSEfooter, HTMLString);
  AsyncBasicResponse *response = new AsyncBasicResponse(200, "text/html", HTMLString2);
  request->send(response);

//  request->send(200, "text/html", HTMLString2); //at big sites >6400 characters not functional
  delete[] HTMLString;
  delete[] HTMLString2;
}
void WebserverOutput(AsyncWebServerRequest *request)
{
  uint16 StringLen = (strlen(html_header) + 50)+strlen(html_OPconfig1)+(8 * (strlen(html_OPconfig2) + 100))+ strlen(html_OPconfig1) + 20 + strlen(html_OPSEfooter);
  char *HTMLString = new char[StringLen];
  char *HTMLString2 = new char[StringLen];
  //Vorbereitung Datum 
  unsigned long long epochTime = timeClient->getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon + 1;
  int currentYear = ptm->tm_year + 1900;
  int TempCurrentOutputState = 0;
  sprintf(HTMLString, html_header, timeClient->getFormattedTime().c_str(), WeekDays[timeClient->getDay()].c_str(), monthDay, currentMonth, currentYear, Output_Values.realValue, Output_Values.Outputstates);
  sprintf(HTMLString2, "%s%s", HTMLString, html_OPconfig1);
  for(int i = 0; i < 8; i++)
  {
    TempCurrentOutputState = (~Output_Values.Outputstates &((uint16) 1<<(i+8)))/((uint16)1<<(i+8)); //Manuel On or Off
    TempCurrentOutputState = (Output_Values.Outputstates &((uint16) 1<<i))?2:TempCurrentOutputState; //Auto or Manuell
    TempCurrentOutputState = Output_Values.OutputstatesAutoSSRelais&((uint8) 1<<i)?3:TempCurrentOutputState; //Auto over Solid state relais
    TempCurrentOutputState = Output_Values.PWM_Manu_activ&((uint8) 1<<i)?4:TempCurrentOutputState; //Manu PWM-Mode
    sprintf(HTMLString, html_OPconfig2, HTMLString2, i, i, OutputsBasicSettings[i].Name, TempCurrentOutputState, Inputs.OnTimeRatio[7-i], i, OutputsBasicSettings[i].StartValue, i, 
                  Un_Checked[OutputsBasicSettings[i].MQTTState%2].c_str());
    strcpy(HTMLString2, HTMLString);
  }
  sprintf(HTMLString, html_OPconfig3, HTMLString2, ValveHeating->getChannelOpen(), ValveHeating->getChannelClose(), ValveHeating->getCycleTimeOpen(), ValveHeating->getCycleTimeClose(), ValveHeating->getValvePosition());
  sprintf(HTMLString2, html_OPSEfooter, HTMLString);
  AsyncBasicResponse *response = new AsyncBasicResponse(200, "text/html", HTMLString2);
  request->send(response);
//  request->send(200, "text/html", HTMLString2);
  delete[] HTMLString;
  delete[] HTMLString2;
}
void WebserverPOST(AsyncWebServerRequest *request)
{
  int parameter = request->params();
  unsigned short int *submitBereich;
  if (parameter)
  {
    submitBereich = (unsigned short int *)request->getParam(0)->name().c_str();
    switch (*submitBereich)
    {
    case subwl:
      varConfig.NW_Flags &= ~NW_WiFi_AP;
      if(parameter <= 3)
      for (int i = 0; i < parameter; i++)
      {
        if (request->getParam(i)->name() == "wlAP")
          varConfig.NW_Flags |= NW_WiFi_AP;
        else if (request->getParam(i)->name() == "wlSSID")
        {
          if (request->getParam(i)->value().length() < 40)
            strcpy(varConfig.WLAN_SSID, request->getParam(i)->value().c_str());
          else
          {
            request->send_P(200, "text/html", "SSID zu lang<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
            return;
          }
        }
        else if (request->getParam(i)->name() == "wlPassword")
        {
          if(request->getParam(i)->value()!="xxxxxx")
          {
            if ((request->getParam(i)->value().length() <= 60)&&(request->getParam(i)->value().length() >= 8))
            { 
              strcpy(varConfig.WLAN_Password, request->getParam(i)->value().c_str());
            }
            else
            {
              request->send_P(200, "text/html", "Passwortlaenge muss zwischen 8 und 60 Zeichen liegen<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
              return;
            }
          }
        }
        else
        {
          request->send_P(200, "text/html", "Unbekannter Rueckgabewert<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
          return;
        }
      }
      request->send_P(200, "text/html", "Daten wurden uebernommen und ESP wird neu gestartet!<br><a href=\\Settings\\>Zurueck</a> <meta http-equiv=\"refresh\" content=\"20; URL=\\\">"); //<a href=\>Startseite</a>
      EinstSpeichern();
      ESP_Restart = true;
      break;
    case subnw:
    {
      char tmp_StatischeIP = 0;
      char tmp_EthernetOn = 0;
      String tmp_IPAddress[5];
      String tmp_NTPServer;
      String tmp_NetzName;
      int tmp_NTPOffset;
      if(parameter < 11)
      for (int i = 0; i < parameter; i++)
      {
        if (request->getParam(i)->name() == "nwEthernetOn")
          tmp_EthernetOn = 1;
        else if (request->getParam(i)->name() == "nwSIP")
          tmp_StatischeIP = 1;
        else if (request->getParam(i)->name() == "nwIP")
          tmp_IPAddress[0] = request->getParam(i)->value();
        else if (request->getParam(i)->name() == "nwIPEthernet")
          tmp_IPAddress[1] = request->getParam(i)->value();
        else if (request->getParam(i)->name() == "nwNetzName")
          tmp_NetzName = request->getParam(i)->value();
        else if (request->getParam(i)->name() == "nwSubnet")
          tmp_IPAddress[2] = request->getParam(i)->value();
        else if (request->getParam(i)->name() == "nwGateway")
          tmp_IPAddress[3] = request->getParam(i)->value();
        else if (request->getParam(i)->name() == "nwDNS")
          tmp_IPAddress[4] = request->getParam(i)->value();
        else if (request->getParam(i)->name() == "nwNTPServer")
          tmp_NTPServer = request->getParam(i)->value();
        else if (request->getParam(i)->name() == "nwNTPOffset")
          sscanf(request->getParam(i)->value().c_str(), "%d", &tmp_NTPOffset);
        else
        {
          request->send_P(200, "text/html", "Unbekannter Rueckgabewert<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
          return;
        }
      }
      if (tmp_StatischeIP)
        if ((tmp_IPAddress[0].length() == 0) || (tmp_IPAddress[1].length() == 0))
        {
          request->send_P(200, "text/html", "Bei Statischer IP-Adresse wird eine IP-Adresse und eine Subnet-Mask benoetigt!<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
          return;
        }
      if(tmp_StatischeIP){
        varConfig.NW_Flags |= NW_StaticIP;}
      else{
        varConfig.NW_Flags &= ~NW_StaticIP;}
      if(tmp_EthernetOn){
        varConfig.NW_Flags |= NW_EthernetActive;}
      else{
        varConfig.NW_Flags &= ~NW_EthernetActive;}
      strcpy(varConfig.NW_IPAddress, tmp_IPAddress[0].c_str());
      strcpy(varConfig.NW_IPAddressEthernet, tmp_IPAddress[1].c_str());
      strcpy(varConfig.NW_NetzName, tmp_NetzName.c_str());
      strcpy(varConfig.NW_SubMask, tmp_IPAddress[2].c_str());
      strcpy(varConfig.NW_Gateway, tmp_IPAddress[3].c_str());
      strcpy(varConfig.NW_DNS, tmp_IPAddress[4].c_str());
      strcpy(varConfig.NW_NTPServer, tmp_NTPServer.c_str());
      varConfig.NW_NTPOffset = tmp_NTPOffset;
      request->send_P(200, "text/html", "Daten wurden uebernommen und ESP wird neu gestartet!<br><meta http-equiv=\"refresh\" content=\"20; URL=\\\">"); //<a href=\>Startseite</a>
      EinstSpeichern();
      ESP_Restart = true;
      break;
    }
    case submq:
    {
      char tmp_MQTTOn = 0, tmp_MQTTSecureOn = 0;
      String Temp[6];
      for (int i = 0; i < parameter; i++)
      {
        if (request->getParam(i)->name() == "mqMQTTOn")
          tmp_MQTTOn = 1;
        else if (request->getParam(i)->name() == "mqServer")
          Temp[0] = request->getParam(i)->value();
        else if (request->getParam(i)->name() == "mqPort")
          Temp[1] = request->getParam(i)->value();
        else if (request->getParam(i)->name() == "mqUser")
          Temp[2] = request->getParam(i)->value();
        else if (request->getParam(i)->name() == "mqPassword")
          Temp[3] = request->getParam(i)->value();
        else if (request->getParam(i)->name() == "mqRootpath")
          Temp[4] = request->getParam(i)->value();
        else if (request->getParam(i)->name() == "mqMQTTsecureOn")
          tmp_MQTTSecureOn = 1;
        else if (request->getParam(i)->name() == "mqFPrint")
          Temp[5] = request->getParam(i)->value();
        else
        {
          request->send_P(200, "text/html", "Unbekannter Rueckgabewert<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
          return;
        }
      }
      if(tmp_MQTTOn){
        varConfig.NW_Flags |= NW_MQTTActive;}
      else{
        varConfig.NW_Flags &= ~NW_MQTTActive;}
      if(tmp_MQTTSecureOn){
        varConfig.NW_Flags |= NW_MQTTSecure;}
      else{
        varConfig.NW_Flags &= ~NW_MQTTSecure;}
      if((Temp[0].length()<49)&&(Temp[0].length()>5))
        strcpy(varConfig.MQTT_Server, Temp[0].c_str());
      if((Temp[1].length()<6)&&(Temp[1].length()>1))
        varConfig.MQTT_Port = Temp[1].toInt();
      if((Temp[2].length()<19)&&(Temp[2].length()>5))
        strcpy(varConfig.MQTT_Username, Temp[2].c_str());
      if((Temp[3].length()<=60)&&(Temp[3].length()>=5)&&(Temp[3]!= "xxxxxx"))
        strcpy(varConfig.MQTT_Password, Temp[3].c_str());
      if((Temp[4].length()<95)&&(Temp[4].length()>5))
        strcpy(varConfig.MQTT_rootpath, Temp[4].c_str());
      if((Temp[5].length()<=65)&&(Temp[5].length()>5)&&(Temp[5]!= "xxxxxx"))
        strcpy(varConfig.MQTT_fprint, Temp[5].c_str());
      EinstSpeichern();
      request->send_P(200, "text/html", "MQTT Daten wurden uebernommen, ESP startet neu!<br><meta http-equiv=\"refresh\" content=\"20; URL=\\\">"); //<a href=\>Startseite</a>
      ESP_Restart = true;
      break;
    }               
    case subPS:
    {
      uint64 Address = 0;
      int Port = 0;
      int Value = 0;
      float TempOffset = 0;
      unsigned int Temp = 0;
      for (int i = 0; i < parameter; i++)
      {
        if(sscanf(request->getParam(i)->name().c_str(), "PS_%d_%d_", &Port, &Value) != 2)
        {          
          request->send_P(200, "text/html", "Unbekannter Rueckgabewert Index 720<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
          return;
        }
        Address = StrToLongInt(request->getParam(i)->name().substring(6));
        switch(Value)
        {
          case 1:
            if(request->getParam(i)->value().length()>15)
            {
              request->send_P(200, "text/html", "Unbekannter Rueckgabewert Index 729<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
              return;
            }
            strcpy(FindTempSensor(TempSensors, MaxSensors, Address)->Name, request->getParam(i)->value().c_str());
            if(Port == 1)
              SensorPort1.GetSensorAddr(Address)->setName(request->getParam(i)->value());
            else
              SensorPort2.GetSensorAddr(Address)->setName(request->getParam(i)->value());
            break;
          case 2:
            if(sscanf(request->getParam(i)->value().c_str(),"%f", &TempOffset)!=1)
            {
              request->send_P(200, "text/html", "Unbekannter Rueckgabewert Index 741<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
              return;
            }
            FindTempSensor(TempSensors, MaxSensors, Address)->Offset = TempOffset;
            if(Port == 1)
              SensorPort1.GetSensorAddr(Address)->setOffset(TempOffset);
            else
              SensorPort2.GetSensorAddr(Address)->setOffset(TempOffset);
            break;
          case 3:
            if(sscanf(request->getParam(i)->value().c_str(),"%u", &Temp)!=1)
            {
              request->send_P(200, "text/html", "Unbekannter Rueckgabewert Index 753<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
              return;
            }
            FindTempSensor(TempSensors, MaxSensors, Address)->SensorState = Temp;
            break;
          default:
            request->send_P(200, "text/html", "Unbekannter Rueckgabewert Index 763<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
            return;
        }
      }
      EinstSpeichern();
      request->send_P(200, "text/html", "Sensordaten wurden uebernommen!<br><meta http-equiv=\"refresh\" content=\"2; URL=/Sensors/\">"); //<a href=\>Startseite</a>
      break;
    }
    case subSD:
    {
      if((parameter > 1)||(request->getParam(0)->name()!="SDelete"))
      {
        request->send_P(200, "text/html", "Unbekannter Rueckgabewert Index 772<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
        return;
      }
      TempSensor * Temp = 0;
      uint64 Address = StrToLongInt(request->getParam(0)->value());
      Temp = FindTempSensor(TempSensors, MaxSensors, Address);
      if(Temp)
      DelTSensor(Temp);
      EinstSpeichern();
      request->send_P(200, "text/html", "Sensor wurde geloescht!<br><meta http-equiv=\"refresh\" content=\"2; URL=/Sensors/\">"); //<a href=\>Startseite</a>
      //ESP_Restart = true;
      break;
    }   
    case subSS:
    {
      SensorPort1.SensorSearch();
      SensorPort2.SensorSearch();
      TakeoverTSConfig(&SensorPort1, TempSensors, MaxSensors);
      TakeoverTSConfig(&SensorPort2, TempSensors, MaxSensors);
      request->send_P(200, "text/html", "Sensoren wurden neu eingelesen!<br><meta http-equiv=\"refresh\" content=\"2; URL=/Sensors/\">"); //<a href=\>Startseite</a>
      break;
    }            
    case subAS:
    {
      if(AirSensValues->toggleSensorState())
      {
        request->send_P(200, "text/html", "Luft-Sensor wurde eingeschaltet!<br><meta http-equiv=\"refresh\" content=\"2; URL=/Sensors/\">");
      }
      else
      {
        request->send_P(200, "text/html", "Luft-Sensor wurde ausgeschaltet!<br><meta http-equiv=\"refresh\" content=\"2; URL=/Sensors/\">");
      }
      break;
    }            
    case subOS:
    {
      int Port = 0;
      int Value = 0;
      for (int i = 0; i < parameter; i++)
      {
        if(sscanf(request->getParam(i)->name().c_str(), "OS_%d_%d_", &Port, &Value) != 2)
        {          
          request->send_P(200, "text/html", "Unbekannter Rueckgabewert Index 808<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
          return;
        }
        OutputsBasicSettings[Port].MQTTState = 0;
        switch(Value)
        {
          case 1:
            if(request->getParam(i)->value().length()>15)
            {
              request->send_P(200, "text/html", "Name zu lang Index 814<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
              return;
            }
            strcpy(OutputsBasicSettings[Port].Name, request->getParam(i)->value().c_str());
            break;
          case 2:
            if((request->getParam(i)->value().toInt() > 4) || (request->getParam(i)->value().toInt() < 0))
            {
              request->send_P(200, "text/html", "Unbekannter Rueckgabewert Index 822<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
              return;
            }
            OutputsBasicSettings[Port].StartValue = request->getParam(i)->value().toInt();
            break;
          case 3:
            if(request->getParam(i)->value()!="on")
            {
              request->send_P(200, "text/html", "Unbekannter Rueckgabewert Index 830<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
              return;
            }
            OutputsBasicSettings[Port].MQTTState = 1;
            break;
          default:
            request->send_P(200, "text/html", "Unbekannter Rueckgabewert Index 836<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
            return;
        }
      }
      EinstSpeichern();
      request->send_P(200, "text/html", "Relaiseinstellungen wurden uebernommen!<br><meta http-equiv=\"refresh\" content=\"2; URL=/Output/\">"); //<a href=\>Startseite</a>
      break;
   }            
    case subWV:
    {
      uint16 ValueType = 0, Value = 0;
      for (int i = 0; i < parameter; i++)
      {
        if(sscanf(request->getParam(i)->name().c_str(), "WV_%hu", &ValueType) > 3)
        {          
          request->send_P(200, "text/html", "Unbekannter Rueckgabewert Index 808<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
          return;
        }
        Value = request->getParam(i)->value().toInt();
        switch(ValueType)
        {
          case 0:
            if(Value >7)
            {
              request->send_P(200, "text/html", "Channel 'Open' invalid Index 1206<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
              return;
            }
            ValveHeating->setChannelOpen(Value);
            break;
          case 1:
            if(Value >7)
            {
              request->send_P(200, "text/html", "Channel 'Close' invalid Index 1214<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
              return;
            }
            break;
          case 2:
            if(Value >1000)
            {
              request->send_P(200, "text/html", "Cycle time 'Open' invalid Index 1221<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
              return;
            }
            ValveHeating->setCycleTimeOpen(Value);
            break;
          case 3:
            if(Value >1000)
            {
              request->send_P(200, "text/html", "Cycle time 'Close' invalid Index 1229<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
              return;
            }
            ValveHeating->setCycleTimeClose(Value);
            break;
          default:
            request->send_P(200, "text/html", "Unbekannter Rueckgabewert Index 1235<form> <input type=\"button\" value=\"Go back!\" onclick=\"history.back()\"></form>");
            return;
        }
      }
      EinstSpeichern();
      request->send_P(200, "text/html", "Ventileinstellungen wurden uebernommen!<br><meta http-equiv=\"refresh\" content=\"2; URL=/Output/\">"); //<a href=\>Startseite</a>
      break;
   }            
    default:
      char strFailure[50];
      sprintf(strFailure, "Anweisung unbekannt, Empfangen: %u", *submitBereich);
      request->send_P(200, "text/html", strFailure);
      break;
    }
  }

}