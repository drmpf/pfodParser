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
 * pfodStream implementation for the Raspberry Pi RP2040 / RP2350 with the Pico SDK,
 * NOT Arduino, following the pattern of Arduino's HardwareSerial.
 * Wraps one of the UARTs, uart0 or uart1 (hardware/uart.h).
 *
 * Use it for pfodParser::connect(pfodStream*), and for debug output via
 * setDebugPtr(pfodStream*). Since a pfodStream is a pfodPrint, it can also be
 * used wherever a pfodPrint* is wanted.
 *
 * Receive is interrupt driven (the UART's RX interrupt) into a pfodRingBuffer,
 * so available(), read() and peek() just use the ring buffer.
 * write() uses uart_putc_raw(), which busy waits until the UART's 32 byte tx FIFO
 * has room; flush() waits until the FIFO and shift register are empty.
 *
 * Do not also use the same UART for Pico SDK stdio (stdio_init_all() /
 * pico_enable_stdio_uart), CMakeLists.txt turns stdio over UART off.
 *
 * Usage (in main.cpp):
 *   static uint8_t rxBuf[128];
 *   target_Stream serialStream;     // global, the constructor does not touch the hardware
 *   ...
 *   // in setup()
 *   serialStream.begin(uart0, 0, 1, 115200, rxBuf, sizeof(rxBuf)); // TX GP0, RX GP1
 *   parser.connect(&serialStream);
 *   setDebugPtr(&serialStream);     // optional, debug output on the same port
 *
 * Debug output can go out on the same port as the pfod messages. pfodWeb ignores
 * characters outside the {..} messages and logs them to the Raw Message Viewer.
 *
 * Compiled and linked against Pico SDK 2.3.0 (build.sh), not tested on hardware -- check against your SDK version.
 */

#include "pfodStream.h"
#include "pfodRingBuffer.h"
#include "hardware/uart.h"

class target_Stream : public pfodStream {
  public:
    /**
     * Constructor, only initialises member variables.
     * Does NOT access the hardware.
     * Call begin() from setup() to start the port.
     */
    target_Stream();

    /**
     * Start the UART: sets the pins to the UART function, initialises the UART
     * (8N1, no flow control, FIFOs on), initialises the rx ring buffer, installs the
     * UART's interrupt handler and enables the RX interrupt.
     * Call once from setup() before using it, e.g. before parser.connect().
     * @param uart uart0 or uart1
     * @param txPin the GPIO for TX, it must be one of this UART's TX pins,
     *              e.g. GP0 (uart0, the Pico's default) or GP4 (uart1)
     * @param rxPin the GPIO for RX, it must be one of this UART's RX pins,
     *              e.g. GP1 (uart0, the Pico's default) or GP5 (uart1)
     * @param baudRate the baud rate, e.g. 115200. The actual rate set is the
     *              nearest the UART clock allows.
     * @param rxBuf caller supplied storage for the rx ring buffer,
     *              must stay valid for the life of this stream
     * @param rxBufSize the size of rxBuf
     * Only one target_Stream per UART. begin() should only be called once.
     */
    void begin(uart_inst_t *uart, unsigned int txPin, unsigned int rxPin, unsigned int baudRate,
               uint8_t *rxBuf, size_t rxBufSize);

    // pfodPrint methods
    // write(buf, size) is inherited from pfodPrint and calls write(uint8_t) for each byte.
    using pfodStream::write; // keep inherited write(buf, size) and write(const char*) visible

    /**
     * Write one byte to the UART with uart_putc_raw(), which busy waits
     * until the tx FIFO has room.
     * @param c the byte to send
     * @return 1 if the byte was sent, 0 if begin() has not been called.
     */
    size_t write(uint8_t c) override;

    /**
     * Wait until all bytes previously written have been sent,
     * i.e. the tx FIFO and shift register are empty (uart_tx_wait_blocking()).
     * Does NOT discard received data.
     */
    void flush() override;

    // pfodStream methods

    /**
     * Number of received bytes waiting in the rx ring buffer.
     * @return the number of bytes available, 0 if none (or not started).
     */
    int available() override;

    /**
     * Read and remove the next received byte from the rx ring buffer.
     * Does not block.
     * @return the next byte (0..255) as an int, or -1 if no byte is available.
     */
    int read() override;

    /**
     * Return the next received byte WITHOUT removing it.
     * Does not block.
     * @return the next byte (0..255) as an int, or -1 if no byte is available.
     */
    int peek() override;

    /**
     * Called from the UART interrupt handler.
     * Moves all the bytes in the UART rx FIFO into the rx ring buffer
     * (dropped if it is full). Not for direct use.
     */
    void onUartInterrupt();

  private:
    uart_inst_t *uart; // set by begin(), NULL until then, no i/o until then
    pfodRingBuffer rxRingBuffer;
};

#endif //target_Stream_h
