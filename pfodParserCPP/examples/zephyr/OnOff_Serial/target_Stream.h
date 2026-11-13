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
 * pfodStream implementation for Zephyr RTOS, NOT Arduino,
 * following the pattern of Arduino's HardwareSerial.
 * Wraps a Zephyr UART device.
 *
 * Use it for pfodParser::connect(pfodStream*), and for debug output via
 * setDebugPtr(pfodStream*). Since a pfodStream is a pfodPrint, it can also be
 * used wherever a pfodPrint* is wanted.
 *
 * Requires CONFIG_UART_INTERRUPT_DRIVEN=y in your prj.conf, for the
 * uart_irq_* interrupt driven API used here.
 * The baud rate etc are set by the devicetree (current-speed) for the UART.
 *
 * Receive is interrupt driven into a pfodRingBuffer, so available(), read()
 * and peek() just use the ring buffer.
 * write() uses uart_poll_out(), which busy waits until the UART has room and
 * the byte is in the hardware, so flush() has nothing to wait for.
 *
 * The Zephyr UART header to include is chosen by PFOD_ZEPHYR_UART_INCLUDE,
 * default "zephyr/drivers/uart.h" (Zephyr >= 3.0). Override to
 * "drivers/uart.h" for older (pre 3.0) Zephyr versions, e.g. add
 * -DPFOD_ZEPHYR_UART_INCLUDE=\"drivers/uart.h\" to your build.
 *
 * Usage (in main.cpp):
 *   uint8_t rxBuf[128];
 *   target_Stream serialStream;     // global, the constructor does not touch the hardware
 *   ...
 *   // in setup()
 *   serialStream.begin(DEVICE_DT_GET(DT_CHOSEN(zephyr_console)), rxBuf, sizeof(rxBuf));
 *   parser.connect(&serialStream);
 *   setDebugPtr(&serialStream);     // optional, debug output on the same port
 *
 * Debug output can go out on the same port as the pfod messages. pfodWeb ignores
 * characters outside the {..} messages and logs them to the Raw Message Viewer.
 *
 * Compiled and linked for the nRF52840 DK against Zephyr v4.5.0 (build.sh) and the Zephyr in nRF Connect SDK v3.4.1, not tested on hardware -- check against your Zephyr version.
 */

#include "pfodStream.h"
#include "pfodRingBuffer.h"

#ifndef PFOD_ZEPHYR_UART_INCLUDE
#define PFOD_ZEPHYR_UART_INCLUDE "zephyr/drivers/uart.h"
#endif
#include PFOD_ZEPHYR_UART_INCLUDE

class target_Stream : public pfodStream {
  public:
    /**
     * Constructor, only initialises member variables.
     * Does NOT access the hardware.
     * Call begin() from setup() to start the port.
     */
    target_Stream();

    /**
     * Start using a Zephyr UART device.
     * Initialises the rx ring buffer, installs the UART interrupt callback and
     * enables the rx interrupt.
     * Call once from setup() before using it, e.g. before parser.connect().
     * @param uartDev the UART device, e.g. DEVICE_DT_GET(DT_CHOSEN(zephyr_console))
     * @param rxBuf caller supplied storage for the rx ring buffer,
     *              must stay valid for the life of this stream
     * @param rxBufSize the size of rxBuf
     * If uartDev is not ready (device_is_ready() false), the stream stays not
     * started, write() returns 0 and read() returns -1.
     */
    void begin(const struct device *uartDev, uint8_t *rxBuf, size_t rxBufSize);

    // pfodPrint methods
    // write(buf, size) is inherited from pfodPrint and calls write(uint8_t) for each byte.
    using pfodStream::write; // keep inherited write(buf, size) and write(const char*) visible

    /**
     * Write one byte to the UART with uart_poll_out(), which busy waits
     * until the UART has room.
     * @param c the byte to send
     * @return 1 if the byte was sent, 0 if not (including if begin() has not been called / failed).
     */
    size_t write(uint8_t c) override;

    /**
     * Wait until all bytes previously written have been sent.
     * write() blocks until each byte is in the UART hardware, so there is nothing to wait for.
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
     * Called from the UART interrupt callback when received data is ready.
     * Moves all the bytes in the UART rx fifo into the rx ring buffer
     * (dropped if it is full). Not for direct use.
     */
    void onUartInterrupt();

  private:
    const struct device *uartDev; // set by begin() if the device is ready, NULL until then, no i/o until then
    pfodRingBuffer rxRingBuffer;
};

#endif //target_Stream_h
