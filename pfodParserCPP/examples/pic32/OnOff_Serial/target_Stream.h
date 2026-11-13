/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
#ifndef target_Stream_h
#define target_Stream_h
/*
 * pfodStream implementation for PIC32 (MPLAB Harmony 3 UART PLIB), NOT Arduino,
 * following the pattern of Arduino's HardwareSerial.
 * Wraps an MCC generated UARTx peripheral library (plib_uartx.h) configured in
 * "Ring buffer" mode.
 *
 * Use it for pfodParser::connect(pfodStream*), and for debug output via
 * setDebugPtr(pfodStream*). Since a pfodStream is a pfodPrint, it can also be
 * used wherever a pfodPrint* is wanted.
 *
 * In Ring buffer mode the PLIB does its own interrupt driven rx/tx buffering
 * (buffer sizes set in MCC), so no pfodRingBuffer is needed.
 * available() and peek() are implemented using a one byte lookAhead filled by read().
 *
 * MCC names every UART function after the peripheral instance (UART1_Read,
 * UART2_Read, ...) rather than taking an instance handle, so begin() takes the
 * UARTx_ functions for whichever instance you configured in MCC.
 *
 * Usage (in main.cpp, after SYS_Initialize() has called UART1_Initialize()):
 *   target_Stream serialStream;     // global, the constructor does not touch the hardware
 *   ...
 *   // in setup()
 *   serialStream.begin(UART1_Read, UART1_Write, UART1_ReadCountGet, UART1_WriteCountGet, UART1_TransmitComplete);
 *   parser.connect(&serialStream);
 *   setDebugPtr(&serialStream);     // optional, debug output on the same port
 *
 * Debug output can go out on the same port as the pfod messages. pfodWeb ignores
 * characters outside the {..} messages and logs them to the Raw Message Viewer.
 *
 * Not compiled/tested against the real Harmony PLIB -- the function signatures
 * below match Harmony 3's Ring buffer mode UART PLIB as documented, but check
 * them against the plib_uartx.h MCC generated for you.
 */

#include "pfodStream.h"

class target_Stream : public pfodStream {
  public:
    typedef size_t (*ReadFn)(uint8_t *pRdBuffer, const size_t size);   // UARTx_Read
    typedef size_t (*WriteFn)(uint8_t *pWrBuffer, const size_t size);  // UARTx_Write
    typedef size_t (*CountFn)(void);                                   // UARTx_ReadCountGet, UARTx_WriteCountGet
    typedef bool (*TransmitCompleteFn)(void);                          // UARTx_TransmitComplete

    /**
     * Constructor, only initialises member variables.
     * Does NOT access the hardware, since global objects are constructed
     * before main() runs.
     * Call begin() from setup() to start the port.
     */
    target_Stream();

    /**
     * Start using an MCC configured UART (Ring buffer mode), whose
     * UARTx_Initialize() has already run (normally via SYS_Initialize()).
     * Call once from setup() before using it, e.g. before parser.connect().
     * @param readFn UARTx_Read
     * @param writeFn UARTx_Write
     * @param readCountFn UARTx_ReadCountGet
     * @param writeCountFn UARTx_WriteCountGet
     * @param transmitCompleteFn UARTx_TransmitComplete
     */
    void begin(ReadFn readFn, WriteFn writeFn, CountFn readCountFn,
               CountFn writeCountFn, TransmitCompleteFn transmitCompleteFn);

    // pfodPrint methods
    // write(buf, size) is inherited from pfodPrint and calls write(uint8_t) for each byte.
    using pfodStream::write; // keep inherited write(buf, size) and write(const char*) visible

    /**
     * Write one byte into the PLIB's tx ring buffer, which is emptied by the
     * UART interrupt. If the tx ring buffer is full, blocks until there is space.
     * @param c the byte to send
     * @return 1 if the byte was queued, 0 if begin() has not been called.
     */
    size_t write(uint8_t c) override;

    /**
     * Wait until all bytes previously written have been sent, i.e. the PLIB's
     * tx ring buffer is empty and the UART has finished sending the last byte.
     */
    void flush() override;

    // pfodStream methods

    /**
     * Number of received bytes waiting to be read.
     * Implemented using the one byte lookAhead: if lookAhead is empty (-1),
     * calls read() and stores the result in lookAhead.
     * Does not block.
     * @return 1 if a byte is available (held in lookAhead), 0 if none.
     */
    int available() override;

    /**
     * Read and remove the next received byte.
     * Returns the lookAhead byte first, if available()/peek() has stored one,
     * otherwise reads from the PLIB's rx ring buffer.
     * Does not block.
     * @return the next byte (0..255) as an int, or -1 if no byte is available
     *         (including if begin() has not been called).
     */
    int read() override;

    /**
     * Return the next received byte WITHOUT removing it.
     * Implemented using available() to fill the one byte lookAhead.
     * Does not block.
     * @return the next byte (0..255) as an int, or -1 if no byte is available.
     */
    int peek() override;

  private:
    ReadFn readFn;
    WriteFn writeFn;
    CountFn readCountFn;
    CountFn writeCountFn;
    TransmitCompleteFn transmitCompleteFn;
    bool started;  // set true by begin(), no i/o until then
    int lookAhead; // one byte look ahead for available()/peek(), -1 if empty
};

#endif //target_Stream_h
