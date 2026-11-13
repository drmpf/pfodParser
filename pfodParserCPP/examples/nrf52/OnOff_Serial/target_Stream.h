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
 * pfodStream implementation for Nordic nRF52 (nRF5 SDK), NOT Arduino, NOT Zephyr,
 * following the pattern of Arduino's HardwareSerial.
 * Wraps the SDK's app_uart (FIFO buffered UART) module.
 *
 * Use it for pfodParser::connect(pfodStream*), and for debug output via
 * setDebugPtr(pfodStream*). Since a pfodStream is a pfodPrint, it can also be
 * used wherever a pfodPrint* is wanted.
 *
 * Requires the nRF5 SDK's app_uart_fifo component (app_uart_fifo.c,
 * app_fifo.c) and its nrf_drv_uart / nrfx_uarte driver in your project --
 * these are included in every nRF5 SDK UART example.
 * app_uart is a single instance module, so only one target_Stream can be used.
 *
 * begin() calls app_uart_init() directly (the function the APP_UART_FIFO_INIT
 * macro wraps) rather than that macro, because the macro needs compile time
 * buffer sizes; this lets the caller supply its own buffers.
 *
 * available() and peek() are implemented using a one byte lookAhead filled by
 * read(), since app_uart has no peek or received byte count.
 *
 * Usage (in main.cpp):
 *   uint8_t rxBuf[64], txBuf[64];   // sizes must be powers of 2
 *   target_Stream serialStream;     // global, the constructor does not touch the hardware
 *   ...
 *   // in setup()
 *   serialStream.begin(RX_PIN_NUMBER, TX_PIN_NUMBER, rxBuf, sizeof(rxBuf), txBuf, sizeof(txBuf));
 *   parser.connect(&serialStream);
 *   setDebugPtr(&serialStream);     // optional, debug output on the same port
 *
 * Debug output can go out on the same port as the pfod messages. pfodWeb ignores
 * characters outside the {..} messages and logs them to the Raw Message Viewer.
 *
 * Compiled and linked against nRF5 SDK 17.1.0 (build.sh), not tested on hardware.
 * app_uart_comm_params_t's field order/names have drifted a little across SDK
 * versions (12/15/16/17), so check
 * against the SDK version you are using. app_uart_init() with an explicit
 * app_uart_buffers_t is available from nRF5 SDK 12 onward.
 */

#include "pfodStream.h"
#include "app_uart.h"

class target_Stream : public pfodStream {
  public:
    /**
     * Constructor, only initialises member variables.
     * Does NOT access the hardware, since global objects are constructed
     * before main() runs.
     * Call begin() from setup() to initialise the port.
     */
    target_Stream();

    /**
     * Initialise the UART with app_uart_init(), 8N1, no parity.
     * Call once from setup() before using it, e.g. before parser.connect().
     * @param rxPin the rx pin number
     * @param txPin the tx pin number
     * @param rxBuf caller supplied storage for app_uart's rx fifo, size must be a
     *              power of 2, must stay valid for the life of this stream
     * @param rxBufSize the size of rxBuf
     * @param txBuf caller supplied storage for app_uart's tx fifo, as for rxBuf
     * @param txBufSize the size of txBuf
     * @param baudRate one of the UART_BAUDRATE_BAUDRATE_Baudxxx register values,
     *                 default 115200
     * @param rtsPin, ctsPin hardware flow control pins, leave as
     *                UART_PIN_DISCONNECTED for no flow control
     * If app_uart_init() fails, the stream stays not started, write()
     * returns 0 and read() returns -1.
     */
    void begin(uint32_t rxPin, uint32_t txPin,
               uint8_t *rxBuf, uint16_t rxBufSize,
               uint8_t *txBuf, uint16_t txBufSize,
               uint32_t baudRate = UART_BAUDRATE_BAUDRATE_Baud115200,
               uint32_t rtsPin = UART_PIN_DISCONNECTED, uint32_t ctsPin = UART_PIN_DISCONNECTED);

    // pfodPrint methods
    // write(buf, size) is inherited from pfodPrint and calls write(uint8_t) for each byte.
    using pfodStream::write; // keep inherited write(buf, size) and write(const char*) visible

    /**
     * Write one byte into app_uart's tx fifo, which is emptied by the UART interrupt.
     * If the tx fifo is full, blocks until there is space.
     * @param c the byte to send
     * @return 1 if the byte was queued, 0 if not (including if begin() has not been called / failed).
     */
    size_t write(uint8_t c) override;

    /**
     * Wait until all bytes previously written have been sent.
     * app_uart has no call to wait for its tx fifo to empty, so this does nothing,
     * see target_Stream.cpp.
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
     * otherwise reads from app_uart's rx fifo.
     * Does not block.
     * @return the next byte (0..255) as an int, or -1 if no byte is available
     *         (including if begin() has not been called / failed).
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
    bool started;  // set true by begin() if app_uart_init() succeeded, no i/o until then
    int lookAhead; // one byte look ahead for available()/peek(), -1 if empty
};

#endif //target_Stream_h
