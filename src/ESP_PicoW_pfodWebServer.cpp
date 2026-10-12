/*
   ESP_Pico_pfodWebServer.cpp
   (c)2025 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This generated code may be freely used for both private and commercial use
   provided this copyright is maintained.

*/
#if defined(ESP8266) || defined(ESP32) || defined(ARDUINO_ARCH_RP2040) && !defined(__MBED__)

#include "ESP_PicoW_pfodWebServer.h"
#include <pfodParser.h>
#include <pfodDebugPtr.h>
#include "pfodCircularLineBuffer.h"

// debug control
// see https://www.forward.com.au/pfod/ArduinoProgramming/Serial_IO/index.html  for how to used BufferedOutput
// to prevent your sketch being held up by Serial
// or just used debugPtr = getDebugPtr()

//#define DEBUG
static Print* debugPtr = NULL;  // local to this file

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#else
#include <WiFi.h>
#include <WebServer.h>
#endif
#ifdef ESP32
#include <NetworkClient.h>
#endif
#include <WiFiClient.h>

#include <ESP_PicoW_LittleFSsupport.h>
#include <pfodStreamString.h>  // string class to capture parser json output
#include "pfodRawDataWriter.h"

static char cacheControlStr[25];// = "max-age=..." uint32_t max 10digits
//max-age=864000 only reloads every 24hrs x 10 => 10days

// Create a StringPrint object to capture JSON/text output
static pfodStreamString httpCapture;

// The raw-data relay -- getRawDataWriter()/writeHttpRawData() write into
// this; handle_pfodWeb() drains whatever's newly available (per the
// requesting client's OWN read cursor -- see rawDataClients[] below) into
// each poll's http reply. A pointer, not a plain global object, since its
// size is only known once start_pfodWebServer()'s own rawDataBufferSize
// argument is read -- a C++ global object's constructor args have to be
// fixed at compile time, which a runtime function argument can't feed.
// Allocated once, in start_pfodWebServer(), and never freed (this server
// runs for the lifetime of the sketch).
static pfodCircularLineBuffer *httpRawDataPtr = NULL;

// Guards httpRawDataPtr's own read/write access (writeHttpRawData() below,
// and the per-client drain in handle_pfodWeb()) -- added 2026-09-26,
// alongside the multi-client cursor support below. Needed because a
// sketch's own raw-data producer (e.g. a rolling debug-log capture) may
// call writeHttpRawData()/getRawDataWriter() from a DIFFERENT thread of
// execution than the one running handle_pfodWebServer() (this file's own
// server.handleClient() call, always from whichever task/core calls that)
// -- pfodCircularLineBuffer has no locking of its own (by design -- see
// its own header comment), so two genuinely concurrent accesses to the
// same instance is a real, not hypothetical, corruption risk.
//
// ESP32: a real FreeRTOS mutex -- a background FreeRTOS task calling
// writeHttpRawData() can genuinely preempt/run concurrently with
// handle_pfodWebServer()'s own task on the other core.
//
// RP2040 (arduino-pico core): also a real lock, via pico/mutex.h's
// mutex_t -- a plain Pico SDK primitive, available with or without
// FreeRTOS. Confirmed 2026-09-26 by reading that core's own
// cores/rp2040/main.cpp: it unconditionally calls
// multicore_launch_core1() whenever a sketch defines setup1()/loop1(),
// running that loop in genuine hardware parallel with core 0's loop() --
// with or without the optional __FREERTOS build flag. So there is no
// "single cooperative thread" guarantee on this platform the way there
// is on ESP8266; a no-op lock here would be a real race for any RP2040
// sketch that writes raw data from its own core1 while core 0 runs
// handle_pfodWebServer() (this corrects an earlier, wrong assumption in
// this same comment that non-FreeRTOS RP2040 was single-threaded like
// ESP8266).
//
// ESP8266 / any other plain (non-FreeRTOS, non-RP2040) Arduino core:
// genuinely single-threaded/cooperative -- no task or core can interrupt
// a write -- so locking is a real no-op there, not an unavailable API.
#ifdef ESP32
static SemaphoreHandle_t rawDataMutex = NULL;
static void lockRawData() {
  if (rawDataMutex == NULL) {
    rawDataMutex = xSemaphoreCreateMutex();
  }
  xSemaphoreTake(rawDataMutex, portMAX_DELAY);
}
static void unlockRawData() {
  if (rawDataMutex) {
    xSemaphoreGive(rawDataMutex);
  }
}
#elif defined(ARDUINO_ARCH_RP2040)
#include <pico/mutex.h>
static mutex_t rawDataMutex;
static bool rawDataMutexInitialized = false;
static void lockRawData() {
  if (!rawDataMutexInitialized) {
    mutex_init(&rawDataMutex);
    rawDataMutexInitialized = true;
  }
  mutex_enter_blocking(&rawDataMutex);
}
static void unlockRawData() {
  if (rawDataMutexInitialized) {
    mutex_exit(&rawDataMutex);
  }
}
#else
static void lockRawData() {}
static void unlockRawData() {}
#endif

// Per-client (by IP) read cursor into httpRawDataPtr -- added 2026-09-26,
// fixing a real bug: with a single shared read position (the old
// resetForRead()/clear()-every-poll design), whichever client's poll
// happened to run first drained and cleared the WHOLE relay for itself,
// leaving any other simultaneously-polling client (a different browser
// tab, or a second device) with nothing -- raw data effectively got
// arbitrarily split between clients instead of each seeing the full
// stream. Each client now gets its own pfodReadCursor (pfodCircularLineBuffer.h),
// resumed and saved back on every poll, same idea as
// pfodCircularLineBuffer's own cursor API was built for. A brand new
// client (never seen before) starts at PFOD_CURSOR_START, so a first poll
// still sees "whatever's currently queued", matching the single-client
// behaviour this replaces.
struct RawDataClientCursor {
  IPAddress ip;
  bool used = false;
  pfodReadCursor cursor = PFOD_CURSOR_START;
  unsigned long lastSeenMs = 0;
};
static const uint8_t MAX_RAW_DATA_CLIENTS = 4;
static RawDataClientCursor rawDataClients[MAX_RAW_DATA_CLIENTS];
// A slot idle this long is assumed abandoned (tab closed, device gone)
// and is reclaimed for a new client rather than permanently exhausting
// the small fixed table.
static const unsigned long RAW_DATA_CLIENT_TIMEOUT_MS = 5UL * 60UL * 1000UL;

// Upper bound on how many raw-data bytes handle_pfodWeb() drains for ONE
// client in ONE response -- see that function's own comment for why: a
// client whose cursor is far behind (e.g. a large rawDataBufferSize
// backlog just replayed via resetCurrentRawDataCursor()) must not have
// all of it appended into httpCapture (a String, only pre-reserve()'d for
// the ordinary small menu/json case) in a single go -- confirmed, in this
// project's own history, as a real memory-fragmentation risk from the
// resulting near-one-reallocation-per-byte growth, not a hypothetical
// one. Left at less than half of httpCapture's own 4096-byte reservation
// so there's still comfortable room for the menu/json content sharing
// the same response.
static const size_t MAX_RAW_DATA_PER_RESPONSE = 2048;

// Index into rawDataClients[] for whichever client handle_pfodWeb() is
// CURRENTLY servicing -- valid only for the duration of one such call
// (set right before handle_mainMenuPtr() runs, cleared after the raw-data
// drain that follows it). Lets resetCurrentRawDataCursor() below (called
// from a menu/dwg command handler running INSIDE that same call, e.g. a
// "replay my rolling log from the start" button) act on the right
// client's own slot without that handler needing to know about IP
// addresses or this table at all.
static int currentRawDataClientIdx = -1;

// Finds this IP's existing slot, or claims one for it (an empty slot if
// any is free, otherwise the least-recently-seen slot -- which, if it's
// still actually in use by some other slow client, just means THAT
// client's own next poll starts fresh from PFOD_CURSOR_START too, the
// same graceful "cursor's data has been evicted" fallback
// setReadCursor() already provides, not a crash or wrong data).
static int findOrClaimRawDataClientSlot(const IPAddress &ip) {
  unsigned long nowMs = millis();
  int freeIdx = -1;
  int oldestIdx = 0;
  unsigned long oldestSeenMs = 0xFFFFFFFFUL;
  for (uint8_t i = 0; i < MAX_RAW_DATA_CLIENTS; i++) {
    if (rawDataClients[i].used && rawDataClients[i].ip == ip) {
      return i;
    }
    if (rawDataClients[i].used && (nowMs - rawDataClients[i].lastSeenMs) > RAW_DATA_CLIENT_TIMEOUT_MS) {
      rawDataClients[i].used = false; // stale -- reclaim outright
    }
    if (!rawDataClients[i].used && freeIdx < 0) {
      freeIdx = i;
    }
    if (rawDataClients[i].lastSeenMs < oldestSeenMs) {
      oldestSeenMs = rawDataClients[i].lastSeenMs;
      oldestIdx = i;
    }
  }
  int idx = (freeIdx >= 0) ? freeIdx : oldestIdx;
  rawDataClients[idx].ip = ip;
  rawDataClients[idx].used = true;
  rawDataClients[idx].cursor = PFOD_CURSOR_START;
  rawDataClients[idx].lastSeenMs = nowMs;
  return idx;
}

void resetCurrentRawDataCursor() {
  if (currentRawDataClientIdx >= 0) {
    rawDataClients[currentRawDataClientIdx].cursor = PFOD_CURSOR_START;
  }
}

void handle_pfodMainMenu(pfodParser& parser)  __attribute__((weak));
void handle_pfodMainMenu(pfodParser& parser) {
  (void)(parser); // to suppress warning about unused var
  httpCapture.clear(); // no parser so just clear any cmd sent (by mistake?)
  // this weak method is here for charting via HTTP when there is not main menu
  // and no app server
}

// Called around the actual streaming of a static LittleFS file (see
// handleFileRead). Weak no-op defaults; a sketch may override these to
// quiet other radio/RAM users (e.g. a BLE scan) for the duration of
// the transfer. Run in the web-server (loop) context - keep short.
void pfodWebFileServeStart() __attribute__((weak));
void pfodWebFileServeStart() {
}
void pfodWebFileServeEnd() __attribute__((weak));
void pfodWebFileServeEnd() {
}

// comment out this line to force reload every time for testing

static const char FS_INIT_ERROR[] PROGMEM = "Invalid LittleFS. Check at least 1024Kb of FS allocated for ESP32, ESP8266 and PiPicoW and data uploaded.";
static const char FS_NOT_USED_ERROR[] PROGMEM = " LittleFS not serving pfodWeb files.\n\n Use pfodWeb";

static bool serverStarted = false;

#ifdef ESP8266
static ESP8266WebServer server(80);
#else
static WebServer server(80);
#endif

static pfodParser webParser;
static String getContentType(String filename);

static void handleIndex();
static void handle_pfodWeb();
static void printRequestArgs(Print* outPtr);
static void handleNotFound();
static bool handleFileRead(String path);
static void redirect(const char* url);
static void returnFail(String msg);
static void sendFileNotFound(const char* uri);
static handle_mainMenuFnPtr handle_mainMenuPtr;

static bool serverFromLittleFS = false;

// this one used in only the web server running and not the app server
static pfodRawDataWriter rawDataWriterWeb(NULL,0);

Print& getRawDataWriter() __attribute__((weak));
Print& getRawDataWriter() {
  return rawDataWriterWeb;
}

void writeHttpRawData(uint8_t c) {
  // NULL until start_pfodWebServer() has run -- e.g. very early boot-time
  // debug output logged before setup() gets to calling it. Silently
  // dropped rather than crashing; nothing can read it back before the
  // server exists to serve it anyway.
  if (!httpRawDataPtr) {
    return;
  }
  lockRawData();
  httpRawDataPtr->write(c); // put in circular buffer
  unlockRawData();
}

void pfodWeb_setVersion(const char* version) {
  webParser.setVersion(version);
}

/*
   Read the given file from the filesystem and stream it back to the client
   Will try and serve .gz if file not found.
*/
static bool handleFileRead(String path) {
  if (!serverFromLittleFS) {
    returnFail(FPSTR(FS_NOT_USED_ERROR));
    return true;
  }

  if (debugPtr) {
    debugPtr->println(String("handleFileRead: ") + path);
  }

  String contentType;
  contentType = getContentType(path);
  if (debugPtr) {
    debugPtr->print("contentType: ");
    debugPtr->println(contentType);
  }

  if (!LittleFS.exists(path)) {
    // try .gz extension
    path += ".gz";
    // set Content-Encoding: gzip
    //server.sendHeader("Content-Encoding", "gzip");  
  }
  if (LittleFS.exists(path)) {
    // Bracket the whole open/stream/close so a sketch can free the
    // radio/RAM for the transfer (e.g. pause BLE scanning) and restore
    // it afterwards. End is called on every post-open path (including
    // the open-failure branch) so Start/End stay balanced.
    pfodWebFileServeStart();
    File file = LittleFS.open(path, "r");
    if (file) {
      if (cacheControlStr[0]) {                               // not leading '\0';
        server.sendHeader("Cache-Control", cacheControlStr);
        if (debugPtr) {
          debugPtr->println(cacheControlStr);
        }
      }
      if ((server.streamFile(file, contentType) != file.size()) &&  (!path.startsWith("/favicon.ico"))) {
        if (debugPtr) {
          debugPtr->println("Sent less data than expected!");
        }
      }
      file.close();
    } else {
      if (debugPtr) {
        debugPtr->println("Failed to open file");
      }
    }
    pfodWebFileServeEnd();
    return true;
  } else {
    if (debugPtr) {
      debugPtr->println("File does not exist");
    }
  }
  return false;
}


static void handleIndex() {
  //redirect and autoConnect
  String targetIP = "?autoConnect&targetIP=";
  targetIP += (WiFi.localIP().toString());
  if (debugPtr) {
  debugPtr->print("Redirecting request to ");
    debugPtr->println(targetIP);
  }
  String url = "/pfodWeb.html" + targetIP;
  redirect(url.c_str());
}

static void handleCORS() {
  // Handle preflight OPTIONS requests
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "x-requested-with, Content-Type");
  server.sendHeader("Access-Control-Max-Age", "86400");
  server.send(200, "text/plain", "");
}

//NOTE esp32 server automatically applies urlDecode to args before using names/values
static void handle_pfodWeb() {
  if (debugPtr) {
    debugPtr->println("Handling /pfodWeb request");
    printRequestArgs(debugPtr);
  }
  
  // Add CORS headers to all responses
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "x-requested-with, Content-Type");


  bool is_pfodWebRequest = false;
  String cmdStr;
  if (server.hasArg("cmd")) {
    if (debugPtr) {
      debugPtr->println("pfodWeb pfod cmd request detected");
    }
    is_pfodWebRequest = true;
    cmdStr = server.arg("cmd");
    cmdStr.trim();
    if (debugPtr) {
      debugPtr->print("cmdStr:");
      debugPtr->println(cmdStr);
    }
  }
  if (is_pfodWebRequest) {
    httpCapture.clear();
    httpCapture.print(cmdStr);
    if (debugPtr) {
      debugPtr->print(" parsing msg: '");
      debugPtr->print(httpCapture);
      debugPtr->println("'");
    }

    // Resolved BEFORE handle_mainMenuPtr() runs, not after -- a command
    // handler it calls into (e.g. a "replay my rolling log from the
    // start" button) may call resetCurrentRawDataCursor(), which needs
    // this client's slot to already be the one about to be used for the
    // drain just below, not a stale/absent one.
    currentRawDataClientIdx = findOrClaimRawDataClientSlot(server.client().remoteIP());
    rawDataClients[currentRawDataClientIdx].lastSeenMs = millis();

    handle_mainMenuPtr(webParser);  // capture parser output as json/text -- may also call resetCurrentRawDataCursor()

    // Per-client cursor, not resetForRead()/clear() -- see rawDataClients[]'s
    // own comment for the multi-client bug this fixes. Never .clear()s the
    // relay: another client may not have caught up to bytes still sitting
    // in it, and clearing here would silently drop them for that client
    // too, exactly the bug this replaces.
    //
    // Capped at MAX_RAW_DATA_PER_RESPONSE per poll, NOT drained to
    // exhaustion in one go -- a client whose cursor is far behind (e.g.
    // right after resetCurrentRawDataCursor() rewinds it to replay a large
    // backlog) would otherwise dump everything currently available
    // straight into httpCapture in a single response. httpCapture is a
    // String pre-reserve()'d for the ordinary (small) menu/json case; a
    // large uncapped append past that reservation means a real
    // reallocation on close to every byte -- confirmed, in this exact
    // codebase's own history, as a real fragmentation risk, not a
    // hypothetical one. What doesn't fit this poll stays in httpRawDataPtr
    // for THIS client's own cursor to pick up on their next one -- same
    // "spread a large backlog over several polls" idea the old pump used,
    // just per-client instead of via one shared pump.
    if (httpRawDataPtr) {
      lockRawData();
      httpRawDataPtr->setReadCursor(rawDataClients[currentRawDataClientIdx].cursor);
      size_t sent = 0;
      while (sent < MAX_RAW_DATA_PER_RESPONSE && httpRawDataPtr->available()) {
        httpCapture.print((char)httpRawDataPtr->read());
        sent++;
      }
      rawDataClients[currentRawDataClientIdx].cursor = httpRawDataPtr->getReadCursor();
      unlockRawData();
    }
    currentRawDataClientIdx = -1; // done servicing this client for this request
    if (httpCapture.length() == 0) {
      httpCapture+=" ";
    }
    if (httpCapture.length() > 0) {
      if (debugPtr) {
        debugPtr->print(" Returning response:- ");
        debugPtr->println(httpCapture);  
      }
      // Send JSON response with proper content type
      server.send(200, "text/plain", (const String&)httpCapture);
      httpCapture.clear();
    }

  } else {  // Serve pfodWeb.html page for non-cmd requests
    handleFileRead("/pfodWeb.html");
  }
}




void start_pfodWebServer(const char* version, bool _serverFromLittleFS, uint32_t cacheSec, handle_mainMenuFnPtr fnPtr, uint32_t rawDataBufferSize) {
  if (serverStarted) {
    return;
  }

  (void)debugPtr;  // suppress unused warning
#ifdef DEBUG
  debugPtr = getDebugPtr();
#endif
  // Sized from the caller's own argument, not fixed at compile time --
  // see httpRawDataPtr's own comment for why this has to be a pointer,
  // allocated here, rather than a plain global object.
  httpRawDataPtr = new pfodCircularLineBuffer(rawDataBufferSize);
  pfodWeb_setVersion(version);
  if (fnPtr) {
    handle_mainMenuPtr = fnPtr;
  } else {
    handle_mainMenuPtr = handle_pfodMainMenu; // weak
  }
  serverFromLittleFS = _serverFromLittleFS;
  if (serverFromLittleFS) {  // start LittleFS to server pages and .js
    if (!initializeFS()) {
      if (debugPtr) {
        debugPtr->println("LittleFS failed to start.");
      }
    } else {
      if (debugPtr) {
        showLittleFS_size(debugPtr);
        debugPtr->println(" LittleFS File list:");
        listDir("/", debugPtr);
      } else {
      }
    }
  } else {
    if (debugPtr) {
      debugPtr->println(" !! LittleFS not used to server pfodWeb files");
      debugPtr->println(" !! Use pfodWeb");
    }
  }
  cacheControlStr[0] = '\0';  // clear it
  if (cacheSec > 0) {
    String str = "max-age=";
    str += cacheSec;
    strcpy(cacheControlStr, str.c_str());  // have check length
    if (debugPtr) {
      debugPtr->print(" cache time set to ");  debugPtr->println(cacheControlStr);
    }
  } else {
    if (debugPtr) {
      debugPtr->print(" No cache time specified.");  debugPtr->println();
    }
  }    
  if (serverFromLittleFS) {
    server.on("/", HTTP_GET, handleIndex);
    server.on("/index.html", handleIndex);  // both GET and POST, to handle redirect after set time
  }

  server.on("/pfodWeb", HTTP_OPTIONS, handleCORS);
  server.on("/pfodWeb", HTTP_GET, handle_pfodWeb);
  server.on("/pfodWeb.html", HTTP_OPTIONS, handleCORS);
  server.on("/pfodWeb.html", HTTP_GET, handle_pfodWeb);

  // Handle 404s with CORS
  server.onNotFound(handleNotFound);
  (void)(redirect);  // to suppress compiler warning only
  server.begin();
  if (debugPtr) {
    debugPtr->println("pfodWeb server started");
  }
  serverStarted = true;
  httpCapture.reserve(4096);        // to minimize memory fragmation
  webParser.connect(&httpCapture);  // connect parser to capture output
}

void handle_pfodWebServer() {
  if (!serverStarted) {
    if (debugPtr) {
      debugPtr->println("Error: pfodWeb server not started.  Call start_pfodWebServer() from setup()");
    }
    return;
  }
  server.handleClient();
}

#ifdef ESP8266
ESP8266WebServer& getPfodHttpServer() {
  return server;
}
#else
WebServer& getPfodHttpServer() {
  return server;
}
#endif

static void redirect(const char* url) {
  if (debugPtr) {
    debugPtr->print("Redirect to: ");
    debugPtr->println(url);
  }
  server.sendHeader("Location", url);
  server.send(307);
}
static void returnFail(String msg) {
  msg += "\r\n";
  if (debugPtr) {
    debugPtr->print("Return Fail with msg: ");
    debugPtr->println(msg);
  }
  server.send(500, "text/plain", msg);
}
static void printRequestArgs(Print* outPtr) {
  if (!outPtr) {
    return;
  }
  outPtr->print("URI: ");
  outPtr->print(server.uri());
  outPtr->print("   Method: ");
  outPtr->println((server.method() == HTTP_GET) ? "GET" : "POST");
  outPtr->print(" Arguments: ");
  outPtr->println(server.args());
  for (uint8_t i = 0; i < server.args(); i++) {
    outPtr->print(" NAME:");
    outPtr->print(server.argName(i));
    outPtr->print("   VALUE:");
    outPtr->println(server.arg(i));
  }
}
static void handleNotFound() {
  // try just serving it
  if (handleFileRead(server.uri())) {
    return;
  }
  sendFileNotFound(server.uri().c_str());
}

static void sendFileNotFound(const char* uri) {
  server.sendHeader("Access-Control-Allow-Origin", "*");

  String message = "File Not Found. Upload data into file system.\n\n";
  message += "URI: ";
  message += uri;
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}
// call this with original ending
// then if file is .gz was used to find file
// set Content-Encoding: gzip
static String getContentType(String filename) {
  if (server.hasArg("download")) {
    return "application/octet-stream";
  } else if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".json")) {
    return "application/json";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".woff2")) {
    return "font/woff2";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }
  return "text/plain";
}

#endif
