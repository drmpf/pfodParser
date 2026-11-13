/*
 * main.c — starting point generated for board: Minimal C code no Dwgs
for small memory non-Arduino uC
 * (c)2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
 * pfodParserStream.c is already completed for the ATtiny84 (interrupt driven software serial, uses Timer1).
 */
#include <avr/io.h>
#include "pfodMenu.h"

void main(void) {
  DDRA |= (1 << PA0); // PA0 output, starts low to match onoff_PA0_var = 0
  pfodParser_setup();
  while (1) {
    pfodParser_parse();
    // drive PA0 from the slider
    if (onoff_PA0_var) {
      PORTA |= (1 << PA0);
    } else {
      PORTA &= (uint8_t) ~(1 << PA0);
    }
  }
}
