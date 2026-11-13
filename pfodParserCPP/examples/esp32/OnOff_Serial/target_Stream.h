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
 * pfodStream implementation for ESP32 (ESP-IDF) and ESP8266 (ESP8266_RTOS_SDK),
 * NOT Arduino, following the pattern of Arduino's HardwareSerial.
 * Wraps the SDK's driver/uart.h UART driver. Both SDKs have the same
 * driver/uart.h API used here (uart_param_config / uart_driver_install /
 * uart_write_bytes / uart_read_bytes / uart_wait_tx_done), so this one
 * implementation covers both. ESP8266_RTOS_SDK has no uart_set_pin() (the
 * ESP8266 UART pins are fixed), so begin() ignores txPin/rxPin there.
 * The ESP8266 NonOS SDK is NOT supported.
 *
 * Use it for pfodParser::connect(pfodStream*), and for debug output via
 * setDebugPtr(pfodStream*). Since a pfodStream is a pfodPrint, it can also be
 * used wherever a pfodPrint* is wanted.
 *
 * One instance per UART port. The constructor does NOT access the hardware.
 * begin() initialises the port.
 *
 * uart_driver_install() gives the UART its own interrupt fed rx buffer, so no
 * pfodRingBuffer is needed. available() and peek() are implemented using a one
 * byte lookAhead filled by read(), as the driver has no non-destructive peek.
 *
 * Usage (in main.cpp):
 *   target_Stream serialStream;     // global, the constructor does not touch the hardware
 *   ...
 *   // in setup()
 *   serialStream.begin(UART_NUM_1, 115200, 17, 16); // port, baud, tx pin, rx pin
 *   parser.connect(&serialStream);
 *   setDebugPtr(&serialStream);     // optional, debug output on the same port
 *
 * Debug output can go out on the same port as the pfod messages. pfodWeb ignores
 * characters outside the {..} messages and logs them to the Raw Message Viewer.
 *
 * Compiled and linked against ESP-IDF v5.5.5 (build.sh), not tested on hardware -- check against your SDK version.
 */

#include "pfodStream.h"
#include "sdkconfig.h" // CONFIG_IDF_TARGET_ESP8266 on ESP8266_RTOS_SDK
#include "driver/uart.h"

class target_Stream : public pfodStream {
  public:
    /**
     * Constructor, only initialises member variables.
     * Does NOT access the hardware, since global objects are constructed
     * before app_main() runs.
     * Call begin() from setup() to initialise the port.
     */
    target_Stream();

    /**
     * Initialise the UART and install the SDK's UART driver.
     * Call once from setup() before using it, e.g. before parser.connect().
     * Uses 8 data bits, no parity, 1 stop bit, no flow control.
     * @param port the UART, e.g. UART_NUM_0, UART_NUM_1
     * @param baudRate the serial baud rate, e.g. 115200
     * @param txPin tx GPIO, UART_PIN_NO_CHANGE keeps the port's default pin (ignored on ESP8266)
     * @param rxPin rx GPIO, UART_PIN_NO_CHANGE keeps the port's default pin (ignored on ESP8266)
     * @param rxBufSize driver rx buffer size, must be > 128 (the UART hardware fifo size)
     * @param txBufSize driver tx buffer size, 0 or > 128.
     *        0 makes write() block until the bytes are in the hardware fifo.
     * If uart_driver_install() fails, the stream stays not started, write()
     * returns 0 and read() returns -1.
     */
    void begin(uart_port_t port, uint32_t baudRate,
               int txPin = UART_PIN_NO_CHANGE, int rxPin = UART_PIN_NO_CHANGE,
               int rxBufSize = 256, int txBufSize = 256);

    // pfodPrint methods
    using pfodStream::write; // keep inherited write(const char*) visible

    /**
     * Write one byte to the UART.
     * uart_write_bytes() blocks until there is space in the driver's tx buffer.
     * @param c the byte to send
     * @return 1 if the byte was queued for sending, 0 if not
     *         (including if begin() has not been called / failed).
     */
    size_t write(uint8_t c) override;

    /**
     * Write a block of bytes to the UART with one driver call, faster than
     * the inherited byte by byte version.
     * Blocks until all the bytes are in the driver's tx buffer.
     * @param buf the bytes to send
     * @param size the number of bytes
     * @return the number of bytes queued, 0 if not started or on a driver error
     */
    size_t write(const uint8_t *buf, size_t size) override;

    /**
     * Wait until all bytes previously written have actually been sent,
     * using uart_wait_tx_done().
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
     * otherwise reads from the driver's rx buffer with a 0 timeout.
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
    uart_port_t uartPort; // the UART passed to begin()
    bool started;  // set true by begin() if the driver installed, no i/o until then
    int lookAhead; // one byte look ahead for available()/peek(), -1 if empty
};

#endif //target_Stream_h
