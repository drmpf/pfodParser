#ifndef ESP_PICOW_PFOD_WEB_SERVER_H
#define ESP_PICOW_PFOD_WEB_SERVER_H

#if defined(ESP8266) || defined(ESP32) || defined(ARDUINO_ARCH_RP2040) && !defined(__MBED__)

#include <Arduino.h>
#include <pfodParser.h>
#ifdef ESP8266
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif
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
 // rawDataBufferSize -- size (bytes) of the raw-data relay (getRawDataWriter()/
 // writeHttpRawData()) each polling client reads from -- see that relay's own
 // comment in the .cpp for what it's for. Default 4096 matches this
 // library's original single-client size; pass a larger value (e.g. 16384)
 // if a sketch's own raw-data producer (e.g. a rolling debug-log dump) needs
 // more backlog held for a slow-polling client to still catch up on.
void start_pfodWebServer(const char* version, bool serverFromLittleFS = false, uint32_t cacheSec = 0,  handle_mainMenuFnPtr fnPtr=NULL, uint32_t rawDataBufferSize = 4096 );
void handle_pfodWebServer();                   // call this each loop()
void pfodWeb_setVersion(const char* version);  // this is called from start_pfodWebServer()
Print& getRawDataWriter();  // this is for use when there is not pfodApp server included

// Resets the CURRENT web-poll client's own raw-data read cursor back to
// PFOD_CURSOR_START -- i.e. "next time this same client's cursor is used to
// drain the raw-data relay, start from the oldest data still available,
// not from wherever this client had already read up to". Meant to be
// called from a menu/dwg command handler (a pfodParser callback running
// INSIDE handle_pfodWebServer()'s own call, e.g. a "show me the last N
// bytes of my rolling log again" button) -- has no effect if called from
// anywhere else (there is no "current client" outside that call), and no
// effect at all if the sketch only uses the pfodApp TCP server (raw data
// there is pushed live to every connected client, not replayed on demand
// per client -- see ESP_PicoW_pfodAppServer.cpp).
void resetCurrentRawDataCursor();

// Access to the underlying web server instance, so a sketch can register its
// own additional server.on(...) handlers (e.g. for its own dynamically
// generated pages) on the same server/port. Call after start_pfodWebServer().
#ifdef ESP8266
ESP8266WebServer& getPfodHttpServer();
#else
WebServer& getPfodHttpServer();
#endif

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
