# Heizung_Zusatzsteuerung
To optimice the efficiency of my heat pump "Tecalor THZ 403 SOL"


2022-08-20
Normaly only Ethernet I want to add because Wifi is disabled at the night.
Because the web server does not work via Ethernet, a Wifi connection is also established for the configuration.

[MQTT]
"Root_Path" is the setting out of the HTML setting side 

MQTT commands

Root_Path/setOutput/"Name of the Output"
Value (0 - 4)
0 = Manual off
1 = Manual on
2 = Automatic mode from Heating
3 = Automatic mode with solid state relais (Input state pass thrue Output)
4 = Manual PWM mode 

Root_Path/setOutputPWM/"Name of the Output"
Value (25 - 230) (min, max values are in structure "digital_Output_current_Values Output_Values" in main.cpp
Ratio of the manually PWM mode 


Root_Path/setGeneral/WLAN_active
Value (0 - 1)
0 = switch off WLAN only possible if Ethernet port active
1 = switch on WLAN

Root_Path/setGeneral/ESP_Restart
Value (1)
1 = restart the ESP

Root_Path/setGeneral/AirSens_active
Value (0 - 1)
0 = switch off the gas quality sensor
1 = switch on sensor

MQTT values
Root_Path/Input_direct/InputPort
Value (0 - 255)
Current state of the whole input port every pin one bit

Root_Path/Input_direct/"Name of the Output"
Value (0 - 1)
0 = Input pin is off
1 = Input pin is on

Root_Path/Input_PWM_Value/"Name of the Output"
Value (0 - 255)
Ratio of the heating port. Only tor the slow PWM, fast PWM cannot detected 

Root_Path/Output_PWM_Value/"Name of the Output"
Value (25 - 230) (min, max values are in structure "digital_Output_current_Values Output_Values" in main.cpp
Ratio of the manually PWM mode 

Root_Path/TempSensors/"Name of the Sensor"
--> Value of the temperature in °C

Root_Path/General_Information/realOutput
--> Outputs as 8bit unsigned integer with the real output of all 8 ports (includes the current state of both relais)

