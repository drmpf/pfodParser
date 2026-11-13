/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
#include <string.h>
#include "target_Stream.h"

/**
 * Constructor, only initialises member variables.
 * Does NOT access the hardware, call begin() from setup() for that.
 */
target_Stream::target_Stream() {
  uartPort = UART_NUM_0;
  started = false;
  lookAhead = -1;
}

/**
 * Initialise the UART (8N1, no flow control) and install the SDK's UART driver.
 * See target_Stream.h for the parameters.
 * started is only set true if uart_driver_install() succeeds.
 */
void target_Stream::begin(uart_port_t port, uint32_t baudRate,
                          int txPin, int rxPin,
                          int rxBufSize, int txBufSize) {
  uartPort = port;

  uart_config_t config;
  memset(&config, 0, sizeof(config));
  config.baud_rate = baudRate;
  config.data_bits = UART_DATA_8_BITS;
  config.parity = UART_PARITY_DISABLE;
  config.stop_bits = UART_STOP_BITS_1;
  config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;

  uart_param_config(uartPort, &config);
#if defined(CONFIG_IDF_TARGET_ESP8266)
  // ESP8266_RTOS_SDK has no uart_set_pin(), the UART pins are fixed
  (void)(txPin);
  (void)(rxPin);
#else
  uart_set_pin(uartPort, txPin, rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
#endif
  started = (uart_driver_install(uartPort, rxBufSize, txBufSize, 0, NULL, 0) == ESP_OK);
  lookAhead = -1;
}

/**
 * Write one byte to the UART, blocking until there is space in the driver's
 * tx buffer.
 * @param c the byte to send
 * @return 1 if the byte was queued, 0 if not (including if not started).
 */
size_t target_Stream::write(uint8_t c) {
  if (!started) {
    return 0;
  }
  return (uart_write_bytes(uartPort, (const char*)&c, 1) == 1) ? 1 : 0;
}

/**
 * Write a block of bytes to the UART with one driver call.
 * @param buf the bytes to send
 * @param size the number of bytes
 * @return the number of bytes queued, 0 if not started or on a driver error (-1)
 */
size_t target_Stream::write(const uint8_t *buf, size_t size) {
  if (!started) {
    return 0;
  }
  int n = uart_write_bytes(uartPort, (const char*)buf, size);
  return (n < 0) ? 0 : (size_t)n;
}

/**
 * Wait until all bytes previously written have been transmitted.
 * uart_wait_tx_done() blocks this task (not a busy wait) until the driver's
 * tx buffer and the hardware fifo are empty.
 * Does NOT discard received data.
 */
void target_Stream::flush() {
  if (!started) {
    return;
  }
  uart_wait_tx_done(uartPort, portMAX_DELAY);
}

/**
 * Number of received bytes waiting to be read, 0 or 1.
 * If lookAhead is empty (-1), calls read() and stores the result in lookAhead.
 * @return 1 if a byte is available (held in lookAhead), 0 if none.
 */
int target_Stream::available() {
  if (lookAhead == -1) {
    lookAhead = read(); // lookAhead is empty so this reads from the UART driver
  }
  return (lookAhead == -1) ? 0 : 1;
}

/**
 * Read and remove the next received byte.
 * Returns the lookAhead byte first, if available()/peek() has stored one,
 * otherwise reads one byte from the driver's rx buffer with a 0 tick timeout,
 * i.e. without blocking.
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
  int n = uart_read_bytes(uartPort, &b, 1, 0); // 0 ticks == non-blocking
  return (n == 1) ? (int)b : -1;
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
