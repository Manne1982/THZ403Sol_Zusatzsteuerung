#include <pgmspace.h>
#include <Arduino.h>

//Definitionen fuer Datum und Uhrzeit
// enum {clh, clmin}; //clh ist 0 fuer Uhrzeit-Stunden, clmin ist 1 fuer Uhrzeit Minuten
// enum {dtYear, dtMonth, dtDay}; //Tag = 2; Monat = 1; Jahr = 0
const String WeekDays[7]={"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};
// //Allgemeine Definitionen
enum {subwl = 27767, subnw = 30574, subcn = 20035, subpd = 17488, subps = 21328, sublf = 17996, sublm = 19788, submq = 29037, subPS = 21328, subSD = 17491, subSS = 21331, subOS = 21327, subAS = 21313, subWV = 22103}; //Zuordnung der Submit-Bereiche einer Ganzzahl
enum {touchUp_wsgn = 33, touchDown_gn = 32, touchRight_wsbr = 15, touchLeft_br = 4, touchF1_bl = 13, touchF2_wsbl = 12, touchF3_or = 14, touchF4_wsor = 27, RGB_Red = 22, RGB_Green = 16, RGB_Blue = 17, Display_Beleuchtung = 21};
enum {indexAGM = 0, indexLED = 1, indexProgStart = 2, indexNWK = 3};
enum {NW_WiFi_AP = 0x01, NW_StaticIP = 0x02, NW_EthernetActive = 0x04, NW_MQTTActive = 0x08, NW_MQTTSecure = 0x10}; //Enum for NWConfig
const String Un_Checked[2]{"","Checked"};
const String varSelected[2]{"", " selected=\"\""};
// const String De_Aktiviert[2]{"Deaktiviert","Aktiviert"};
const String Ein_Aus[3]{"Aus","Ein","---"};



const char html_header[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
 <title>Heizung Zusatzsteuerung</title>
 <meta name="viewport" content="width=device-width, initial-scale=1", charset="UTF-8">
</head>
<body bgcolor=\"#BBFFFF\">
Uhrzeit: %s | Datum: %s, %02d.%02d.%d | Status: %u, %X
<br />
<hr><h3>
<a href=\>Startseite</a> | 
<a href=\Sensors\>Sensor-Einstellungen</a> | 
<a href=\Output\>Relais-Einstellungen</a> | 

</h3><hr>
)rawliteral";


const char html_SEconfig1[] PROGMEM = R"rawliteral(
%s
<h1>Sensor Einstellungen</h1><hr>
<h2>Luftsensor</h2><br />
<table> <!-- 'border="1"-->
 <tbody><tr>
 <td width="150" valign="TOP">
  <h3>Sensorstatus</h3> <br></td>
 <td width="150" valign="TOP">
	 <h3>Einschaltdauer</h3></td>
 <td width="150" valign="TOP">
	 <h3>Messwert LPG</h3></td>
 <td width="200" valign="TOP">
	 <h3>Messwert CO</h3></td>
 <td width="200" valign="TOP">
	 <h3>Messwert Rauch</h3></td>
 <td width="200" valign="TOP">
	 <h3>Messwert AD Wandler</h3></td>
 <td width="200" valign="TOP">
	 </td>
 </tr>
 <tr>
 <form method="post" action="/POST">
 <td valign="TOP">
	 %s</td>
 <td valign="TOP">
	 %u s</td>
 <td valign="TOP">
	 %f ppm</td>
 <td valign="TOP">
	 %f ppm</td>
 <td valign="TOP">
	 %f ppm</td>
 <td valign="TOP">
	 %d</td>
 <td valign="TOP">
	 <input type="hidden" name="AS_OnOff" value="all"><input value="Ein/Aus" type="submit"></td>
 </tr></form>
</tbody></table>
<hr>
<datalist id="SensorStates">
 <option value="1">Aktiv</option>
 <option value="0">nicht konfiguriert</option>
 <option value="2">nicht verbunden</option>
</datalist>
<h2>Temperatursensoren</h2><br />
<form method="post" action="/POST">
<input type="hidden" name="SSearch" value="all"><input value="Sensorsuche" type="submit">
</form>
<TABLE> <!-- 'border="1"-->
 <TR>
 <TD WIDTH="200" VALIGN="TOP">
  <h3>Sensoradresse</h3> <br /></TD>
 <TD WIDTH="150" VALIGN="TOP">
	 <h3>Temperatur</h3></TD>
 <TD WIDTH="150" VALIGN="TOP">
	 <h3>Sensorname</h3></TD>
 <TD WIDTH="120" VALIGN="TOP">
	 <h3>Offset in °K</h3></TD>
 <TD WIDTH="80" VALIGN="TOP">
	 <h3>Aktiv</h3></TD>
 <TD WIDTH="200" VALIGN="TOP">
	 </TD>
 </TR>
)rawliteral";

const char html_SEconfig2[] PROGMEM = R"rawliteral(
 %s
 <TR>
 <form method="post" action="/POST">
 <TD VALIGN="TOP">
	 %s</TD>
 <TD VALIGN="TOP">
	 %0.2f °C</TD>
 <TD VALIGN="TOP">
	 <input name="PS_%d_1_%llu" type="text" maxlength="14" size="15" value="%s" required="1"><br /><br /></TD>
 <TD VALIGN="TOP">
	 <input name="PS_%d_2_%llu" type="number" min="-5" max="+5" step="0.01" maxlength="4" size="5" value="%0.1f" required="1"><br /><br /></TD>
 <TD VALIGN="TOP">
	 <input name="PS_%d_3_%llu" type="number" min="0" max="2" step="1" size="5" value="%u" list="SensorStates" required="1"><br /><br /></TD>
 <TD VALIGN="TOP">
	 <input type="reset"><input value="Submit" type="submit"></TD>
 </form>
 </TR>
)rawliteral";
const char html_SEconfig3[] PROGMEM = R"rawliteral(
%s
</TABLE>
<hr>
<h2>Fehlende Temperatursensoren</h2><br />
<TABLE> <!-- 'border="1"-->
 <TR>
 <TD WIDTH="200" VALIGN="TOP">
 <h3>Sensoradresse</h3> <br /></TD>
 <TD WIDTH="150" VALIGN="TOP">
	 <h3>Sensorname</h3></TD>
 <TD WIDTH="120" VALIGN="TOP">
	 <h3>Offset in °K</h3></TD>
 <TD WIDTH="80" VALIGN="TOP">
	 <h3>Aktiv</h3></TD>
 <TD WIDTH="200" VALIGN="TOP">
	 </TD>
 </TR>
)rawliteral";
const char html_SEconfig4[] PROGMEM = R"rawliteral(
 %s
 <TR>
 <form method="post" action="/POST">
 <TD VALIGN="TOP">
	 %s</TD>
 <TD VALIGN="TOP">
	 %s</TD>
 <TD VALIGN="TOP">
	 %0.1f</TD>
 <TD VALIGN="TOP">
	 %u</TD>
 <TD VALIGN="TOP">
	 <input type="hidden" name="SDelete" value="%llu"><input value="Delete" type="submit"></TD>
 </form>
 </TR>
)rawliteral";
const char html_OPSEfooter[] PROGMEM = R"rawliteral(
%s
</TABLE>
<br />
</body>
</html>
)rawliteral";

const char html_OPconfig1[] PROGMEM = R"rawliteral(
<h1>Relais Einstellungen</h1><hr>
<datalist id="OutputStates">
  <option value="0">Aus</option>
  <option value="1">An</option>
  <option value="2">Automatik</option>
  <option value="3">Automatik over SSR</option>
</datalist>
<TABLE> <!-- 'border="1"-->
  <TR>
    <TD WIDTH="100" VALIGN="TOP">
      <h3>Ausgang</h3> <br /></TD>
    <TD WIDTH="200" VALIGN="TOP">
      <h3>Bezeichnung</h3> <br /></TD>
    <TD WIDTH="150" VALIGN="TOP">
	  <h3>Aktueller Modus</h3></TD>
    <TD WIDTH="150" VALIGN="TOP">
	  <h3>Heizung ist als %</h3></TD>
    <TD WIDTH="150" VALIGN="TOP">
	  <h3>Anfangsmodus</h3></TD>
    <TD WIDTH="80" VALIGN="TOP">
	  <h3>Aktiv</h3></TD>
    <TD WIDTH="200" VALIGN="TOP">
	  </TD>
  </TR>
)rawliteral";
const char html_OPconfig2[] PROGMEM = R"rawliteral(
%s  <TR>
  <form method="post" action="/POST">
    <TD VALIGN="TOP">
	  %d</TD>
    <TD VALIGN="TOP">
	  <input name="OS_%d_1" type="text" maxlength="14" size="15" value="%s" required="1"><br /><br /></TD>
    <TD VALIGN="TOP">
	  %d</TD>
    <TD VALIGN="TOP">
	  %hhu</TD>
    <TD VALIGN="TOP">
	  <input name="OS_%d_2" type="number" min="0" max="3" step="1" size="5" value="%d" list="OutputStates" required="1"><br /><br /></TD>
    <TD VALIGN="TOP">
	  <input name="OS_%d_3" %s type="checkbox"><br /><br /></TD>
    <TD VALIGN="TOP">
	  <input type="reset"><input value="Submit" type="submit"></TD>
  </form>
  </TR>
)rawliteral";
const char html_OPconfig3[] PROGMEM = R"rawliteral(
%s </TABLE>
<br />
</h3><hr>
<h1>3-Wege-Ventil Einstellungen</h1><hr>
<form method="post" action="/POST">
<TABLE> <!-- 'border="1"-->
  <TR>
    <TD WIDTH="300" VALIGN="TOP">
      Kanal "Auf"<br /><br /></TD>
    <TD WIDTH="200" VALIGN="TOP">
	  <input name="WV_0" type="number" min="0" max="7" step="1" size="5" value="%u" required="1"><br /><br /></TD>
  </TR>
  <TR>
    <TD VALIGN="TOP">
      Kanal "Zu"<br /><br /></TD>
    <TD VALIGN="TOP">
	  <input name="WV_1" type="number" min="0" max="7" step="1" size="5" value="%u" required="1"><br /><br /></TD>
  </TR>
  <TR>
    <TD VALIGN="TOP">
      Zykluszeit "Auf" in Sekunden<br /><br /></TD>
    <TD VALIGN="TOP">
	  <input name="WV_2" type="number" min="0" max="1000" step="1" size="5" value="%u" required="1"><br /><br /></TD>
  </TR>
  <TR>
    <TD VALIGN="TOP">
      Zykluszeit "Zu" in Sekunden<br /><br /></TD>
    <TD VALIGN="TOP">
	  <input name="WV_3" type="number" min="0" max="1000" step="1" size="5" value="%u" required="1"><br /><br /></TD>
  </TR>
  <TR>
    <TD VALIGN="TOP">
      Ventilposition (min=0, max=10000)<br /><br /></TD>
    <TD VALIGN="TOP">
	  %u<br /><br /></TD>
  </TR>
  <TR>
    <TD VALIGN="TOP">
      </TD>
    <TD VALIGN="TOP">
	  <input type="reset"><input value="Submit" type="submit"></form></TD>
  </TR>
)rawliteral";

const char html_NWconfig[] PROGMEM = R"rawliteral(
<h1>Heizung Zusatzsteuerung NW Einstellungen</h1><hr>
<h2>WLAN</h2>
<form method="post" action="/POST">
<TABLE>
  <TR>
    <TD WIDTH="120" VALIGN="TOP">
    Access Poin: </TD>
    <TD WIDTH="300" VALIGN="TOP">
    <input name="wlAP" value="1" type="checkbox" %s> <br /><br /></TD>
  </TR>
  <TR>
    <TD VALIGN="TOP">
      SSID: </TD>
  <TD>
    <input name="wlSSID" type="text" value="%s" minlength="2" maxlength="30" size="15" required="1"><br /><br /></TD>
  <TD>
    <br /><br /></TD>
  </TR>
  <TR>
    <TD VALIGN="TOP">
      Passwort: </TD>
  <TD>
    <input name="wlPassword" type="password" value="xxxxxx" minlength="8" maxlength="60" size="35"><br /><br /></TD>
  </TR>
</TABLE>
  <BR>
  <input value="Submit" type="submit">
</form>
<hr>
<h2>Netzwerk</h2><br />
<form method="post" action="/POST">
<TABLE>
  <TR>
    <TD WIDTH="200" VALIGN="TOP">
      Ethernet on: </TD>
    <TD WIDTH="200" VALIGN="TOP">
    <input name="nwEthernetOn" value="" type="checkbox" %s> <br /><br /></TD>
    <TD WIDTH="200" VALIGN="TOP">
    </TD>
  </TR>
  <TR>
    <TD VALIGN="TOP">
      Manuelle IP Konfiguration: </TD>
    <TD VALIGN="TOP">
    <input name="nwSIP" value="" type="checkbox" %s> <br /><br /></TD>
    <TD VALIGN="TOP">
    </TD>
  </TR>
  <TR>
    <TD VALIGN="TOP">
      IP-Adresse: </TD>
  <TD>
      <input name="nwIP" type="text" minlength="7" maxlength="15" size="15" value="%s" pattern="^((\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.){3}(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])$"><br /><br /></TD>
    <TD VALIGN="TOP">
       </TD>
  </TR>
  <TR>
    <TD VALIGN="TOP">
      IP-Adresse Ethernet: </TD>
  <TD>
      <input name="nwIPEthernet" type="text" minlength="7" maxlength="15" size="15" value="%s" pattern="^((\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.){3}(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])$"><br /><br /></TD>
    <TD VALIGN="TOP">
       </TD>
  </TR>
  <TR>
    <TD VALIGN="TOP">
      Netzwerkname: </TD>
  <TD>
    <input name="nwNetzName" type="text" minlength="3" maxlength="15"  value="%s" required="1"> <br /><br /></TD>
    <TD VALIGN="TOP">
      </TD>
  </TR>
  <TR>
    <TD VALIGN="TOP">
      Subnetmask: </TD>
  <TD>  
    <input name="nwSubnet" type="text" minlength="7" maxlength="15" size="15" value="%s" pattern="^((\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.){3}(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])$"><br /><br /></TD>
    <TD VALIGN="TOP">
       </TD>
  </TR>
  <TR>
    <TD VALIGN="TOP">
      Gateway: </TD>
  <TD>
    <input name="nwGateway" type="text" minlength="7" maxlength="15" size="15" value="%s" pattern="^((\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.){3}(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])$"><br /><br /></TD>
    <TD VALIGN="TOP">
      Wird nur bei einem externen NTP-Server benoetigt!</TD>
  </TR>
  <TR>
    <TD VALIGN="TOP">
      DNS-Server: </TD>
  <TD>
    <input name="nwDNS" type="text" minlength="7" maxlength="15" size="15" value="%s" pattern="^((\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.){3}(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])$"><br /><br /></TD>
    <TD VALIGN="TOP">
      Wird nur bei einem externen NTP-Server benoetigt!</TD>
  </TR>
  <TR>
    <TD VALIGN="TOP">
      Zeitserver (z.B. fritz.box): </TD>
  <TD>
    <input name="nwNTPServer" type="text" minlength="3" maxlength="50"  value="%s" required="1"> <br /><br /></TD>
    <TD VALIGN="TOP">
      </TD>
  </TR>
  <TR>
    <TD VALIGN="TOP">
      Zeitoffset (in Stunden): </TD>
    <TD>
      <select name="nwNTPOffset" autofocus="autofocus">
      <option%s>-2</option>
      <option%s>-1</option>
      <option%s>0</option>
      <option%s>+1</option>
      <option%s>+2</option>
      </select> 
    <BR /><BR /></TD>
    <TD VALIGN="TOP">
      </TD>
  </TR>

</TABLE>
    <BR>
  <input value="Submit" type="submit">
  </form>
<HR>
<h2>MQTT</h2><BR>
<form method="post" action="/POST">
<TABLE>
  <TBODY>
  <TR>
    <TD VALIGN="TOP">
      MQTT On/Off: </TD>
    <TD VALIGN="TOP">
    <input name="mqMQTTOn" value="" type="checkbox" %s> <br /><br /></TD>
  </TR>
  <TR>
    <TD valign="TOP">
      MQTT Server: </td>
    <TD>
      <input name="mqServer" type="text" minlength="7" maxlength="15" size="45" value="%s"><br><br></td>
    <TD valign="TOP">
       </td>
  </TR>
  <TR>
    <td valign="TOP">
      MQTT Port: </td>
    <TD>
    <input name="mqPort" type="number" minlength="3" maxlength="5" size="8" value="%u" required="1" pattern="[0-9]{5}"> <br><br></td>
    <TD valign="TOP">
      </td>
  </TR>
  <TR>
    <TD valign="TOP">
      Benutzername: </td>
    <TD>  
    <input name="mqUser" type="text" minlength="6" maxlength="15" size="19" value="%s"><br><br></td>
    <TD valign="TOP">
       </td>
  </TR>
  <TR>
    <TD valign="TOP">
      Passwort: </td>
    <TD>
    <input name="mqPassword" type="password" minlength="5" maxlength="60" size="35" value="xxxxxx"><br><br></td>
    <TD valign="TOP">
  </TR>
  <TR>
    <TD valign="TOP">
      MQTT Hauptpfad: </td>
  <TD>
    <input name="mqRootpath" type="text" minlength="5" maxlength="100" size="35" value="%s"><br><br></td>
    <TD valign="TOP">
  </TR>
  <TR>
    <TD VALIGN="TOP">
      MQTT secure On/Off: </TD>
    <TD VALIGN="TOP">
    <input name="mqMQTTsecureOn" value="" type="checkbox" %s> <br /><br /></TD>
  </TR>
  <TR>
    <TD valign="TOP">
      Zertifikat: </td>
    <TD>
    <input name="mqFPrint" type="password" minlength="8" maxlength="60" size="35" value="xxxxxx"><br><br></td>
    <TD valign="TOP">
  </TR>

</tbody></table>
    <br>
  <input value="Submit" type="submit">
  </form>

</body>
</html>
)rawliteral";
