#ifndef ProjectFunctions
#define ProjectFunctions
#include <Arduino.h>
#include <NTPClient.h>


enum {touchUp_wsgn = 33, touchDown_gn = 32, touchRight_wsbr = 15, touchLeft_br = 4, touchF1_bl = 13, touchF2_wsbl = 12, touchF3_or = 14, touchF4_wsor = 27, RGB_Red = 22, RGB_Green = 16, RGB_Blue = 17, Display_Beleuchtung = 21};


//Functions for saving settings
void EinstSpeichern();
bool EinstLaden();
char ResetVarLesen();
void ResetVarSpeichern(char Count);
void NetworkInit(bool OnlyWiFi = false);


#include "ProjectFunctions.cpp"
#endif