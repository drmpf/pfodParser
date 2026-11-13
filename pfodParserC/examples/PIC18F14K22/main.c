/*
 * main.c — starting point generated for board: Minimal C code no Dwgs
for small memory non-Arduino uC
 * (c)2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
 * pfodParserStream.c is already completed for the PIC18F14K22 EUSART (MCC generated driver).
 */
#include "pfodMenu.h"
#include "mcc_generated_files/mcc.h"

void main(void) {
  SYSTEM_Initialize(); // MCC generated clock, pin and EUSART setup
  INTERRUPT_GlobalInterruptEnable(); // the MCC EUSART driver is interrupt driven
  INTERRUPT_PeripheralInterruptEnable();
  LATCbits.LATC0 = 0; // RC0 starts low to match onoff_RC0_var = 0
  TRISCbits.TRISC0 = 0; // RC0 output
  pfodParser_setup();
  while (1) {
    pfodParser_parse();
    LATCbits.LATC0 = onoff_RC0_var ? 1 : 0; // drive RC0 from the slider
  }
}
