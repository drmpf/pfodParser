/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
/*
 * target_Pin.cpp -- the output pin the OnOff_Serial menu turns on and off,
 * for NXP MCX with the MCUXpresso SDK, using fsl_port.h (pin mux) and fsl_gpio.h.
 *
 * The default is the red LED from the SDK's board.h: BOARD_LED_RED_GPIO / _PIN, lit when
 * the pin is LOGIC_LED_ON. On the FRDM-MCXN947 that is P0_10, lit when low.
 * PFOD_PIN_PORT and PFOD_PIN_PORT_CLOCK, the pin's PORT and its clock gate, default to
 * PORT0 / kCLOCK_Port0 to match (board.h has no PORT define for the LEDs).
 * GPIO_PinInit() enables the GPIO's own clock.
 * Set PFOD_PIN_GPIO, PFOD_PIN_PORT, PFOD_PIN_PORT_CLOCK, PFOD_PIN and PFOD_PIN_ON_LEVEL
 * for another pin, e.g. add
 *   -DPFOD_PIN_GPIO=GPIO1 -DPFOD_PIN_PORT=PORT1 -DPFOD_PIN_PORT_CLOCK=kCLOCK_Port1 -DPFOD_PIN=2U
 * to the build , OR edit the defines below, for the FRDM-MCXN947's blue LED, P1_2.
 * Do not use the pfod LPUART's pins (P1_8 / P1_9, see main.cpp).
 *
 * Compiled and linked against the MCUXpresso SDK (mcuxsdk-core, mcux-devices-mcx,
 * mcuxsdk-examples board files) for the FRDM-MCXN947 (build.sh), not tested on hardware
 * -- check against your SDK version.
 */
#include "target_Pin.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#include "board.h" // BOARD_LED_RED_GPIO etc and LOGIC_LED_ON for your board

#ifndef PFOD_PIN_GPIO
#define PFOD_PIN_GPIO BOARD_LED_RED_GPIO
#endif
#ifndef PFOD_PIN_PORT
#define PFOD_PIN_PORT PORT0 // the PORT for PFOD_PIN_GPIO
#endif
#ifndef PFOD_PIN_PORT_CLOCK
#define PFOD_PIN_PORT_CLOCK kCLOCK_Port0 // the clock gate for PFOD_PIN_PORT
#endif
#ifndef PFOD_PIN
#define PFOD_PIN BOARD_LED_RED_GPIO_PIN
#endif
#ifndef PFOD_PIN_ON_LEVEL
#define PFOD_PIN_ON_LEVEL LOGIC_LED_ON // the pin level for on, 1 = high, 0 = low
#endif

/**
 * Enable the pin's PORT clock, mux it as a GPIO, then make it an output, off.
 * GPIO_PinInit() enables the GPIO clock and sets the output level before making the
 * pin an output, so the pin does not glitch on.
 * The SDK calls used here return nothing, there is no failure to report.
 */
void target_PinInit() {
  CLOCK_EnableClock(PFOD_PIN_PORT_CLOCK);
  PORT_SetPinMux(PFOD_PIN_PORT, PFOD_PIN, kPORT_MuxAsGpio);
  gpio_pin_config_t pinConfig = { kGPIO_DigitalOutput, (uint8_t)((PFOD_PIN_ON_LEVEL != 0) ? 0U : 1U) };
  GPIO_PinInit(PFOD_PIN_GPIO, PFOD_PIN, &pinConfig);
}

/**
 * Set PFOD_PIN to PFOD_PIN_ON_LEVEL for on, or the other level for off.
 * @param on true to turn the output on, false to turn it off
 */
void target_PinWrite(bool on) {
  GPIO_PinWrite(PFOD_PIN_GPIO, PFOD_PIN, (uint8_t)(on ? (PFOD_PIN_ON_LEVEL != 0) : (PFOD_PIN_ON_LEVEL == 0)));
}
