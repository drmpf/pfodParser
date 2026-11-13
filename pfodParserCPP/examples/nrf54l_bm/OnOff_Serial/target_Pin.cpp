/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
/*
 * target_Pin.cpp -- the output pin the OnOff_Serial menu turns on and off,
 * for the nRF54L Series with the nRF Connect SDK Bare Metal option, using the
 * nrf_gpio HAL (hal/nrf_gpio.h), as the Bare Metal samples drive the board LEDs.
 *
 * The default is BOARD_PIN_LED_0 with BOARD_LED_ACTIVE_STATE, from the board's
 * board-config.h (as target_Stream.cpp uses for the UARTE pins), e.g. for the
 * nRF54L15 DK (bm_nrf54l15dk) BOARD_PIN_LED_0 is P2.09 (LED0), lit when the pin is
 * high (BOARD_LED_ACTIVE_STATE 1).
 * Set PFOD_PIN and PFOD_PIN_ON_LEVEL for your board, e.g. add
 *   zephyr_compile_definitions(PFOD_PIN=NRF_PIN_PORT_TO_PIN_NUMBER(4,1) PFOD_PIN_ON_LEVEL=0)
 * to CMakeLists.txt , OR edit the defines below,
 * for an LED on P1.04 that is lit when the pin is low.
 * Do not use BOARD_APP_UARTE_PIN_TX / BOARD_APP_UARTE_PIN_RX (the pfod UARTE, see target_Stream.cpp).
 *
 * Compiled and linked against sdk-nrf-bm v2.0.1 (build.sh), not tested on hardware -- check against your sdk-nrf-bm version.
 */
#include "target_Pin.h"
#include <hal/nrf_gpio.h>
#include <board-config.h> // BOARD_PIN_LED_0, BOARD_LED_ACTIVE_STATE for your board

#ifndef PFOD_PIN
#define PFOD_PIN BOARD_PIN_LED_0
#endif
#ifndef PFOD_PIN_ON_LEVEL
#define PFOD_PIN_ON_LEVEL BOARD_LED_ACTIVE_STATE // the pin level for on, 1 = high, 0 = low
#endif

/**
 * Set PFOD_PIN's output level to off, then configure it as an output
 * (standard drive, input buffer disconnected, no pull).
 * The level is set first so the pin does not glitch on.
 * nrf_gpio_cfg_output() returns nothing, there is no failure to report.
 */
void target_PinInit() {
  target_PinWrite(false);
  nrf_gpio_cfg_output(PFOD_PIN);
}

/**
 * Set PFOD_PIN to PFOD_PIN_ON_LEVEL for on, or the other level for off.
 * @param on true to turn the output on, false to turn it off
 */
void target_PinWrite(bool on) {
  nrf_gpio_pin_write(PFOD_PIN, on ? PFOD_PIN_ON_LEVEL : !PFOD_PIN_ON_LEVEL);
}
