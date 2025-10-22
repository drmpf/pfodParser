#ifndef ESP_PICOW_PFODAPP_SERVER_H
#define ESP_PICOW_PFODAPP_SERVER_H
#include <Arduino.h>
/*   
   ESP_PicoW_pfodAppServer.h
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
 */
#if defined(ESP8266) || defined(ESP32) || defined(ARDUINO_ARCH_RP2040) && !defined(__MBED__)

void start_pfodAppServer(const char* version);
void handle_pfodAppServer();
void pfodApp_setVersion(const char* version);
void closeConnection_pfodAppServer(Stream *io);
#endif
#endif
