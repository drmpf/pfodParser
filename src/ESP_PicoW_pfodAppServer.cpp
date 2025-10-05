/*
   ESP_PicoW_pfodAppServer.cpp
   (c)2025 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This generated code may be freely used for both private and commercial use
   provided this copyright is maintained.
*/

#include <ESP_PicoW_pfodAppServer.h>
#include <pfodParser.h>
#include <pfodDebugPtr.h>

// This library needs handle_pfodMainMenu and init_pfodMainMenu to be defined in the sketch, pfodMainMenu.cpp

extern void handle_pfodMainMenu(pfodParser & parser);


// debug control
// see https://www.forward.com.au/pfod/ArduinoProgramming/Serial_IO/index.html  for how to used BufferedOutput
// to prevent your sketch being held up by Serial
// or just used debugPtr = getDebugPtr();

// #define DEBUG
static Print* debugPtr = NULL;  // local to this file

#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <WiFiServer.h>
#include <WiFiClient.h>
// pfodESPBufferedClient included in pfodParser library
#include <pfodESPBufferedClient.h>

static pfodParser parser; // always have this one // create a parser with menu version string to handle the pfod messages
static pfodESPBufferedClient bufferedClient;

static const uint8_t MAX_CLIENTS = 4; // the default MUST BE AT LEAST 1 !!
static WiFiClient clients[MAX_CLIENTS]; // hold the currently open clients
static pfodParser *parserPtrs[MAX_CLIENTS]; // hold the parsers for each client
static pfodESPBufferedClient *bufferedClientPtrs[MAX_CLIENTS]; // hold the parsers for each client

static const int portNo = 4989; // What TCP port to listen on for connections.

static WiFiServer server(portNo);
static WiFiClient client;
static bool serverStarted = false;
static bool initialized = false;

static void initParsers() {
  if (initialized) {
    return;
  }
#ifdef DEBUG
  debugPtr = getDebugPtr();
#endif

  // always have at least one
  parserPtrs[0] = &parser;
  bufferedClientPtrs[0] = &bufferedClient;

  // fill in the rest
  for (size_t i = 1; i < MAX_CLIENTS; i++) {
    parserPtrs[i] = new pfodParser();
    bufferedClientPtrs[i] = new pfodESPBufferedClient();
  }
  initialized = true;
}

void pfodApp_setVersion(const char* version) {
  initParsers();
  for (size_t i = 0; i < MAX_CLIENTS; i++) {
    parserPtrs[i]->setVersion(version);
  }
}

void start_pfodAppServer(const char* version) {
  if (serverStarted) {
    return;
  }
  pfodApp_setVersion(version);  // calls  initParsers();
  // Start the server
  server.begin();
  if (debugPtr) {
    debugPtr->println("pfodApp Server started");
    // Print the IP address
    debugPtr->print(" on ");
    debugPtr->print(WiFi.localIP());
    debugPtr->print(":");
    debugPtr->println(portNo);
  }
  serverStarted = true;
}

bool validClient(WiFiClient& client) {
  return (client.connected());
}



void handle_pfodAppServer() {
  if (!serverStarted) {
    if (debugPtr) {
      debugPtr->println("Error: pfodApp server not started.  Call start_pfodAppServer() from setup()");
    }
    return;
  }
  if (server.hasClient()) { // new connection
    if (debugPtr) {
      debugPtr->print("new client:");
    }
    bool foundSlot = false;
    size_t i = 0;
    for (; i < MAX_CLIENTS; i++) {
      if (!validClient(clients[i])) { // this space if free
        foundSlot = true;
        clients[i] = server.accept(); // was previously server.available();
        parserPtrs[i]->connect(bufferedClientPtrs[i]->connect(&(clients[i]))); // sets new io stream to read from and write to
        break;
      }
    }
    if (!foundSlot) {
      WiFiClient newClient = server.accept(); // was previously server.available(); // get any new client and close it
      newClient.stop();
      if (debugPtr) {
        debugPtr->println(" NO Slots available");
      }
    } else {
      if (debugPtr) {
        debugPtr->println(i);
      }
    }
  }
  for (size_t i = 0; i < MAX_CLIENTS; i++) {
    if (validClient(clients[i])) {
      handle_pfodMainMenu(*parserPtrs[i]);
    }
  }
}

void closeConnection_pfodAppServer(Stream * io) {
  if (!io) {
    if (debugPtr) {
      debugPtr->println("closeConnection: Connection stream NULL");
    }
    return;
  }
  if (debugPtr) {
    debugPtr->print("closeConnection:");
  }
  bool foundSlot = false;
  size_t i = 0;
  for (; i < MAX_CLIENTS; i++) {
    if ((parserPtrs[i]->getPfodAppStream() == io) && validClient(clients[i])) {
      foundSlot = true;
      break;
    }
  }
  if (foundSlot) {
    if (debugPtr) {
      debugPtr->println(i);
    }
    // found match
    parserPtrs[i]->closeConnection(); // nulls io stream
    bufferedClientPtrs[i]->stop(); // clears client reference
    clients[i].stop();
  } else {
    if (debugPtr) {
      debugPtr->println(" Connection stream NOT found");
    }
  }
}
