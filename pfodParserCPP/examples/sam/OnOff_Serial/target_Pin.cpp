/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
/*
 * target_Pin.cpp -- the output pin the OnOff_Serial menu turns on and off,
 * for Microchip SAM with MPLAB Harmony 3, using the MCC generated PORT PLIB
 * (plib_port.h, included by definitions.h).
 *
 * TODO: in MCC's Pin Configuration, set the LED pin's function to GPIO, direction Out,
 * and name it LED. MCC then generates LED_PIN (e.g. PORT_PIN_PB30) in plib_port.h,
 * which is the default PFOD_PIN below. The LED pin differs from board to board,
 * see your board's user guide.
 * PFOD_PIN_ON_LEVEL defaults to 1, for an LED lit when the pin is high.
 * Set PFOD_PIN and PFOD_PIN_ON_LEVEL for your board, e.g. add
 *   -DPFOD_PIN=PORT_PIN_PB30 -DPFOD_PIN_ON_LEVEL=0
 * to the project's compiler defines, OR edit the defines below,
 * for the SAM D21 Xplained Pro's LED0, on PB30, lit when the pin is low.
 * Do not use the pfod SERCOM's pins (see main.cpp).
 *
 * Compiled and linked against MCC Harmony 3 generated code for the SAM D21 Xplained Pro
 * (build.sh), not tested on hardware -- check against the plib_port.h MCC generated for you.
 */
#include "target_Pin.h"
extern "C" {
#include "definitions.h" // MCC/Harmony generated master include: PORT_PinWrite, LED_PIN etc
}

#ifndef PFOD_PIN
#define PFOD_PIN LED_PIN // the MCC pin named LED
#endif
#ifndef PFOD_PIN_ON_LEVEL
#define PFOD_PIN_ON_LEVEL 1 // the pin level for on, 1 = high, 0 = low
#endif

/**
 * Set PFOD_PIN's output latch to off, then make it an output.
 * The latch is set first so the pin does not glitch on.
 * MCC's PORT_Initialize() (from SYS_Initialize()) has normally already made it an
 * output, if it is configured as Out in MCC; making it an output again does no harm.
 * The PORT PLIB calls return nothing, there is no failure to report.
 */
void target_PinInit() {
  target_PinWrite(false);
  PORT_PinOutputEnable(PFOD_PIN);
}

/**
 * Set PFOD_PIN to PFOD_PIN_ON_LEVEL for on, or the other level for off.
 * @param on true to turn the output on, false to turn it off
 */
void target_PinWrite(bool on) {
  PORT_PinWrite(PFOD_PIN, on ? (PFOD_PIN_ON_LEVEL != 0) : (PFOD_PIN_ON_LEVEL == 0));
}
