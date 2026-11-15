# pfodWeb Arduino Example Sketches

The `examples/` directory contains seven Arduino sketches that act as **pfodDevices** for
[pfodWeb](../README.md). Each one demonstrates a different connection type (Serial, BLE, TCP/IP,
HTTP) and a different level of complexity, from a single "Hello World" drawing up to a full
multi-screen demo of different pfod menu items.

The BLE and TCP/IP versions also work unchanged with the Android **pfodApp**, since both clients speak the same
[pfod protocol](https://www.pfod.com.au/).

---

## Quick reference — sketches and the micros they compile for

| Sketch | Connection | Compiles for | Notes |
|---|---|---|---|
| `demoScreens_serial` | Serial @ 115200 | Any board with `Serial` | Uno or higher - Full demo over Serial |
| `Hello_World_serial` | Serial @ 115200 | Any Arduino board with a `Serial` port | Trivial Display |
| `Menu_1` | Serial @ 115200 | Any board with `Serial`, `A0` and PWM on pin 5 | Menu items, a chart and a sub-menu; no drawings |
| `LedOnOff_serial` | Serial @ 115200 | Any board with `Serial` and `LED_BUILTIN` | Has an `#ifdef ESP8266` LED-polarity switch |
| `LedOnOff_ble` | BLE (Nordic UART service) | **ESP32, ESP32-C3** | Needs ≥2 MB APP partition. Connects to pfodApp as well |
| `LedOnOff_tcp` | TCP/IP, port 4989 | **ESP8266, Pi Pico W, Pi Pico 2W, ESP32, ESP32-C3** | Needs WiFi credentials |
| `LedOnOff_http` | HTTP, port 80 | **ESP8266, Pi Pico W, Pi Pico 2W, ESP32, ESP32-C3** | Can optionally serve pfodWeb itself from LittleFS |


### "Compiles for" in more detail

* **Serial examples** (`Hello_World_serial`, `Menu_1`, `LedOnOff_serial`, `demoScreens_serial`)
  contain no processor-specific code beyond `LED_BUILTIN` (and `Menu_1`'s pin numbers), so they
  build for AVR (UNO/Nano/Mega), ESP32,
  ESP8266, Pi Pico and anything else with a `Serial` stream. The `// Board:` comment at the top of
  a generated sketch only records which board was selected in the pfodWeb Designer when the code
  was generated — it is not a restriction.
* **WiFi examples** need a networked core, so they are limited to ESP8266, ESP32/ESP32-C3 and the
  WiFi-capable Raspberry Pi Pico W / Pico 2W. They rely on `ESP_PicoW_pfodWebServer.h` /
  `ESP_PicoW_pfodAppServer.h` from the pfodParser library, which cover exactly those families.
* **The BLE example** is ESP32-only: it uses `BLEDevice.h`/`BLEServer.h`/`BLE2902.h` and FreeRTOS
  semaphores from the ESP32 core.

---

## Required libraries

Install from the Arduino Library Manager, or from
<http://www.forward.com.au/pfod/pfodParserLibraries/index.html>.

| Library | Used by | Provides |
|---|---|---|
| **pfodParser** V5.1.14+ | all sketches | `pfodParser.h`, `pfodDwgs.h`, `pfodDrawing.h`, `pfodDebugPtr.h`, `pfodBLEBufferedSerial.h`, `ESP_PicoW_pfodWebServer.h`, `ESP_PicoW_pfodAppServer.h`, `pfodDelay.h` |

---

## Connecting from pfodWeb

Open `pfodWeb.html` in a browser (from the `pfodWeb/` directory of this repo, or from the
`pfodWeb` sub-directory of the pfodParser library), then:

| Connection | Steps |
|---|---|
| **Serial** | Select *Serial*, follow the on-screen instructions to start **pfodProxy**, click *Select COM Port*, then *Connect via pfodProxy* |
| **BLE** | Select *BLE*, start pfodProxy, click *Select BLE* and choose the device (advertised as `pfod_BLE`), then *Connect via pfodProxy* |
| **TCP/IP** | Select *TCP/IP*, start pfodProxy, enter the board's IP, then *Connect via pfodProxy* |
| **HTTP** | Select *HTTP*, enter the board's IP, click *Connect*. No pfodProxy needed |

Only HTTP talks to the board directly from the browser; Serial, BLE and TCP/IP go through the
pfodProxy bridge.  If the browser supports Serial or BLE connections itself, then you can optionally use the browser native connection, but pfodProxy provides a richer experience and
 also remembers your selected serial/ble connection allows it to be bookmarked.

---

## The sketches

### 1. `demoScreens_serial` — the full pfod demo, over Serial

**Connection:** Serial @ 115200 · **Header board:** Arduino Uno, Mega, etc.

A single-file Arduino port of the `V3_DemoMsgProcessor.java` pfodApp demo, mirroring its command
hierarchy.  Its code does **NOT** match the layout of pfodWeb's generated code.

It also streams **raw data**: *outside* any `{…}` message so that
pfodWeb treats it as CSV plot data.


### 2. `Hello_World_serial` — simplest GUI example

**Connection:** Serial @ 115200 · **Header board:** Arduino UNO

The minimal pfodWeb generated project. The main menu holds a single drawing menu item, `HelloWorld`, which is a
50 × 50 blue canvas with the red bold text "Hello World" centred in it. Nothing is updated and no
pins are touched, so it is the easiest sketch to confirm that the toolchain, the pfodParser
library and pfodProxy are all working.

```
Hello_World_serial.ino     setup()/loop(), Serial.begin(115200), parser.connect(&Serial)
pfodMainMenu.cpp/.h        generated menu handler — sends the menu containing the drawing
Dwg_HelloWorld.cpp/.h      the drawing itself
menujson/                  pfodWeb Designer source (see "The menujson folders" below)
```

### 3. `Menu_1` — menu items, a chart and a sub-menu

**Connection:** Serial @ 115200 · **Header board:** Arduino UNO

A pfodWeb generated project built only from menu items, with no drawings. It shows the main menu item
types the pfodWeb Designer offers, each wired to a pin. The menu is titled *"Arduino Data"* and
refreshes every 1000 ms.

| Item | Type | Pin | What it does |
|---|---|---|---|
| *A0 … V* | data display | `A0` | `analogRead(A0)`, shown as 0 – 5.0 V |
| *Door is Closed / Open* | on/off display | `D4` | `digitalRead(4)` |
| *PWM Setting … %* | slider | `D5` | `analogWrite(5, value)`, shown as 0 – 100 % |
| *LED is Off / On* | on/off toggle | `D13` | `digitalWrite(13, value)` |
| *Voltage Plot* | chart | `A0` | plots the A0 voltage once a second |
| *Submenu* | sub-menu | — | opens *"Example Submenu"*, holding one button |

Two empty labels act as spacers between the items.

```
Menu_1.ino                 setup()/loop(), Serial.begin(115200), parser.connect(&Serial)
pfodMainMenu.cpp/.h        generated main menu: pin reads/writes, the chart data and its hooks
SubMenu_submenu.cpp/.h     the generated sub-menu and its button hook
menujson/                  pfodWeb Designer source (see "The menujson folders" below)
```

The chart's data is sent as raw CSV outside any `{…}` message, so it is not limited to the 1024-byte
message size. The pin numbers are the UNO's; change them in the Designer, or in the
`_pin` constants at the top of `pfodMainMenu.cpp`, to suit another board.

### 4. `LedOnOff_serial` — LED control over Serial

**Connection:** Serial @ 115200 · 

Adds mouse click interactions to the GUI. The main-menu drawing `LedOnOff` inserts two sub-drawings — `LedOn` and
`LedOff` — that act as touch buttons, plus an indexed label that reads *"Led is ON"* in red or
*"Led is Off"* in black. The drawing refreshes every 2000 ms, and touches call `turnLedOn()` /
`turnLedOff()` in `LedOnOffApp.cpp`, which drive `LED_BUILTIN`.  The buttons are designed to change color while the mouse is held down on them.

### 5. `LedOnOff_ble` — the same GUI over Bluetooth Low Energy

**Connection:** BLE · **Compiles for:** ESP32, ESP32-C3

Identical menu and drawings to `LedOnOff_serial`, but the parser is connected to a BLE stream
instead of `Serial`. The sketch contains a complete `pfodBLESerial` class implementing the
Nordic UART service:

Sends are buffered through `pfodBLEBufferedSerial` to avoid flooding the BLE link.

* Choose a partition scheme with **at least 2 MB APP**, e.g. *NO OTA (2M APP/2M SPIFFS)*.
* The advertised name is set by `const char* localName = "pfod_BLE";` — change it to
  customise what appears in the BLE device list.
* `Serial` remains free and is used for debug output at 115200.

### 6. `LedOnOff_tcp` — the same GUI over TCP/IP

**Connection:** TCP/IP port 4989 · **Compiles for:** ESP8266, Pi Pico W, Pi Pico 2W, ESP32, ESP32-C3

Uses `ESP_PicoW_pfodAppServer.h`

Before uploading:

```cpp
const char *ssid = "xxxxxx";        // your router's SSID
const char *password = "xxxxxx";    // your router's password
//IPAddress staticIP(10, 1, 1, 100);  // uncomment and set an unused IP (recommended)
IPAddress staticIP;                   // or leave blank for DHCP and read the IP from the Serial Monitor
```

If you are debugging a Pi Pico with a PicoProbe, uncomment `#define PICO_PROBE` to move `Serial`
to `Serial1`.

### 7. `LedOnOff_http` — the same GUI over HTTP

**Connection:** HTTP port 80 · **Compiles for:** ESP8266, Pi Pico W, Pi Pico 2W, ESP32, ESP32-C3

Uses `ESP_PicoW_pfodWebServer.h`. This is the only connection type that needs no pfodProxy — the
browser talks to the board directly.

It also ships a `data/` sub-directory containing the gzipped pfodWeb bundle, so the board can serve
the whole web GUI itself for a stand-alone, off-line installation:

```cpp
static bool useLittleFSToServe_pfodWeb = false;   // set true to serve pfodWeb from LittleFS
static uint32_t cacheSec = 10*60;                 // browser cache timeout, used only when the above is true
```

To use it:

1. Set the Tools menu flash size to give a **1 MB LittleFS** file system.
2. Set `useLittleFSToServe_pfodWeb = true`, set the SSID/password and static IP.
3. Upload the sketch, **close the Serial Monitor**, then `Ctrl+Shift+P` →
   *Upload LittleFS to Pico/ESP8266/ESP32* to upload the `data/` directory.
   This needs the [arduino-littlefs-upload](https://github.com/earlephilhower/arduino-littlefs-upload)
   V0.2.0 plugin for Arduino IDE V2.
4. Browse to `http://<board-ip>` — note **http://**, not https://.

---

## Common details

### The `menujson` folders

`Hello_World_serial`, `Menu_1` and the four `LedOnOff_*` examples were produced by the **pfodWeb Designer**
and keep their design source alongside the code.

Generate Code writes **one** file into `menujson/` — exactly what *Save Design to File* would
have downloaded, so the two commands produce the same artifact:

* `<name>.pfodMenu_json` — when the design links no drawings, the design on its own.
  Load it with **Edit existing Menu → Load Design from File**.
* `<name>_menuJson.zip` — when it does, a bundle laid out as

  ```
  <name>.pfodMenu_json
  dwgs/<dwg>.pfodDwg_json      one per linked drawing
  ```

  covering every drawing the design reaches, recursively including whatever each reaches via
  `insertDwg`. Load it with the same **Edit existing Menu → Load Design from File** (one button
  takes either shape), or with the Dwg
  Controls Panel's **Load Dwg**, which takes the drawings *and* the design. Both accept
  the whole generated sketch zip as well — they look inside `menujson/` for the bundle.

  Neither overwrites anything already loaded. A drawing whose name is already in the library is
  left alone; a design already in the menu list is reported as a duplicate by the Dwg Controls
  Panel, and loaded beside it as `<name>_2` by Edit existing Menu, which is where you go when
  you actually want that design open. Both say what they did.

Load these back into the Designer to modify the GUI and re-generate `pfodMainMenu.*`,
`SubMenu_*.*` and `Dwg_*.*`. The `demoScreens_*` sketches are hand-written and have no design source.

What each sketch here ships:

| Sketch | `menujson/` holds |
|---|---|
| `Hello_World_serial` | `Hello_World_Serial_menuJson.zip` — the design + `HelloWorld` |
| `LedOnOff_serial` / `_ble` / `_tcp` / `_http` | `<name>_menuJson.zip` — the design + `LedOnOff`, `LedOn`, `LedOff` |
| `Menu_1` | `Menu_1.pfodMenu_json` — no drawings, so no zip |


### The `data` folder

`LedOnOff_http/data` holds the gzipped pfodWeb bundle
(`pfodWeb.html.gz`, `pfodweb-00N-*.js.gz`, `favicon.ico`, `version.js`) plus an `extraFonts`
sub-directory with Cyrillic/Greek Roboto subsets. Upload it to LittleFS only if you set
`useLittleFSToServe_pfodWeb = true`; about 1 MB of file system is needed.

### Menu version and caching

Every generated sketch sets `const char version[] = "V1";` (`demoScreens_serial` uses
`pfodParser parser("V361");`). pfodWeb and pfodApp cache a menu against this version string, so **change the version (or clear the cache) each time you edit a menu**,
otherwise the client will keep displaying the old cached screen.

### Editing generated code

Regenerating from the Designer overwrites `pfodMainMenu.*`, `SubMenu_*.*` and `Dwg_*.*`, so keep
application logic in sub-classes and the .ino.

Every generated class is reached through an **accessor function**, never through the global
instance it defaults to:

| Generated class | Accessor | Default instance |
|---|---|---|
| `Dwg_<Name>` | `get_dwg_<Name>()` | `dwg_<Name>` |
| `pfodMainMenu` | `get_pfodMainMenu()` | `mainMenu` |
| `SubMenu_<Ident>` | `get_subMenu_<Ident>()` | `subMenu_<Ident>` |

Each accessor is defined `__attribute__((weak))` in the generated .cpp. Defining the same function
in a file of **your own** replaces it at link time, so the generated code drives your object
instead — without a single edit to a generated file:

```cpp
// MyLedOnOff.cpp -- a file the Designer never touches
#include "Dwg_LedOnOff.h"

class MyLedOnOff : public Dwg_LedOnOff {
  protected:
    virtual void sendIndexedItems();   // your version, not the generated one
};

MyLedOnOff myLedOnOff;

// Replaces the weak default in the generated Dwg_LedOnOff.cpp.
Dwg_LedOnOff& get_dwg_LedOnOff() { return myLedOnOff; }
```

Two things make this work, and both are already in the generated code:

* the methods worth overriding are `virtual` — for a drawing `init()`, `sendIndexedItems()` and each
  `Dwg_<Name>_<cmdName>()` touch handler; for the main menu `init()`, `handle()`,
  `sendMainMenu()`, `sendMainMenuUpdate()` and the item hooks, named from the item's type
  (`onOffOut_Changed()` for a toggle, `sliderIO_Changed()` for a slider, `button_Pressed()` for a
  button, `chart_readPlotData()` for a chart); for a sub-menu `init()`, `sendMenu()`,
  `sendMenuUpdate()`, `handleCmd()` and its own item hooks. Calls arrive through a base-class
  reference, so a non-virtual method would silently keep running the base version;
* the `pfodAutoIdx` / `pfodAutoCmd` members are `protected`, so an override can name the indexes and
  commands it needs to send or handle.

The default instance is left un-`init()`ed, so it never registers with the parser and, for a
drawing, never allocates its `pfodDwgs`.

The exact recipe, with your own class and instance names filled in, is repeated in a comment block
at the bottom of each generated header.

`LedOnOffApp.cpp` in each `LedOnOff_*` sketch is a complete worked example. It subclasses all
three drawings and the main menu: `MyLedOnOff` overrides `sendIndexedItems()` to show the LED
state, `MyLedOn`/`MyLedOff` handle the button touches, and `MyMainMenu` sets up the LED pin in
`init()`. Each takes over by defining its accessor (`get_dwg_LedOnOff()`, `get_dwg_LedOn()`,
`get_dwg_LedOff()`, `get_pfodMainMenu()`).

### Debug output

The BLE and WiFi sketches call `setDebugPtr(&Serial)`. Every generated sketch has a
commented-out `// #define DEBUG` in `pfodMainMenu.cpp` and each `Dwg_*.cpp`. Uncomment it in a file to get that file's debug tracing
on `Serial`. In the WiFi sketches the startup countdown and the assigned IP address are printed to `Serial` at 115200.

---

## Further reading

* pfodWeb tutorial — <https://www.forward.com.au/pfod/pfodWeb/index.html>
* pfodWeb Designer — <https://www.forward.com.au/pfod/pfodWeb/Designer/index.html>
* Getting started with pfodApp for Android —
  <https://www.forward.com.au/pfod/Android_pfodApp/pfodAppForAndroidGettingStarted.pdf>
* pfodParser libraries — <http://www.forward.com.au/pfod/pfodParserLibraries/index.html>

(c) Forward Computing and Control Pty. Ltd. — see [../docs/pfodWeb_pfodProxy_License.html](../docs/pfodWeb_pfodProxy_License.html).
