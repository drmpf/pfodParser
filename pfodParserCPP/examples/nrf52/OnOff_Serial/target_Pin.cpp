/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
/*
 * target_Pin.cpp -- the output pin the OnOff_Serial menu turns on and off,
 * for nRF52 with the nRF5 SDK, using the nrf_gpio HAL (hal/nrf_gpio.h).
 *
 * The default is LED_1 with LEDS_ACTIVE_STATE, from your board's bsp header via
 * boards.h (as main.cpp uses for RX_PIN_NUMBER / TX_PIN_NUMBER), e.g. for the
 * nRF52 DK (BOARD_PCA10040) LED_1 is P0.17, lit when the pin is low (LEDS_ACTIVE_STATE 0),
 * for the nRF52840 DK (BOARD_PCA10056) LED_1 is P0.13, also active low.
 * Set PFOD_PIN and PFOD_PIN_ON_LEVEL for your board, e.g. add
 *   -DPFOD_PIN=NRF_GPIO_PIN_MAP(1,10) -DPFOD_PIN_ON_LEVEL=1
 * , OR edit the defines below,
 * to your build for an LED on P1.10 that is lit when the pin is high.
 * Do not use RX_PIN_NUMBER / TX_PIN_NUMBER (the pfod UART, see main.cpp).
 *
 * Compiled and linked against nRF5 SDK 17.1.0 (build.sh), not tested on hardware -- check against your SDK version.
 */
#include "target_Pin.h"
#include "nrf_gpio.h"
#include "boards.h" // LED_1, LEDS_ACTIVE_STATE for your board

#ifndef PFOD_PIN
#define PFOD_PIN LED_1
#endif
#ifndef PFOD_PIN_ON_LEVEL
#define PFOD_PIN_ON_LEVEL LEDS_ACTIVE_STATE // the pin level for on, 1 = high, 0 = low
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
