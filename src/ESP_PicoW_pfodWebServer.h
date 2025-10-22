#ifndef ESP_PICOW_PFOD_WEB_SERVER_H
#define ESP_PICOW_PFOD_WEB_SERVER_H

#if defined(ESP8266) || defined(ESP32) || defined(ARDUINO_ARCH_RP2040) && !defined(__MBED__)

#include <Arduino.h>
/*   
   ESP_PicoW_pfodWebServer.h
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
 */

 // default serverFromLittleFS == false, need to use pfodWebServer to request dwg
 // otherwise if serverFromLittleFS == true, starts LittleFS to server all .html / js files
 // http://<boardIP> will return page with ip filled in for pfodWeb / pfodWebDebug
 // can still use pfodWebServer even if serverFromLittleFS == true
void start_pfodWebServer(const char* version, bool serverFromLittleFS = false);
void handle_pfodWebServer();                   // call this each loop()
void pfodWeb_setVersion(const char* version);  // this is called from start_pfodWebServer()
#endif
#endif
