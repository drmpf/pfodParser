/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
#ifndef TARGET_PIN_H
#define TARGET_PIN_H
/*
 * target_Pin.h -- the output pin that the OnOff_Serial menu turns on and off.
 *
 * Implemented for each board in target_Pin.cpp, using the board's own SDK
 * GPIO API. Used by OnOff_SerialApp.cpp, which subclasses the generated menu
 * and drawing classes to drive this pin.
 */

/**
 * Configure the output pin as a push-pull output and turn it off.
 * Call once, before target_PinWrite(). Called from pfodMainMenuApp::init().
 */
void target_PinInit();

/**
 * Turn the output pin on or off.
 * target_Pin.cpp maps on/off to the pin's high/low level, so an active low LED
 * (lit when the pin is low) still lights for on.
 * @param on true to turn the output on, false to turn it off
 */
void target_PinWrite(bool on);

#endif
