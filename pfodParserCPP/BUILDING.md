# Building pfodParser as a standalone C++ library

`src/` (the core parser and drawing-command library: `pfodParser`, `pfodRawCmdParser`,
`pfodDwgs` + `dwgs/`, buffering/utility classes, etc.) is plain C++ with no Arduino
dependency, and builds standalone with CMake.

## Requirements

- **C++ standard: C++11 or later.** The library is built with `-std=c++11` (see
  `CMakeLists.txt`); it also compiles cleanly under C++14/17/20 with no changes.
  C++11 is required for `override` specifiers, in-class member initializers,
  and `nullptr`. (The library does not use `std::chrono`; a program's own
  `micros()` may, see [Clock](#clock-micros-millis-and-yield).)
- **A C99/C11 C compiler** for `src/HexConversionUtils.c`.
- **CMake >= 3.10**.

## Compilers tested / expected to work

- **GCC >= 4.8**
- **Clang >= 3.3**

Any GCC or Clang compatible compiler with reasonably complete C++11 support should work.
The code uses `__attribute__((weak))`, for the default `millis()` and `yield()` in
`pfodClock.cpp` and for the `get_pfodMainMenu()` / `get_dwg_xxx()` functions in the code
pfodWeb generates (`pfodMainMenu.cpp`, `Dwg_xxx.cpp`), which is how a program replaces the
generated menu and drawings with its own subclasses without editing the generated files,
so the compiler and target object format must support weak symbols (e.g. ELF, as
used by GCC/Clang embedded toolchains).

### Common non-Arduino micros and their compilers

The last column is whether the compiler supports the `__attribute__((weak))` used for
the default `millis()` / `yield()` in `pfodClock.cpp` and the `get_pfodMainMenu()` /
`get_dwg_xxx()` functions in the pfodWeb generated code. "Check" entries have not been
verified, see the compiler's manual.

| Micro family | SDK / framework | Usual compiler(s) | `__attribute__((weak))` |
|---|---|---|---|
| **STM32** (Cortex-M0/M3/M4/M7/M33) | STM32Cube HAL/LL, CubeIDE | arm-none-eabi-gcc (GNU Arm), Keil MDK (armclang / ARMCC 5), IAR EWARM | GCC, armclang: yes. ARMCC 5: yes (also `__weak`). IAR: check, it uses `__weak` / `#pragma weak` |
| **Nordic nRF52/nRF53/nRF91** | nRF5 SDK (legacy), nRF Connect SDK (Zephyr) | arm-none-eabi-gcc, Segger Embedded Studio (gcc/clang), Keil, IAR | gcc/clang: yes |
| **Nordic nRF54L Series** (nRF54L15/L10/L05, LM20, LV10, LS05...) | nRF Connect SDK: Zephyr, or the **Bare Metal option** (no RTOS, SoftDevice S115/S145, from 2025) | arm-none-eabi-gcc (Zephyr SDK toolchain) | yes |
| **ESP32** (Xtensa LX6/LX7) | ESP-IDF | xtensa-esp32-elf-gcc (xtensa-esp-elf in IDF 5.x) | yes |
| **ESP32-C3/C6/H2/P4** (RISC-V) | ESP-IDF | riscv32-esp-elf-gcc | yes |
| **ESP8266** | ESP8266_RTOS_SDK (NonOS SDK not supported) | xtensa-lx106-elf-gcc | yes |
| **Raspberry Pi RP2040 / RP2350** | Pico SDK | arm-none-eabi-gcc (RP2350 RISC-V cores: riscv32 gcc) | yes |
| **Microchip PIC32** (MIPS) | MPLAB Harmony 3 / MCC | XC32 (gcc based) | yes |
| **Microchip SAM** (SAMD21/51, SAME70; Cortex-M) | Harmony 3 / ASF4 / MCC | XC32 (Arm) or arm-none-eabi-gcc | yes |
| **Microchip PIC16/PIC18** (8 bit) | MCC | XC8 (C only) | No C++. Use pfodParserC |
| **Microchip dsPIC33/PIC24** (16 bit) | MCC | XC16 (C; C++ only in XC-DSC) | Check C++ support first |
| **AVR** (ATmega, ATtiny, AVR-Dx) without Arduino | AVR-libc / Atmel Studio | avr-gcc, XC8 (AVR) | avr-gcc: No standard C++ library.  Use pfodParserC |
| **NXP LPC / Kinetis / i.MX RT** (Cortex-M) | MCUXpresso SDK | arm-none-eabi-gcc (MCUXpresso IDE), Keil, IAR | gcc: yes |
| **TI MSP432 / Tiva C / CC13xx/CC26xx** (Cortex-M) | SimpleLink SDK, TI-RTOS / FreeRTOS | TI Arm Clang (tiarmclang), arm-none-eabi-gcc, IAR | clang/gcc: yes |
| **TI MSP430** (16 bit) | MSP430 driverlib | msp430-elf-gcc, TI MSP430 CGT, IAR | gcc: yes |
| **TI C2000** (DSP) | C2000Ware | TI C2000 CGT | Non-GCC; check `#pragma WEAK`. Its char is 16 bit, so it needs care |
| **Renesas RA** (Cortex-M) | FSP | arm-none-eabi-gcc (e² studio), IAR, armclang | gcc: yes |
| **Renesas RX / RL78** | FIT / Code Generator | CC-RX / CC-RL, GCC for RX/RL78 | GCC: yes. CC-RX: check |
| **Silicon Labs EFR32 / EFM32** (Cortex-M) | Gecko SDK / Simplicity Studio | arm-none-eabi-gcc, IAR | gcc: yes |
| **Infineon PSoC 6 / XMC / AURIX** | ModusToolbox / DAVE / iLLD | arm-none-eabi-gcc; AURIX: TASKING, HighTec (gcc/clang) | gcc: yes. TASKING: check |
| **GD32 / CH32V / WCH** (Cortex-M / RISC-V clones) | vendor SDK / MounRiver | arm-none-eabi-gcc, riscv-none-elf-gcc | yes |
| **SiFive / generic RISC-V** | Freedom SDK, Zephyr | riscv64-unknown-elf-gcc, LLVM clang | yes |
| **Any Zephyr-supported board** | Zephyr RTOS | Zephyr SDK (gcc per arch), LLVM | yes |

- **GCC/Clang toolchains producing ELF** (almost all of the above) work as-is. The
  library needs C++11 and weak symbols.
- **Keil (armclang/ARMCC), IAR, TASKING, TI CGT and Renesas CC-xx** are proprietary
  compilers. They mostly support weak symbols, but some only through their own keyword
  (`__weak`) or `#pragma weak`, so check that the compiler accepts the GNU attribute, as
  both the library and the pfodWeb generated code use it.
- **8-bit PIC (XC8) and pure-C toolchains** have no C++. Use the trivial
  [pfodParserC](https://www.forward.com.au/pfod/pfodParserLibraries/pfodParserC.html) instead.
- **`std::chrono`** is only relevant if you keep the template's default `micros()`. On
  bare-metal newlib and vendor C libraries it often doesn't work (it fails to link or
  always returns 0), so use the board's timer, as the examples do
  (see [Clock](#clock-micros-millis-and-yield)).

## Build

```sh
cmake -S . -B build
cmake --build build
```

This produces a static library `pfodParser` (`build/libpfodParser.a`).

To use it in your own CMake project:

```cmake
add_subdirectory(path/to/pfodParser)
target_link_libraries(your_target PRIVATE pfodParser)
```

`target_include_directories` on the `pfodParser` target already exposes `src/` as a
public include directory, so consumers can `#include "pfodParserCPP.h"` etc. directly.

## Manual build (without CMake)

```sh
g++ -std=c++11 -I src -c src/*.cpp src/dwgs/*.cpp
gcc -std=c11 -I src -c src/*.c
ar rcs libpfodParser.a *.o
```

Only `-I src` is needed on the include path — headers in `src/dwgs/` resolve their
own sibling includes, and files outside `dwgs/` reference it as `"dwgs/pfodXxx.h"`.

## Scope

`src/` contains only the core parser/drawing library. Hardware-specific transports
(BLE, GSM/SMS, ESP web/app servers, MAC/Radio, EEPROM, security handshake) were
part of the original Arduino library but are not included here — they depend on
specific hardware/platform APIs and are out of scope for a plain C++ port.

## Connecting to a UART on a microcontroller

`pfodParser` reads/writes through the abstract `pfodStream` class
(`src/pfodStream.h`, a `pfodPrint` subclass, `src/pfodPrint.h`); it has no
hardware setup of its own, by design (same as Arduino's `Stream`). To use it on
a microcontroller, write a small `pfodStream` subclass wrapping that platform's
native UART driver, following the pattern of Arduino's `HardwareSerial`:

* a constructor that does **not** touch the hardware (global objects are
  constructed before `main()`, i.e. before the board's clocks/pins are set up),
* a `begin()` doing the hardware bring-up (baud rate, pins, rx buffer, interrupts),
  called from your setup code,
* `write(uint8_t)` and, if output is buffered, `flush()`,
* `read()`, `available()` and `peek()`, all non-blocking.

Then call `begin()` and `pfodParser::connect(&yourStream)`. `write(buffer, size)`,
`write(const char*)` and `print()`/`println()` are inherited from `pfodPrint` and
all end up calling `write(uint8_t)`.

[`examples/`](examples/README.md) has the pfodWeb generated OnOff_Serial sketch
with its `target_Stream` (a `pfodStream` subclass) and `target_pfodClock.cpp`
(`micros()`) completed for each of:

* `examples/stm32/OnOff_Serial/` -- STM32 (STM32Cube HAL UART)
* `examples/nrf52/OnOff_Serial/` -- Nordic nRF52 (legacy nRF5 SDK `app_uart`)
* `examples/nrf54l_bm/OnOff_Serial/` -- Nordic nRF54L Series (nRF Connect SDK Bare Metal option, nrfx UARTE, low power)
* `examples/esp32/OnOff_Serial/` -- ESP32 (ESP-IDF)
* `examples/esp8266/OnOff_Serial/` -- ESP8266 (ESP8266_RTOS_SDK)
* `examples/zephyr/OnOff_Serial/` -- Zephyr RTOS
* `examples/pic32/OnOff_Serial/` -- Microchip PIC32 (MPLAB Harmony 3 UART PLIB)

**ESP8266: only ESP8266_RTOS_SDK is supported. The ESP8266 NonOS SDK is not
supported.** NonOS user code runs in SDK callbacks that must return before the
WiFi stack can run, so `pfodBufferedStream`'s blocking `write()`/`flush()` would
stall WiFi, and `yield()` has no way to hand control back to the SDK.

See [`examples/README.md`](examples/README.md) for details. These are not part of
the desktop CMake build above (they need each vendor's cross-toolchain/SDK)
and have only been compiled against stand-in SDK headers, not the real SDKs — check them against your
SDK version before relying on them.

## Clock: micros(), millis() and yield()

`src/pfodClock.h` declares the library's clock functions:

```cpp
unsigned long micros(); // us since startup -- the only clock source, supplied by your program
unsigned long millis(); // ms since startup -- derived from micros()
void yield();           // called while busy waiting -- empty default
```

`micros()` is **not** implemented in the library; your program must supply it,
e.g. in a `target_pfodClock.cpp` alongside `main.cpp`. The `millis()` and `yield()`
defaults in `src/pfodClock.cpp` are `__attribute__((weak))`, so defining your own
(non-weak) version in a `.cpp` file of your program replaces them at link time --
do not edit `pfodClock.cpp`.

`millis()` is used for the parser's message timeout and by `pfodDelay`,
`pfodWaitForUtils` and `pfodBufferedStream`; `micros()` by `pfodBufferedStream`
and `pfodRandom`. There is no `delay()`.

**micros()** -- supplied by your program. A generic implementation uses
`std::chrono::steady_clock`:

```cpp
// target_pfodClock.cpp
#include "pfodClock.h"
#include <chrono>
unsigned long micros() {
  static const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
  return (unsigned long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime).count();
}
```

That works where the C/C++ runtime has a real clock (e.g. ESP-IDF, Zephyr with
POSIX/libc time support, Linux). On many bare metal toolchains (e.g. arm-none-eabi
with newlib) it either fails to link (no `clock_gettime`/`_gettimeofday`) or always
returns 0, so timeouts never expire. In that case use the board's timer instead
(and drop `<chrono>`), e.g.

```cpp
// ESP-IDF (ESP32), ESP8266_RTOS_SDK
#include "esp_timer.h"
unsigned long micros() { return (unsigned long)esp_timer_get_time(); }

// Zephyr
#include <zephyr/kernel.h>
unsigned long micros() { return (unsigned long)k_ticks_to_us_floor64(k_uptime_ticks()); }

// STM32 HAL: 1 ms SysTick count plus the us into the current SysTick period,
// re-reading if the ms tick changed during the calculation
#include "main.h" // your CubeMX generated header
unsigned long micros() {
  uint32_t ms;
  uint32_t ticks;
  do {
    ms = HAL_GetTick();
    ticks = SysTick->LOAD - SysTick->VAL;
  } while (ms != HAL_GetTick());
  return (unsigned long)(ms * 1000UL + ticks / (SystemCoreClock / 1000000UL));
}

// bare metal: a free running 32 bit timer clocked at 1 MHz
unsigned long micros() { return (unsigned long)YOUR_TIMER_COUNT_REGISTER; }
```

The generated `target_pfodClock.cpp` has these as commented out examples, and each
example in [`examples/`](examples/README.md) has a completed `target_pfodClock.cpp`
for its platform (including nRF52 using a TIMER, and PIC32 using the core timer).

The library itself does not reference `std::chrono`, so once your `micros()` does
not use it, nothing pulls in `clock_gettime` / `_gettimeofday`.

Your `micros()` **must count through the full `unsigned long` range and wrap from
its maximum value back to 0** (every ~71.6 minutes for 32 bit), so that
`micros() - start` is correct across a wrap. Truncating a 64 bit count to
`unsigned long` does this; a cycle counter divided by cycles-per-us does not.

**millis()** -- already implemented from `micros()`; normally you do not replace it
(it is weak, so you can, e.g. if your board already has a suitable ms tick). It
accumulates the `micros()` elapsed since its previous call, so it keeps counting
correctly when `micros()` wraps, and itself wraps after ~49.7 days (32 bit). It must
be called at least once per `micros()` wrap period, which `pfodParser` does every
loop. It is not safe to call from an interrupt handler.

**yield()** -- the library has no `delay()`. Its only busy waits are in
`pfodBufferedStream`, and both call `yield()` repeatedly while waiting.  
**So yield() need not be implemented if `pfodBufferedStream` is not used.**:

* `write()`: in blocking mode, when the transmit buffer is full, it waits in 1 ms
  steps, timed with `millis()`, for space.
* `flush()`: waits until the buffer is empty, about (buffered bytes x one byte time
  at the baud rate).

`pfodBufferedStream` releases at most one byte per `yield()` call, so if your
`yield()` blocks (e.g. `vTaskDelay(1)`) the wait is correspondingly longer.

The default `yield()` in `pfodClock.cpp` is an empty function marked
`__attribute__((weak))`, so by default the wait just spins. On boards that must
run other things while waiting (e.g. an RTOS: task watchdog, WiFi stack), define
your own `yield()` in any `.cpp` file of your program, e.g. `target_pfodClock.cpp`
(the generated one has these examples commented out).

`yield()` is **not** defined by ESP-IDF, ESP8266_RTOS_SDK, FreeRTOS or Zephyr.
(Only the Arduino cores define one, and that is an `extern "C"` function, a
different symbol from this C++ `yield()`, so it would not replace the default.)

```cpp
// ESP-IDF (ESP32), ESP8266_RTOS_SDK, other FreeRTOS -- define ONE of these
#include "freertos/FreeRTOS.h" // plain FreeRTOS: "FreeRTOS.h"
#include "freertos/task.h"     // plain FreeRTOS: "task.h"
void yield() { vTaskDelay(1); } // block for 1 tick, lets every task run
void yield() { taskYIELD(); }   // equal/higher priority tasks only

// Zephyr -- define ONE of these
#include <zephyr/kernel.h>
void yield() { k_yield(); }     // equal/higher priority threads only
void yield() { k_msleep(1); }   // lower priority threads also run

// bare metal with a hardware watchdog enabled, e.g. STM32 HAL
void yield() { HAL_IWDG_Refresh(&hiwdg); }
```

`taskYIELD()` does not let the lowest priority IDLE task run, and IDLE is what
resets the FreeRTOS / ESP-IDF task watchdog, so use `vTaskDelay(1)` if the wait can
be long enough to trip the watchdog. With the ESP-IDF default 100 Hz tick,
`vTaskDelay(1)` blocks for 10 ms, so a blocked `write()`/`flush()` then sends only
~100 bytes/sec -- set `CONFIG_FREERTOS_HZ=1000` if that is too slow.

The ESP8266 NonOS SDK is not supported (see above) -- use ESP8266_RTOS_SDK and the
FreeRTOS `yield()`. On bare metal without a watchdog the default empty `yield()`
is fine.

Your (non-weak) `yield()` replaces the weak default at link time -- do not edit
`pfodClock.cpp` for this. This needs a toolchain with weak symbol support
(GCC/Clang producing ELF objects, as micro toolchains do; see
[Compilers](#compilers-tested--expected-to-work) above).

**nRF52 with a SoftDevice (nRF5 SDK):** the SoftDevice runs the radio from its own
highest priority interrupts, which preempt application code, so a busy wait in
`write()`/`flush()` does not stop the radio. But BLE *events* to your application
are delayed while it waits: if they are dispatched from an interrupt
(`NRF_SDH_DISPATCH_MODEL_INTERRUPT`, the default) they are delayed only if the wait
runs at that interrupt priority or higher; if they are dispatched with
`app_scheduler` (`NRF_SDH_DISPATCH_MODEL_APPSH`) they wait until your main loop
calls `app_sched_execute()` again. Do not call `app_sched_execute()` from `yield()`
-- an event handler could then re-enter `pfodBufferedStream`. On Zephyr (nRF Connect
SDK) use the Zephyr `yield()` above.
