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

// This library needs handle_pfodMainMenu and init_pfodMainMenu to be defined in the sketch, pfodMainMenu.cpp
extern void handle_pfodMainMenu(pfodParser& parser);

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

// Create a StringPrint object to capture JSON output
static pfodStreamString jsonCapture;

// comment out this line to force reload every time for testing
// otherwise only reloads every 24hrs x 10 => 10days
#define cacheControlStr "max-age=864000"

static const char FS_INIT_ERROR[] PROGMEM = "Invalid LittleFS. Check at least 500Kb of FS allocated for ESP32 and PiPicoW and data uploaded. For ESP8266 need >550Kb";
static const char FS_NOT_USED_ERROR[] PROGMEM = " LittleFS not serving pfodWeb files.\n\n Use pfodWebServer";

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
static void handle_pfodWebDebug();
static void printRequestArgs(Print* outPtr);
static void handleNotFound();
static bool handleFileRead(String path);
static void redirect(const char* url);
static void returnFail(String msg);
static void sendFileNotFound(const char* uri);

static bool serverFromLittleFS = false;

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
  }
  if (LittleFS.exists(path)) {
    File file = LittleFS.open(path, "r");
    if (file) {
#ifdef cacheControlStr
      server.sendHeader("Cache-Control", cacheControlStr);  // 24hrs
#endif
      if (server.streamFile(file, contentType) != file.size()) {
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
    return true;
  } else {
    if (debugPtr) {
      debugPtr->println("File does not exist");
    }
  }
  return false;
}


static void handleIndex() {
  // else
  if (handleFileRead("/index.html")) {
    return;
  }
  sendFileNotFound("/index.html");
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
// Note: _debug is ignored for pfod { } cmds,
// it is only check for non-commands to return either pfodWeb.html or pfodWebDebug.html
// for pfodCmds the path is ALWASY /pfodWeb?cmd=...
static void handle_pfodWeb_page(bool _debug) {
  if (debugPtr) {
    debugPtr->print("Handling request with ");
    debugPtr->println(_debug ? "debug true" : "debug false");
  }
  // Add CORS headers to all responses
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "x-requested-with, Content-Type");


  bool isAjaxJsonRequest = false;
  String cmdStr = server.arg("cmd");
  cmdStr.trim();
  if (debugPtr) {
    debugPtr->print("cmdStr:");
    debugPtr->println(cmdStr);
  }
  if (!cmdStr.isEmpty()) {
    isAjaxJsonRequest = true;
    if (debugPtr) {
      debugPtr->println("AJAX JSON request detected - returning JSON data");
    }
  }

  if (isAjaxJsonRequest) {
    jsonCapture.clear();
    jsonCapture.splitCmds = false;  // don't interfer with | and }
    jsonCapture.print(cmdStr);
    if (debugPtr) {
      debugPtr->print(" parsing msg: '");
      debugPtr->print(jsonCapture);
      debugPtr->println("'");
    }

    // add the json header after the inputMsg.
    // The parser will stop reading at the } terminating the input msg leaving this in the Stream_String
    jsonCapture.println();
    jsonCapture.print('{');
    jsonCapture.print('"');
    jsonCapture.print("cmd");
    jsonCapture.print('"');
    jsonCapture.print(':');
    jsonCapture.println('[');
    jsonCapture.print('"');

    jsonCapture.splitCmds = true;
    handle_pfodMainMenu(webParser);  // capture parser output as json
    jsonCapture.splitCmds = false;

    // close the cmd array
    jsonCapture.println('"');
    jsonCapture.print("]}");

    if (jsonCapture.length() > 0) {
      if (debugPtr) {
        debugPtr->print(" Returning JSON response:- ");
        debugPtr->println(jsonCapture);  // print jsonCaptre
      }
      // Send JSON response with proper content type
      server.send(200, "application/json", (const String&)jsonCapture);
      jsonCapture.clear();
    }

  } else {  // Serve pfodWeb.html page for non-cmd requests
    
    bool has_targetIP = server.hasArg("targetIP");
    if (has_targetIP) {
      if (_debug) {
        if (debugPtr) {
          debugPtr->println("Browser request with arg detected - serving pfodWebDebug as html");
        }
        handleFileRead("/pfodWebDebug.html");
      } else {
        if (debugPtr) {
          debugPtr->println("Browser request with arg detected - serving pfodWeb as html");
        }
        handleFileRead("/pfodWeb.html");
      }
    } else {
      //redirect
      String targetIP = "?targetIP=";
      targetIP += (WiFi.localIP().toString());
      if (debugPtr) {
        debugPtr->print("Redirecting request to ");
        debugPtr->println(targetIP);
      }
      if (_debug) {
        String url = "/pfodWebDebug.html" + targetIP;
        redirect(url.c_str());
      } else {
        String url = "/pfodWeb.html" + targetIP;
        redirect(url.c_str());
      }
    }
  }
}


static void handle_pfodWebDebug() {
  if (debugPtr) {
    debugPtr->println("Handling /pfodWebDebug request");
    printRequestArgs(debugPtr);
  }
  handle_pfodWeb_page(true);
}

//NOTE esp32 server automatically applies urlDecode to args before using names/values
static void handle_pfodWeb() {
  if (debugPtr) {
    debugPtr->println("Handling /pfodWeb request");
    printRequestArgs(debugPtr);
  }
  handle_pfodWeb_page(false);  // adds CORS
}

void start_pfodWebServer(const char* version, bool _serverFromLittleFS) {
  if (serverStarted) {
    return;
  }
  (void)debugPtr;  // suppress unused warning
#ifdef DEBUG
  debugPtr = getDebugPtr();
#endif
  pfodWeb_setVersion(version);
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
      debugPtr->println(" !! Use pfodWebServer");
    }
  }
  if (serverFromLittleFS) {
    server.on("/", HTTP_GET, handleIndex);
    server.on("/index.html", handleIndex);  // both GET and POST, to handle redirect after set time
  }

  server.on("/pfodWeb", HTTP_OPTIONS, handleCORS);
  server.on("/pfodWeb", HTTP_GET, handle_pfodWeb);
  server.on("/pfodWebDebug", HTTP_OPTIONS, handleCORS);
  server.on("/pfodWebDebug", HTTP_GET, handle_pfodWebDebug);
  server.on("/pfodWeb.html", HTTP_OPTIONS, handleCORS);
  server.on("/pfodWeb.html", HTTP_GET, handle_pfodWeb);
  server.on("/pfodWebDebug.html", HTTP_OPTIONS, handleCORS);
  server.on("/pfodWebDebug.html", HTTP_GET, handle_pfodWebDebug);

  // Handle 404s with CORS
  server.onNotFound(handleNotFound);
  (void)(redirect);  // to suppress compiler warning only
  server.begin();
  if (debugPtr) {
    debugPtr->println("pfodWeb server started");
  }
  serverStarted = true;
  jsonCapture.reserve(4096);        // to minimize memory fragmation
  webParser.connect(&jsonCapture);  // connect parser to capture output
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