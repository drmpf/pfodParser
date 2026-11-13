/**
 * This is the interface to the i/o
 * Modify this file to suit your serial/uart connection code
 * This UART code is for PIC18F14K22 and similar
 * 
 */
#include "pfodParserStream.h"
#include "mcc_generated_files/mcc.h"

extern uint8_t pfodParser_connected; // set true (1) when {.} parsed, set false (0) when {!} parsed.

/**
 * do any Stream/UART setup required, called from pfodParser_pfodParser() 
 */ 
void pfodParserStream_init(void) {
  // empty for PIC18F14K22
}

/**
 * return space available for write();
 * @return bytes free, if space is available any non-zero return will do
 */
size_t pfodParser_TXfree(void) {
    return eusartTxBufferRemaining;
} 

int pfodParser_write(uint8_t c) {
    if (!pfodParser_connected) {
        return 1;
    }
    while (pfodParser_TXfree() == 0) {
        // spin
    }
    putch((char)c); // spins if no space available
    return 1;
}

/**
 * return bytes available to read()
 * @return bytes available, if bytes are available any non-zero return will do
 */
size_t pfodParser_RXavailable(void) {
    return eusartRxCount;
}

int pfodParser_read(void) {
    while (pfodParser_RXavailable() == 0) {
        // spin
    }
    return (int)getch(); // spins if not data available
}


