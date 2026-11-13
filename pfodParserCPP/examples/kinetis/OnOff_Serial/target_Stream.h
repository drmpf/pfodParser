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
 * pfodStream implementation for NXP Kinetis K parts with the MCUXpresso SDK UART
 * driver (fsl_uart.h), NOT Arduino, following the pattern of Arduino's HardwareSerial.
 * e.g. the FRDM-K64F's UART0, the OpenSDA virtual COM port.
 * (Parts with an LPUART instead, e.g. Kinetis KL / K32L / KE1x, i.MX RT, MCX, need the
 * same class written with fsl_lpuart.h's LPUART_ functions.)
 *
 * Use it for pfodParser::connect(pfodStream*), and for debug output via
 * setDebugPtr(pfodStream*). Since a pfodStream is a pfodPrint, it can also be
 * used wherever a pfodPrint* is wanted.
 *
 * Receive is interrupt driven (the UART's RX data register full interrupt) into a
 * pfodRingBuffer, so available(), read() and peek() just use the ring buffer.
 * The interrupt handler is defined in target_Stream.cpp, named by PFOD_UART_IRQHandler,
 * default UART0_RX_TX_IRQHandler (UART0's handler in the SDK's startup vector table).
 * For another UART add e.g. -DPFOD_UART_IRQHandler=UART1_RX_TX_IRQHandler to the build.
 * write() uses UART_WriteBlocking(), which busy waits until the UART has room;
 * flush() waits until the last byte has been sent (transmission complete).
 *
 * Usage (in main.cpp, after BOARD_InitPins() has muxed the UART pins and
 * BOARD_BootClockRUN() has set the clocks):
 *   static uint8_t rxBuf[128];
 *   target_Stream serialStream;     // global, the constructor does not touch the hardware
 *   ...
 *   // in setup()
 *   serialStream.begin(UART0, UART0_RX_TX_IRQn, CLOCK_GetFreq(UART0_CLK_SRC), 115200, rxBuf, sizeof(rxBuf));
 *   parser.connect(&serialStream);
 *   setDebugPtr(&serialStream);     // optional, debug output on the same port
 *
 * Debug output can go out on the same port as the pfod messages. pfodWeb ignores
 * characters outside the {..} messages and logs them to the Raw Message Viewer.
 *
 * Compiled and linked against the MCUXpresso SDK 2.x (legacy-mcux-sdk) for the FRDM-K64F
 * (build.sh), not tested on hardware -- check against your SDK version.
 */

#include "pfodStream.h"
#include "pfodRingBuffer.h"
#include "fsl_uart.h"

class target_Stream : public pfodStream {
  public:
    /**
     * Constructor, only initialises member variables.
     * Does NOT access the hardware.
     * Call begin() from setup() to start the port.
     */
    target_Stream();

    /**
     * Start the UART: UART_Init() with the given baud rate (8N1, no flow control),
     * initialises the rx ring buffer, enables the UART's receive interrupts and its IRQ.
     * Call once from setup() before using it, e.g. before parser.connect().
     * The UART's pins must already be muxed (BOARD_InitPins()).
     * @param base the UART, e.g. UART0
     * @param irq its IRQ number, e.g. UART0_RX_TX_IRQn. The handler for it is
     *            PFOD_UART_IRQHandler in target_Stream.cpp.
     * @param srcClockHz the UART's clock, e.g. CLOCK_GetFreq(UART0_CLK_SRC)
     *            (UART0 and UART1 run from the core clock, the others from the bus clock)
     * @param baudRate the baud rate, e.g. 115200
     * @param rxBuf caller supplied storage for the rx ring buffer,
     *              must stay valid for the life of this stream
     * @param rxBufSize the size of rxBuf
     * If UART_Init() fails (e.g. the baud rate cannot be made from srcClockHz), the
     * stream stays not started, write() returns 0 and read() returns -1.
     * Only one target_Stream can be started, as there is one interrupt handler.
     */
    void begin(UART_Type *base, IRQn_Type irq, uint32_t srcClockHz, uint32_t baudRate,
               uint8_t *rxBuf, size_t rxBufSize);

    // pfodPrint methods
    // write(buf, size) is inherited from pfodPrint and calls write(uint8_t) for each byte.
    using pfodStream::write; // keep inherited write(buf, size) and write(const char*) visible

    /**
     * Write one byte to the UART with UART_WriteBlocking(), which busy waits
     * until the UART has room.
     * @param c the byte to send
     * @return 1 if the byte was sent, 0 if not (including if begin() has not been called / failed).
     */
    size_t write(uint8_t c) override;

    /**
     * Wait until all bytes previously written have been sent,
     * i.e. the UART's transmission complete flag is set.
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
     * Moves the received bytes from the UART into the rx ring buffer
     * (dropped if it is full). Not for direct use.
     */
    void onUartInterrupt();

  private:
    UART_Type *base; // set by begin() if UART_Init() succeeds, NULL until then, no i/o until then
    pfodRingBuffer rxRingBuffer;
};

#endif //target_Stream_h
