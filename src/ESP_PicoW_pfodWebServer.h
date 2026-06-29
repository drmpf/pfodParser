#ifndef ESP_PICOW_PFOD_WEB_SERVER_H
#define ESP_PICOW_PFOD_WEB_SERVER_H

#if defined(ESP8266) || defined(ESP32) || defined(ARDUINO_ARCH_RP2040) && !defined(__MBED__)

#include <Arduino.h>
#include <pfodParser.h>
/*   
   ESP_PicoW_pfodWebServer.h
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
 */
 
typedef  void (*handle_mainMenuFnPtr)(pfodParser &parser);

 // default serverFromLittleFS == false, need to use pfodWebServer to request dwg
 // otherwise if serverFromLittleFS == true, starts LittleFS to server all .html / js files
 // http://<boardIP> will return page with ip filled in for pfodWeb / pfodWebDebug
 // can still use pfodWebServer even if serverFromLittleFS == true
void start_pfodWebServer(const char* version, bool serverFromLittleFS = false, uint32_t cacheSec = 0,  handle_mainMenuFnPtr fnPtr=NULL );
void handle_pfodWebServer();                   // call this each loop()
void pfodWeb_setVersion(const char* version);  // this is called from start_pfodWebServer()
Print& getRawDataWriter();  // this is for use when there is not pfodApp server included

// Optional hooks, called immediately before / after the server streams
// a static file from LittleFS (the heavy, radio- and RAM-bound part of
// serving the pfodWeb files). Default implementations (weak, in the
// .cpp) do nothing. A sketch can provide its own strong definitions -
// e.g. to pause BLE scanning on a single-radio chip so WiFi gets the
// radio while the ~400 KB of pfodWeb files are served, then resume.
// These run in the web-server (loop) context, so keep them short.
void pfodWebFileServeStart();
void pfodWebFileServeEnd();
#endif
#endif
