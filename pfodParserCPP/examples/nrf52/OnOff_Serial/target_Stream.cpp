/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
#include "target_Stream.h"

/**
 * app_uart event handler, required by app_uart_init().
 * app_uart uses it to report data ready, fifo errors and comms errors.
 * This stream polls with app_uart_get() / app_uart_put() instead, so it does nothing.
 * @param p_event the app_uart event, unused
 */
static void target_StreamUartEventHandler(app_uart_evt_t *p_event) {
  (void)p_event;
}

/**
 * Constructor, only initialises member variables.
 * Does NOT access the hardware, call begin() from setup() for that.
 */
target_Stream::target_Stream() {
  started = false;
  lookAhead = -1;
}

/**
 * Initialise the UART with app_uart_init().
 * See target_Stream.h for the parameters.
 * started is only set true if app_uart_init() succeeds.
 */
void target_Stream::begin(uint32_t rxPin, uint32_t txPin,
                          uint8_t *rxBuf, uint16_t rxBufSize,
                          uint8_t *txBuf, uint16_t txBufSize,
                          uint32_t baudRate,
                          uint32_t rtsPin, uint32_t ctsPin) {
  app_uart_comm_params_t commParams;
  commParams.rx_pin_no = rxPin;
  commParams.tx_pin_no = txPin;
  commParams.rts_pin_no = rtsPin;
  commParams.cts_pin_no = ctsPin;
  commParams.flow_control = (rtsPin != UART_PIN_DISCONNECTED && ctsPin != UART_PIN_DISCONNECTED)
                               ? APP_UART_FLOW_CONTROL_ENABLED : APP_UART_FLOW_CONTROL_DISABLED;
  commParams.use_parity = false;
  commParams.baud_rate = baudRate; // uint32_t in nRF5 SDK 17.1.0 app_uart.h

  app_uart_buffers_t buffers;
  buffers.rx_buf = rxBuf;
  buffers.rx_buf_size = rxBufSize;
  buffers.tx_buf = txBuf;
  buffers.tx_buf_size = txBufSize;

  uint32_t errCode = app_uart_init(&commParams, &buffers, target_StreamUartEventHandler, APP_IRQ_PRIORITY_LOWEST);
  started = (errCode == NRF_SUCCESS);
  lookAhead = -1;
}

/**
 * Write one byte into app_uart's tx fifo.
 * app_uart_put() returns NRF_ERROR_NO_MEM when the tx fifo is full; the UART
 * interrupt is emptying it, so retry until there is space.
 * @param c the byte to send
 * @return 1 if the byte was queued, 0 on any other app_uart error
 *         (including if not started).
 */
size_t target_Stream::write(uint8_t c) {
  if (!started) {
    return 0;
  }
  uint32_t errCode;
  do {
    errCode = app_uart_put(c);
  } while (errCode == NRF_ERROR_NO_MEM); // tx fifo full, wait for space
  return (errCode == NRF_SUCCESS) ? 1 : 0;
}

/**
 * Wait until all bytes previously written have been sent.
 * app_uart has no call to wait for its tx fifo to empty (app_uart_flush()
 * DISCARDS both fifos, so it must not be used here), so this does nothing.
 * The queued bytes are still sent by the UART interrupt.
 * Does NOT discard received data.
 */
void target_Stream::flush() {
}

/**
 * Number of received bytes waiting to be read, 0 or 1.
 * If lookAhead is empty (-1), calls read() and stores the result in lookAhead.
 * @return 1 if a byte is available (held in lookAhead), 0 if none.
 */
int target_Stream::available() {
  if (lookAhead == -1) {
    lookAhead = read(); // lookAhead is empty so this reads from app_uart
  }
  return (lookAhead == -1) ? 0 : 1;
}

/**
 * Read and remove the next received byte.
 * Returns the lookAhead byte first, if available()/peek() has stored one,
 * otherwise gets one byte from app_uart's rx fifo, without blocking.
 * @return the next byte (0..255) as an int, or -1 if no byte is available
 *         (including if not started).
 */
int target_Stream::read() {
  if (lookAhead != -1) {
    int c = lookAhead;
    lookAhead = -1; // empty the lookAhead
    return c;
  }
  if (!started) {
    return -1;
  }
  uint8_t b;
  return (app_uart_get(&b) == NRF_SUCCESS) ? (int)b : -1;
}

/**
 * Return the next received byte WITHOUT removing it.
 * Implemented using available() to fill the one byte lookAhead.
 * @return the next byte (0..255) as an int, or -1 if no byte is available.
 */
int target_Stream::peek() {
  available(); // fills lookAhead if a byte can be read
  return lookAhead;
}
