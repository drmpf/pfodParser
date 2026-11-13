/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
#include "target_Stream.h"
#include <zephyr/device.h>

/**
 * Zephyr interrupt driven UART callback, installed by begin().
 * Passes received data on to the target_Stream instance given as userData.
 * @param dev the UART device that interrupted
 * @param userData the target_Stream instance passed to uart_irq_callback_user_data_set()
 */
static void target_StreamUartIsr(const struct device *dev, void *userData) {
  target_Stream *self = (target_Stream*)userData;
  // must be called first in the ISR; returns void from Zephyr 4.5 (int before), so the
  // result is not tested, as in Zephyr's echo_bot sample
  uart_irq_update(dev);
  if (uart_irq_rx_ready(dev)) {
    self->onUartInterrupt();
  }
}

/**
 * Constructor, only initialises member variables.
 * Does NOT access the hardware, call begin() from setup() for that.
 */
target_Stream::target_Stream() {
  uartDev = NULL;
}

/**
 * Start using a Zephyr UART device. See target_Stream.h for the parameters.
 * uartDev is only stored (i.e. the stream started) if the device is ready.
 */
void target_Stream::begin(const struct device *_uartDev, uint8_t *rxBuf, size_t rxBufSize) {
  if (!device_is_ready(_uartDev)) {
    return; // not started, write() returns 0, read() returns -1
  }
  rxRingBuffer.init(rxBuf, rxBufSize);
  uartDev = _uartDev;
  uart_irq_callback_user_data_set(uartDev, target_StreamUartIsr, this);
  uart_irq_rx_enable(uartDev);
}

/**
 * Write one byte with uart_poll_out(), which busy waits until the UART has room.
 * @param c the byte to send
 * @return 1 if the byte was sent, 0 if not started.
 */
size_t target_Stream::write(uint8_t c) {
  if (!uartDev) {
    return 0;
  }
  uart_poll_out(uartDev, c);
  return 1;
}

/**
 * Wait until all bytes previously written have been sent.
 * write() uses uart_poll_out(), which only returns once the byte is in the
 * UART hardware, so there is nothing to wait for.
 * Does NOT discard received data.
 */
void target_Stream::flush() {
}

/**
 * @return the number of received bytes waiting in the rx ring buffer.
 */
int target_Stream::available() {
  return rxRingBuffer.available();
}

/**
 * Read and remove the next received byte from the rx ring buffer.
 * @return the next byte (0..255) as an int, or -1 if no byte is available.
 */
int target_Stream::read() {
  return rxRingBuffer.read();
}

/**
 * Return the next received byte WITHOUT removing it.
 * @return the next byte (0..255) as an int, or -1 if no byte is available.
 */
int target_Stream::peek() {
  return rxRingBuffer.peek();
}

/**
 * Called from the UART interrupt callback (interrupt context).
 * Moves all the bytes in the UART rx fifo into the rx ring buffer
 * (dropped if the ring buffer is full).
 */
void target_Stream::onUartInterrupt() {
  uint8_t c;
  while (uart_fifo_read(uartDev, &c, 1) == 1) {
    rxRingBuffer.write(c);
  }
}
