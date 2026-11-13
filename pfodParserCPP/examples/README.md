# pfodParserCPP examples

Each example is the **OnOff_Serial** sketch that pfodWeb's Designer generates for the
**pfodParserCPP** target, with its two board specific files, `target_Stream.h/.cpp`
and `target_pfodClock.cpp`, completed for one platform's *native, non-Arduino* SDK.

| Directory | Platform | `target_Stream` uses | `micros()` uses |
|---|---|---|---|
| `esp32/OnOff_Serial/` | ESP32 (ESP-IDF) | `driver/uart.h`, the driver's own rx/tx buffers | `esp_timer_get_time()` |
| `esp8266/OnOff_Serial/` | ESP8266 under **ESP8266_RTOS_SDK** | `driver/uart.h` on UART0 (same `target_Stream` as esp32) | `esp_timer_get_time()` |
| `stm32/OnOff_Serial/` | STM32 (any family) | STM32Cube HAL UART, interrupt driven RX into a `pfodRingBuffer` | HAL 1 ms SysTick + `SysTick->VAL` |
| `nrf52/OnOff_Serial/` | Nordic nRF52 (nRF5 SDK) | nRF5 SDK `app_uart` (FIFO buffered UART) | TIMER2 as a free running 1 MHz 32 bit counter |
| `nrf54l_bm/OnOff_Serial/` | Nordic nRF54L Series, **nRF Connect SDK Bare Metal option** (no RTOS, low power) | nrfx UARTE driver, RX into a `pfodRingBuffer`; CPU sleeps between received bytes | GRTC SYSCOUNTER (`k_cycle_get_64()`), 1 µs, runs in all power modes |
| `zephyr/OnOff_Serial/` | Zephyr RTOS (incl. nRF Connect SDK) | Zephyr interrupt driven UART API (`uart_irq_*`/`uart_fifo_read`/`uart_poll_out`), RX into a `pfodRingBuffer` | `k_uptime_ticks()` |
| `rp2040/OnOff_Serial/` | Raspberry Pi RP2040 / RP2350 (Pico SDK) | Pico SDK `hardware/uart.h`, RX interrupt into a `pfodRingBuffer` | `time_us_32()`, the 1 MHz system timer |
| `sam/OnOff_Serial/` | Microchip SAM D21 / D51 / E5x ... Cortex-M (MPLAB Harmony 3) | Harmony 3 SERCOM USART PLIB in Ring buffer mode (MCC generated `SERCOMx_USART_Read`/`Write`/...) | Cortex-M SysTick, 1 ms interrupt + `SysTick->VAL` |
| `kinetis/OnOff_Serial/` | NXP Kinetis K (FRDM-K64F), MCUXpresso SDK 2.x | MCUXpresso SDK `fsl_uart.h`, RX interrupt into a `pfodRingBuffer` | Cortex-M SysTick, 1 ms interrupt + `SysTick->VAL` |
| `mcx/OnOff_Serial/` | NXP MCX N (FRDM-MCXN947), current MCUXpresso SDK | MCUXpresso SDK `fsl_lpuart.h` (LP_FLEXCOMM version), RX interrupt into a `pfodRingBuffer` | Cortex-M SysTick, 1 ms interrupt + `SysTick->VAL` |
| `pic32/OnOff_Serial/` | Microchip PIC32 (MPLAB Harmony 3) | Harmony 3 UART PLIB in Ring buffer mode (MCC generated `UARTx_Read`/`Write`/...) | MIPS core timer, accumulated |

For other micros and their compilers see
[Common non-Arduino micros and their compilers](../BUILDING.md#common-non-arduino-micros-and-their-compilers).

**ESP8266: only ESP8266_RTOS_SDK is supported (`esp8266/OnOff_Serial/`). The ESP8266
NonOS SDK is not supported.**

## Files in each example

The generated files, unchanged from pfodWeb's OnOff_Serial output:

- **`pfodMainMenu.h/.cpp`** -- the main menu: an On/Off slider, labels and a drawing menu item.
- **`Dwg_LedOnOff.h/.cpp`, `Dwg_LedOn.h/.cpp`, `Dwg_LedOff.h/.cpp`** -- the drawings.
- **`pfodParser.h`** -- adapter so the generated files' `#include <pfodParser.h>`,
  `F("...")` and `millis()` work against pfodParserCPP. Keep it in the same
  directory and put that directory on the include path.
- **`readme.txt`** -- the generated porting guide for completing `target_Stream` and
  `target_pfodClock.cpp`. It describes the uncompleted templates; in these
  examples that work has already been done for the platform.
- **`menujson/`** -- the pfodWeb Designer menu design (`OnOff_serial_menuJson.zip`),
  to load back into pfodWeb's Designer to edit and regenerate the menu.

The board specific files:

- **`main.cpp`** -- the generated `main.cpp` with the board's `serialStream.begin(...)`
  arguments, and the platform's entry point (`app_main()` on ESP-IDF, `main()` with
  `SYS_Initialize()`/`SYS_Tasks()` on Harmony, etc.) calling `setup()` once and then
  `loop()` forever.
- **`target_Stream.h/.cpp`** -- the generated `target_Stream` template, a `pfodStream`
  subclass following the pattern of Arduino's `HardwareSerial`, with `begin()`,
  `write(uint8_t)`, `flush()` and `read()` implemented with the platform's UART driver.
  Where the driver has its own rx buffer count, or receive is interrupt driven into a
  `pfodRingBuffer`, `available()`/`peek()` use that instead of the one byte look ahead.
- **`target_pfodClock.cpp`** -- `micros()` for the platform (pfodParserCPP does not
  implement it), and a commented out `yield()` for the platform.

The application, which turns an output pin (normally the board's LED) on and off:

- **`OnOff_SerialApp.cpp`** -- the same in every example. Subclasses the generated menu
  and drawing classes (`pfodMainMenuApp`, `Dwg_LedOnOffApp`, `Dwg_LedOnApp`,
  `Dwg_LedOffApp`) and defines strong `get_pfodMainMenu()` / `get_dwg_LedXXX()` functions
  returning them, which replace the generated weak ones. The generated files are not edited.
  The menu slider and the drawings' On / Off buttons set the pin, and the drawing's label
  shows its state.
- **`target_Pin.h/.cpp`** -- `target_PinInit()` and `target_PinWrite(bool on)` for the
  platform's GPIO API. The header comment of each `target_Pin.cpp` gives the default pin,
  and how to set `PFOD_PIN` / `PFOD_PIN_ON_LEVEL` (`PFOD_PIN_NODE` for Zephyr) for your board.
  For PIC32 name the LED pin `LED` in MCC; for Zephyr add `CONFIG_GPIO=y` to prj.conf.

Compare these with the generated templates in a freshly generated sketch (and its
`readme.txt`) to port to another board.

## Building

These are not part of the desktop CMake build in [`../BUILDING.md`](../BUILDING.md)
(`CMakeLists.txt` only picks up `src/*.cpp` and `src/dwgs/*.cpp`). Add the example's
files, plus pfodParserCPP's `src/` and `src/dwgs/` files, to your own cross compiled
project (ESP-IDF `idf.py` "main" component, STM32CubeIDE, nRF5 SDK Makefile/SES project,
Zephyr app, MPLAB X / MCC Harmony project), with the example directory and `src/` on
the include path. Each `main.cpp` header comment lists what else that platform needs
(e.g. `CONFIG_UART_INTERRUPT_DRIVEN=y` for Zephyr, Ring buffer mode for the Harmony UART).

### build.sh

Every example (`esp32/`, `esp8266/`, `stm32/`, `nrf52/`, `nrf54l_bm/`, `zephyr/`, `rp2040/`,
`sam/`, `kinetis/`, `mcx/` and `pic32/OnOff_Serial/`) has a `build.sh` that builds the example
stand alone, into `build/` in the example directory. See the comments at the top of each
script for the details and the environment variables that override its defaults.

- **`esp32/OnOff_Serial/build.sh`** (`bash build.sh`, Git Bash on Windows, or Linux / macOS)
  -- no ESP-IDF install needed. Uses the prebuilt ESP-IDF libraries, headers, xtensa
  toolchain and esptool from the Arduino ESP32 core 3.x (Arduino IDE Boards Manager),
  for an ESP32 Dev Module. Produces `build/OnOff_Serial.merged.bin` to flash at 0x0.
- **`stm32/OnOff_Serial/build.sh`** (`bash build.sh [--delete-sdk]`) -- builds for an
  STM32F401RE (Nucleo-F401RE) with the STM32CubeF4 HAL and arm-none-eabi-gcc. Uses an
  installed STM32CubeF4 package if found, otherwise downloads the HAL (about 40MB) into
  `build/`; `--delete-sdk` deletes that download afterwards. Uses `STM32F401RE.ld`.
  This checks the example compiles and links; `main.cpp`'s HAL / clock / UART setup TODO
  still has to be done (normally by CubeMX) before it runs on a board.
- **`nrf52/OnOff_Serial/build.sh`** (`bash build.sh [--delete-sdk]`) -- builds for the
  nRF52 DK (nRF52832, PCA10040) with the nRF5 SDK 17.1.0 and arm-none-eabi-gcc, using the
  `sdk_config.h`, sources, flags and linker script of the SDK's
  `examples/peripheral/uart/pca10040/blank`. Uses `NRF5_SDK` if set, otherwise downloads
  Nordic's SDK zip (about 130MB, 575MB unzipped) into `build/`; `--delete-sdk` deletes
  it afterwards. Produces `build/OnOff_Serial.hex`.
- **`nrf54l_bm/OnOff_Serial/build.sh`** (`bash build.sh [--delete-sdk]`) -- builds for the
  nRF54L15 DK with the nRF Connect SDK Bare Metal option and the S115 SoftDevice, with
  `west build` and this directory's `CMakeLists.txt` / `prj.conf`. Uses `NRF_BM_WS` (an
  existing sdk-nrf-bm west workspace) if set, otherwise downloads sdk-nrf-bm v2.0.1 and its
  modules, the Zephyr SDK arm-zephyr-eabi toolchain and the python packages (about 2.7GB)
  into `build/`; `--delete-sdk` deletes them afterwards. Needs python 3.10+ with venv.
  Produces `build/app/OnOff_Serial/zephyr/zephyr.hex`; `west flash -d build/app` flashes
  it and the SoftDevice.
- **`zephyr/OnOff_Serial/build.sh`** (`bash build.sh [--delete-sdk]`) -- builds the example
  as a Zephyr app with `west build`, default for the nRF52840 DK (`BOARD` overrides it).
  Uses `ZEPHYR_WS` (an existing Zephyr / nRF Connect SDK west workspace) if set, otherwise
  downloads Zephyr v4.5.0 with only the cmsis, picolibc, segger and `ZEPHYR_HAL` (default
  `hal_nordic`) modules, the Zephyr SDK arm-zephyr-eabi toolchain and the python packages
  (about 3.8GB) into `build/`; `--delete-sdk` deletes them afterwards. Needs python 3.10+ with venv.
  Produces `build/app/zephyr/zephyr.hex`.
- **`esp8266/OnOff_Serial/build.sh`** (`wsl bash build.sh [--delete-sdk]` on Windows, or
  Linux / macOS) -- builds with ESP8266_RTOS_SDK release/v3.4 and `idf.py`, in a generated
  project in `build/project`. Uses `IDF_PATH` if set, otherwise downloads the SDK, the
  xtensa-lx106 toolchain and the python packages (about 400MB) into `build/`.
- **`pic32/OnOff_Serial/build.sh`** (`PIC32_MCC_SRC=... PIC32_PROCESSOR=... bash build.sh`)
  -- builds with MPLAB XC32 against your MCC Harmony 3 project's generated code, without
  MPLAB X. Downloads nothing: XC32 must be installed, and the MCC project generated in
  MPLAB X first (UART in Ring buffer mode, LED pin named `LED`). It compiles everything in
  the project's `firmware/src/config/<config>/`, plus this example (whose `main.cpp`
  replaces MCC's `main.c`) and pfodParserCPP. Produces `build/OnOff_Serial.hex`.
- **`rp2040/OnOff_Serial/build.sh`** (`bash build.sh [--delete-sdk]`) -- builds for the
  Raspberry Pi Pico (`PICO_BOARD=pico2` for the Pico 2, RP2350) with the Pico SDK and this
  directory's `CMakeLists.txt`. Uses `PICO_SDK_PATH`, or the Pico SDK in the Arduino
  rp2040 (arduino-pico) core, otherwise clones pico-sdk 2.3.0 into `build/`; a standard
  arm-none-eabi-gcc (`PICO_TOOLCHAIN_PATH`, PATH or an Arduino15 `tools/arm-none-eabi-gcc`,
  not arduino-pico's `pqt-gcc`, see the script); cmake and ninja from the PATH or pip
  installed into `build/venv` (about 120MB). Produces `build/app/OnOff_Serial.uf2` (with
  picotool, e.g. the arduino-pico core's) to copy to the board's BOOTSEL drive.
- **`sam/OnOff_Serial/build.sh`** (`bash build.sh [--delete-sdk]`) -- builds with
  arm-none-eabi-gcc (no MPLAB X / XC32) against MCC Harmony 3 generated code: your MCC
  project's `firmware/src/config/<config>/` (`SAM_MCC_SRC`), otherwise Microchip's own
  generated SAM D21 Xplained Pro Ring buffer USART project from GitHub
  (`csp_apps_sam_d21_da1`, sparse cloned), plus the SAMD21 device pack (about 10MB) for the
  device headers and the gcc startup / linker script (MCC's are XC32 only). CMSIS core
  headers from an Arduino15 `tools/CMSIS`, otherwise downloaded. `--delete-sdk` deletes the
  downloads afterwards. Produces `build/OnOff_Serial.hex` / `.bin`.
- **`kinetis/OnOff_Serial/build.sh`** (`bash build.sh [--delete-sdk]`) -- builds for the
  FRDM-K64F with the MCUXpresso SDK 2.x and arm-none-eabi-gcc (no MCUXpresso IDE), using the
  SDK's device files, drivers, board files and the flags of its uart interrupt example (whose
  `pin_mux.c` is used). The FRDM-K64F is not in the current MCUXpresso SDK, so this uses the
  last SDK with it, github.com/nxp-mcuxpresso/legacy-mcux-sdk: `NXP_SDK` if set, otherwise a
  sparse clone of just the needed parts (about 6MB) into `build/`; `--delete-sdk` deletes it
  afterwards. Produces `build/OnOff_Serial.bin` to copy to the board's OpenSDA drive.
- **`mcx/OnOff_Serial/build.sh`** (`bash build.sh [--delete-sdk]`) -- builds for the
  FRDM-MCXN947 (Cortex-M33 core 0) with the current MCUXpresso SDK and arm-none-eabi-gcc
  (no MCUXpresso IDE / west), using the SDK's device files, drivers (including the
  LP_FLEXCOMM LPUART driver), FRDM-MCXN947 board files and its lpuart interrupt example's
  `pin_mux.c`. Uses `NXP_SDK` (a west fetched SDK) if set, otherwise sparse clones just the
  needed parts of mcuxsdk-core / mcux-devices-mcx and downloads the board files (about 17MB)
  into `build/`; `--delete-sdk` deletes them afterwards. Produces `build/OnOff_Serial.hex` /
  `.bin` to flash through the board's MCU-Link.

The esp32, stm32, nrf52, nrf54l_bm, zephyr, rp2040, sam, kinetis and mcx scripts have been run (all compile with
`-Wall -Wextra` and no warnings from the example or pfodParserCPP, and link). The esp8266
and pic32 scripts have not been run against the real SDK yet (the pic32 script has only
been checked with stand-in tools and MCC files).

`zephyr/OnOff_Serial/` and `nrf54l_bm/OnOff_Serial/` have a `CMakeLists.txt` and `prj.conf`,
so they also build with `west build` in your own workspace, e.g.
`west build -b nrf52840dk/nrf52840` for the zephyr example. `rp2040/OnOff_Serial/` has a
Pico SDK `CMakeLists.txt`, so it also builds like any Pico SDK project.

## Compiled against the real SDKs, not tested on hardware

None of the examples has been run on a board yet. These have been compiled and linked
against the real SDK (with the build.sh scripts / west, see above):

| Example | Compiled and linked against |
|---|---|
| `esp32` | ESP-IDF v5.5.5 (the Arduino ESP32 core 3.3.11 prebuilt libraries), ESP32 Dev Module |
| `stm32` | STM32CubeF4 HAL, STM32F401RE |
| `nrf52` | nRF5 SDK 17.1.0, nRF52 DK (PCA10040) |
| `nrf54l_bm` | sdk-nrf-bm v2.0.1, nRF54L15 DK with the S115 SoftDevice |
| `zephyr` | Zephyr v4.5.0 and the Zephyr in nRF Connect SDK v3.4.1, nRF52840 DK |
| `rp2040` | Pico SDK 2.3.0 (from the arduino-pico core) with arm-none-eabi-gcc 9-2019q4, Pico (RP2040) and Pico 2 (RP2350) |
| `kinetis` | MCUXpresso SDK 2.x (legacy-mcux-sdk) with arm-none-eabi-gcc 9-2019q4, FRDM-K64F (MK64FN1M0VLL12) |
| `mcx` | current MCUXpresso SDK (mcuxsdk-core, mcux-devices-mcx, mcuxsdk-examples board files) with arm-none-eabi-gcc 9-2019q4, FRDM-MCXN947 (MCXN947VDF core 0) |
| `sam` | MCC Harmony 3 generated code (Microchip's csp_apps_sam_d21_da1 ring buffer USART project) and SAMD21 DFP 3.8.270, with arm-none-eabi-gcc 9-2019q4, SAM D21 Xplained Pro (ATSAMD21J18A) |

`esp8266` (ESP8266_RTOS_SDK) and `pic32` (MPLAB Harmony 3) have only been compiled and
linked (g++ `-Wall -Wextra`, together with the pfodParserCPP library and the generated
files) against minimal stand-in SDK headers. For those check struct field names, macro
names and function signatures against the SDK version you are using.

For all of them, check against your SDK version if it differs; some APIs (notably the
nRF5 SDK's `app_uart_comm_params_t`) have drifted a little across SDK versions.

## yield()

The library has no `delay()`. Its only busy waits are in `pfodBufferedStream`
(`write()` in blocking mode, and `flush()`), which call `yield()` while waiting. The
default `yield()` in `src/pfodClock.cpp` is weak and empty. On the RTOS platforms
`target_pfodClock.cpp` defines `yield()` so other tasks keep running while waiting:
`vTaskDelay(1)` for ESP-IDF / ESP8266_RTOS_SDK, `k_msleep(1)` for Zephyr. On the bare
metal ones (STM32, nRF52, PIC32) there is nothing else to run, so the default empty
`yield()` is used; each has a commented out watchdog feed to uncomment if you enable
the watchdog.
See "Clock: micros(), millis() and yield()" in
[`../BUILDING.md`](../BUILDING.md#clock-micros-millis-and-yield).
