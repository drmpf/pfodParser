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

// Create a Print object to capture raw data
pfodCircularLineBuffer httpRawData; // default 4K buffer // for raw data

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
  httpRawData.write(c); // put in circular buffer
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

    handle_mainMenuPtr(webParser);  // capture parser output as json/text
//    if (debugPtr) {
//      debugPtr->print(" buffer used: ");
//      httpRawData.debugBufferRange(debugPtr);
//      debugPtr->println();
//    }
    httpRawData.resetForRead(); // need to do this here to set up available()
    if (httpRawData.available()) { // add raw data
      while(httpRawData.available()) {
        httpCapture.print((char)httpRawData.read());
      }
    }
    httpRawData.clear();
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




void start_pfodWebServer(const char* version, bool _serverFromLittleFS, uint32_t cacheSec, handle_mainMenuFnPtr fnPtr) {
  if (serverStarted) {
    return;
  }
  
  (void)debugPtr;  // suppress unused warning
#ifdef DEBUG
  debugPtr = getDebugPtr();
#endif
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
