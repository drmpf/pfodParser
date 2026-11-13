/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
/*
 * target_Pin.cpp -- the output pin the OnOff_Serial menu turns on and off,
 * for the Raspberry Pi RP2040 / RP2350 with the Pico SDK, using hardware/gpio.h.
 *
 * The default is PICO_DEFAULT_LED_PIN from your board's header (selected by PICO_BOARD),
 * e.g. GP25 on the Pico and Pico 2, lit when the pin is high. If the board header
 * defines PICO_DEFAULT_LED_PIN_INVERTED 1 the LED is lit when the pin is low.
 * The Pico W / Pico 2 W LED is on the CYW43 wifi chip, not a GPIO, so those boards
 * have no PICO_DEFAULT_LED_PIN; set PFOD_PIN to a GPIO with an LED on it instead.
 * Set PFOD_PIN and PFOD_PIN_ON_LEVEL for your board, e.g. add
 *   target_compile_definitions(OnOff_Serial PRIVATE PFOD_PIN=15 PFOD_PIN_ON_LEVEL=0)
 * to CMakeLists.txt , OR edit the defines below,
 * for an LED on GP15 that is lit when the pin is low.
 * Do not use the pfod UART's pins (GP0 / GP1 by default, see main.cpp).
 *
 * Compiled and linked against Pico SDK 2.3.0 (build.sh), not tested on hardware -- check against your SDK version.
 */
#include "target_Pin.h"
#include "pico/stdlib.h" // the board header, PICO_DEFAULT_LED_PIN
#include "hardware/gpio.h"

#ifndef PFOD_PIN
#ifndef PICO_DEFAULT_LED_PIN
#error "This board has no PICO_DEFAULT_LED_PIN (e.g. Pico W, whose LED is on the wifi chip), define PFOD_PIN"
#endif
#define PFOD_PIN PICO_DEFAULT_LED_PIN
#endif

#ifndef PFOD_PIN_ON_LEVEL
#if defined(PICO_DEFAULT_LED_PIN_INVERTED) && PICO_DEFAULT_LED_PIN_INVERTED
#define PFOD_PIN_ON_LEVEL 0 // the board's LED is lit when the pin is low
#else
#define PFOD_PIN_ON_LEVEL 1 // the pin level for on, 1 = high, 0 = low
#endif
#endif

/**
 * Initialise PFOD_PIN as a GPIO, set its output level to off, then make it an output.
 * The level is set first so the pin does not glitch on.
 * The gpio_ calls return nothing, there is no failure to report.
 */
void target_PinInit() {
  gpio_init(PFOD_PIN);
  target_PinWrite(false);
  gpio_set_dir(PFOD_PIN, GPIO_OUT);
}

/**
 * Set PFOD_PIN to PFOD_PIN_ON_LEVEL for on, or the other level for off.
 * @param on true to turn the output on, false to turn it off
 */
void target_PinWrite(bool on) {
  gpio_put(PFOD_PIN, on ? (PFOD_PIN_ON_LEVEL != 0) : (PFOD_PIN_ON_LEVEL == 0));
}
