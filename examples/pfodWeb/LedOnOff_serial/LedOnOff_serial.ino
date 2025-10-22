/*
   LedOnOff_serial.ino
   (c)2025 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This generated code may be freely used for both private and commercial use
   provided this copyright is maintained.

*/

/**
  This example should compile for all Arduino boards Connects via serial at 115200 baud
  See the tutorial at https://www.forward.com.au/pfod/pfodWeb/index.html

  Setup Notes:
  =============
  Install the latest pfodParser and SafeString libraries from the library manager
  Use a Chrome or Edge browser (>V141)
  Load this sketch and CLOSE the Serial Monitor
  From the pfodParse library, in sub-directory pfodWeb, open pfodWeb.html in a Chrome or Edge browser (>V141)
  and selected Serial connection, 115200 and click Connect and Pair your board's serial port.
  Note: This will fail if the Arduino IDE or other application already has that port open. Close the Arduino Serial Monitor.

  If connection fails the first time, just try again, check that baud rate is correct for your board (115200 in this sketch)
  If it continues to fail, check that the Serial pfod messages are working by opening the Arduino IDE serial monitor
  and enterings {.}  The monitor should show {,<bg w>~`0~V1|+A~c1}

*/

// download the libraries from http://www.forward.com.au/pfod/pfodParserLibraries/index.html
// pfodParser.zip V3.65+ contains pfodParser, pfodSecurity, pfodDelay, pfodBLEBufferedSerial, pfodSMS and pfodRadio
#include <pfodParser.h>

#include <pfodDebugPtr.h>
#include "pfodMainMenu.h"

pfodParser parser("V1");                  // create a parser with menu version string to handle the pfod messages

// initialize digital pin for the LED
const int ledPin = LED_BUILTIN;
bool ledIsOn = false;

#ifdef ESP8266
bool highIsOn = false; // some boards use low for On, set this false for those boards
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

  Serial.begin(115200);
  Serial.println();

  setDebugPtr(&Serial);      //set global debug
  debugPtr = getDebugPtr();  // enable extra debug here

  // connect parser
  parser.connect(&Serial);  // connect the parser to the i/o stream via buffer
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

