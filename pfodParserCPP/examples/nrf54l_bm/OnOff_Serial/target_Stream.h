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
 * pfodStream implementation for Nordic nRF54L Series using the
 * nRF Connect SDK Bare Metal option (sdk-nrf-bm, no RTOS), NOT Arduino,
 * following the pattern of Arduino's HardwareSerial.
 * Wraps the nrfx UARTE driver (nrfx_uarte.h), as in the Bare Metal SDK's
 * samples/peripherals/uarte sample.
 *
 * Use it for pfodParser::connect(pfodStream*), and for debug output via
 * setDebugPtr(pfodStream*). Since a pfodStream is a pfodPrint, it can also be
 * used wherever a pfodPrint* is wanted.
 *
 * Uses the board's application UARTE from board-config.h:
 * BOARD_APP_UARTE_INST, BOARD_APP_UARTE_PIN_TX, BOARD_APP_UARTE_PIN_RX
 * (on the nRF54L15 DK, UARTE30 on P0.00 / P0.01, the DK's second virtual COM port;
 * the first one is the Bare Metal console / log UARTE).
 * Only one target_Stream instance can be used, as the UARTE instance and its
 * interrupt are fixed at compile time.
 *
 * Receive is interrupt driven, one byte at a time into two alternating 1 byte
 * DMA buffers (as in the Bare Metal UARTE sample), copied into a pfodRingBuffer,
 * so available(), read() and peek() just use the ring buffer.
 * write() uses nrfx_uarte_tx() in blocking mode, so flush() has nothing to wait for.
 *
 * Low power: the UARTE receiver is left enabled, which keeps the high frequency
 * clock running while waiting for input. main.cpp puts the CPU to sleep between
 * received bytes. (The Bare Metal SDK's LPUARTE driver is lower power but needs
 * extra REQ/RDY handshake lines and the same driver at the other end, so it
 * cannot be used with a PC serial port / pfodProxy.)
 *
 * Usage (in main.cpp):
 *   uint8_t rxBuf[128];
 *   target_Stream serialStream;     // global, the constructor does not touch the hardware
 *   ...
 *   // in setup()
 *   serialStream.begin(NRF_UARTE_BAUDRATE_115200, rxBuf, sizeof(rxBuf));
 *   parser.connect(&serialStream);
 *   setDebugPtr(&serialStream);     // optional, debug output on the same port
 *
 * Debug output can go out on the same port as the pfod messages. pfodWeb ignores
 * characters outside the {..} messages and logs them to the Raw Message Viewer.
 *
 * Compiled and linked against sdk-nrf-bm v2.0.1 (build.sh), not tested on hardware -- check against your sdk-nrf-bm version.
 */

#include "pfodStream.h"
#include "pfodRingBuffer.h"
#include <nrfx_uarte.h>

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
     * Initialise the board's application UARTE (8N1, no flow control),
     * connect its interrupt and start receiving.
     * Call once from setup() before using it, e.g. before parser.connect().
     * @param baudRate one of the nrf_uarte_baudrate_t values, e.g. NRF_UARTE_BAUDRATE_115200
     * @param rxBuf caller supplied storage for the rx ring buffer,
     *              must stay valid for the life of this stream
     * @param rxBufSize the size of rxBuf
     * If the UARTE driver fails to initialise or start receiving, the stream
     * stays not started, write() returns 0 and read() returns -1.
     */
    void begin(nrf_uarte_baudrate_t baudRate, uint8_t *rxBuf, size_t rxBufSize);

    // pfodPrint methods
    using pfodStream::write; // keep inherited write(const char*) visible

    /**
     * Write one byte, blocking until it has been transferred to the UARTE.
     * @param c the byte to send
     * @return 1 if the byte was sent, 0 if not (including if begin() has not been called / failed).
     */
    size_t write(uint8_t c) override;

    /**
     * Write a block of bytes with one blocking nrfx_uarte_tx() call, faster
     * than the inherited byte by byte version.
     * Data not in RAM (e.g. const strings in RRAM/flash) is copied through a
     * small RAM tx cache by the driver, as EasyDMA can only read RAM.
     * @param buf the bytes to send
     * @param size the number of bytes
     * @return size if they were sent, 0 if not (including if begin() has not been called / failed).
     */
    size_t write(const uint8_t *buf, size_t size) override;

    /**
     * Wait until all bytes previously written have been sent.
     * write() blocks until the transfer has finished, so there is nothing to wait for.
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
     * Called from the nrfx UARTE event handler (interrupt context) with the
     * received bytes. Stores them in the rx ring buffer (dropped if full).
     * Not for direct use.
     * @param data the received bytes
     * @param len the number of bytes
     */
    void onReceived(const uint8_t *data, size_t len);

  private:
    bool started;  // set true by begin() if the UARTE started, no i/o until then
    pfodRingBuffer rxRingBuffer;
};

#endif //target_Stream_h
