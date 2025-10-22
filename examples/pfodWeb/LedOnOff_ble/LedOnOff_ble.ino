/*
   LedOnOff_ble.ino
   (c)2025 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This generated code may be freely used for both private and commercial use
   provided this copyright is maintained.

*/

/**
  This example sketch compiles for ESP32, ESP32C3.  Should also work for other ESP32 variants but has not been tested on all of them
  The project has been tested using Arduino IDE V2.3.6, ESP32 board support V3.3.2
  NOTE: Choose memory setting with at least 2M APP  E.g NO OTA (2M APP/2M SPIFFS)
  
  See the tutorial at https://www.forward.com.au/pfod/pfodWeb/index.html

  Setup Notes:
  =============
  Install the latest pfodParser and SafeString libraries from the library manager
  Use a Chrome or Edge browser (>V141)
  Load this sketch
  NOTE: Choose memory setting with at least 2M APP  E.g NO OTA (2M APP/2M SPIFFS)
  From the pfodParse library, in sub-directory pfodWeb, open pfodWeb.html in a Chrome or Edge browser (>V141)
  and selected BLE connection, click Connect and Pair with pfod_LedOnOff
  
  For connecting via Android pfodApp, setup a connection in pfodApp. See https://www.forward.com.au/pfod/Android_pfodApp/pfodAppForAndroidGettingStarted.pdf

*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// download the libraries from http://www.forward.com.au/pfod/pfodParserLibraries/index.html
// pfodParser.zip V3.65+ contains pfodParser, pfodSecurity, pfodDelay, pfodBLEBufferedSerial, pfodSMS and pfodRadio
#include <pfodParser.h>

#include <pfodBLEBufferedSerial.h>  // used to prevent flooding bluetooth sends

#include <pfodDebugPtr.h>
#include "pfodMainMenu.h"

// =========== pfodBLESerial definitions
const char* localName = "pfod_LedOnOff";  // <<<<<<  change this string to customize the adverised name of your board
class pfodBLESerial : public Stream, public BLEServerCallbacks, public BLECharacteristicCallbacks {
public:
  pfodBLESerial();
  void begin();
  void poll();
  size_t write(uint8_t);
  size_t write(const uint8_t*, size_t);
  int read();
  int available();
  void flush();
  int peek();
  void close();
  bool isConnected();
  static void addReceiveBytes(const uint8_t* bytes, size_t len);
  const static uint8_t pfodEOF[1];
  const static char* pfodCloseConnection;
  volatile static bool connected;
  void onConnect(BLEServer* serverPtr);
  void onDisconnect(BLEServer* serverPtr);
  void onWrite(BLECharacteristic* pCharacteristic);

private:
  static const int BLE_MAX_LENGTH = 20;
  static const int BLE_RX_MAX_LENGTH = 256;
  static volatile size_t rxHead;
  static volatile size_t rxTail;
  volatile static uint8_t rxBuffer[BLE_RX_MAX_LENGTH];
  size_t txIdx;
  volatile uint8_t txBuffer[BLE_MAX_LENGTH];
};
volatile size_t pfodBLESerial::rxHead = 0;
volatile size_t pfodBLESerial::rxTail = 0;
volatile uint8_t pfodBLESerial::rxBuffer[BLE_RX_MAX_LENGTH];
const uint8_t pfodBLESerial::pfodEOF[1] = { (uint8_t)-1 };
const char* pfodBLESerial::pfodCloseConnection = "{!}";
volatile bool pfodBLESerial::connected = false;

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"  // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
// NOTE: the reverse characteristics do no work
BLEServer* serverPtr = NULL;
BLECharacteristic* characteristicTXPtr;
// =========== end pfodBLESerial definitions

pfodParser parser("V1");                  // create a parser with menu version string to handle the pfod messages
// create a parser to handle the pfod messages
pfodBLESerial bleSerial;                  // create a BLE serial connection
pfodBLEBufferedSerial bleBufferedSerial;  // create a BLE serial connection

unsigned long plot_mSOffset = 0;  // set by {@} response
bool clearPlot = false;           // set by the {@} response code

// give the board pins names, if you change the pin number here you will change the pin controlled
int cmd_A_var;            // name the variable for 'Led'
const int cmd_A_pin = 5;  // name the output 5 for 'Led'

// initialize digital pin for the LED
const int ledPin = LED_BUILTIN;
bool ledIsOn = false;
#ifdef ESP8266
bool highIsOn = false;
#else
bool highIsOn = true;
#endif

// these fns call from the button code
void turnLedOff() {
  if (highIsOn) {
    digitalWrite(ledPin, LOW);
  } else {
    digitalWrite(ledPin, HIGH);
  }
  ledIsOn = false;
}

void turnLedOn() {
  if (highIsOn) {
    digitalWrite(ledPin, HIGH);
  } else {
    digitalWrite(ledPin, LOW);
  }
  ledIsOn = true;
}

bool isLedOn() {
  return ledIsOn;
}

static Stream* debugPtr = NULL;


const char version[] = "V1";  // need non blank version for auto refresh


// the setup routine runs once on reset:
void setup() {
  cmd_A_var = 0;
  // ledcAttachPin(cmd_A_pin,cmd_A_channel); // assign pin to channel
  // ledcSetup(cmd_A_channel, 490, 8); // 490hz PWM, 8-bit resolution to match Arduino Uno
  // pfodESP32Utils::analogWrite(cmd_A_channel,cmd_A_var); // set PWM output

  Serial.begin(115200);
  for (int i = 10; i > 0; i--) {
    Serial.print(i);
    Serial.print(' ');
    delay(500);
  }
  Serial.println();


  setDebugPtr(&Serial);      //set global debug
  debugPtr = getDebugPtr();  // enable extra debug here


  // Create the BLE Device
  BLEDevice::init(localName);
  // Create the BLE Server
  serverPtr = BLEDevice::createServer();
  serverPtr->setCallbacks(&bleSerial);
  // Create the BLE Service
  BLEService* servicePtr = serverPtr->createService(SERVICE_UUID);
  // Create a BLE Characteristic
  characteristicTXPtr = servicePtr->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
  characteristicTXPtr->addDescriptor(new BLE2902());
  BLECharacteristic* characteristicRXPtr = servicePtr->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
  characteristicRXPtr->setCallbacks(&bleSerial);

  serverPtr->getAdvertising()->addServiceUUID(BLEUUID(SERVICE_UUID));
  // Start the service
  servicePtr->start();
  // Start advertising
  serverPtr->getAdvertising()->start();
  Serial.println("BLE Server and Advertising started");

  bleSerial.begin();
  // connect parser
  parser.connect(bleBufferedSerial.connect(&bleSerial));  // connect the parser to the i/o stream via buffer

  init_pfodMainMenu();
  // <<<<<<<<< Your extra setup code goes here
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(ledPin, OUTPUT);
}


// the loop routine runs over and over again forever:
void loop() {
  handle_pfodMainMenu(parser);
  //  <<<<<<<<<<<  Your other loop() code goes here
}


// ========== pfodBLESerial methods
pfodBLESerial::pfodBLESerial() {}

bool pfodBLESerial::isConnected() {
  return (connected);
}
void pfodBLESerial::begin() {}

void pfodBLESerial::close() {}

void pfodBLESerial::poll() {}

size_t pfodBLESerial::write(const uint8_t* bytes, size_t len) {
  for (size_t i = 0; i < len; i++) {
    write(bytes[i]);
  }
  return len;  // just assume it is all written
}

size_t pfodBLESerial::write(uint8_t b) {
  if (!isConnected()) {
    return 1;
  }
  txBuffer[txIdx++] = b;
  if ((txIdx == sizeof(txBuffer)) || (b == ((uint8_t)'\n')) || (b == ((uint8_t)'}'))) {
    flush();  // send this buffer if full or end of msg or rawdata newline
  }
  return 1;
}

int pfodBLESerial::read() {
  if (rxTail == rxHead) {
    return -1;
  }
  // note increment rxHead befor writing
  // so need to increment rxTail befor reading
  rxTail = (rxTail + 1) % sizeof(rxBuffer);
  uint8_t b = rxBuffer[rxTail];
  return b;
}

// called as part of parser.parse() so will poll() each loop()
int pfodBLESerial::available() {
  flush();  // send any pending data now. This happens at the top of each loop()
  int rtn = ((rxHead + sizeof(rxBuffer)) - rxTail) % sizeof(rxBuffer);
  return rtn;
}

void pfodBLESerial::flush() {
  if (txIdx == 0) {
    return;
  }
  characteristicTXPtr->setValue((uint8_t*)txBuffer, txIdx);
  txIdx = 0;
  characteristicTXPtr->notify();
}

int pfodBLESerial::peek() {
  if (rxTail == rxHead) {
    return -1;
  }
  size_t nextIdx = (rxTail + 1) % sizeof(rxBuffer);
  uint8_t byte = rxBuffer[nextIdx];
  return byte;
}

void pfodBLESerial::addReceiveBytes(const uint8_t* bytes, size_t len) {
  // note increment rxHead befor writing
  // so need to increment rxTail befor reading
  for (size_t i = 0; i < len; i++) {
    rxHead = (rxHead + 1) % sizeof(rxBuffer);
    rxBuffer[rxHead] = bytes[i];
  }
}

//=========== ESP32 BLE callback methods
void pfodBLESerial::onConnect(BLEServer* serverPtr) {
  Serial.println("Connect");
  // clear parser with -1 in case partial message left, should not be one
  addReceiveBytes(bleSerial.pfodEOF, sizeof(pfodEOF));
  connected = true;
}

void pfodBLESerial::onDisconnect(BLEServer* serverPtr) {
  Serial.println("Disconnect");
  // clear parser with -1 and insert {!} incase connection just lost
  addReceiveBytes(bleSerial.pfodEOF, sizeof(pfodEOF));
  addReceiveBytes((const uint8_t*)pfodCloseConnection, sizeof(pfodCloseConnection));
  serverPtr->getAdvertising()->start();
  connected = false;
  Serial.println("Start Advert");
}

void pfodBLESerial::onWrite(BLECharacteristic* pCharacteristic) {
  uint8_t* data = (uint8_t*)pCharacteristic->getData();
  size_t len = pCharacteristic->getLength();
  addReceiveBytes((const uint8_t*)data, len);
}
//======================= end pfodBLESerial methods


