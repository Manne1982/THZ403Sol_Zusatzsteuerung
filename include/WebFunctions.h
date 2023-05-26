#ifndef WebFunctions
#define WebFunctions
#include "ESPAsyncWebServer.h"

void notFound(AsyncWebServerRequest *request);
void WebserverRoot(AsyncWebServerRequest *request);
void WebserverSensors(AsyncWebServerRequest *request);
void WebserverOutput(AsyncWebServerRequest *request);
void WebserverSettings(AsyncWebServerRequest *request);
void WebserverPOST(AsyncWebServerRequest *request);

char * GetLastMessagesHTML();


#include "WebFunctions.cpp"
#endif