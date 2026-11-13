/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
#include "target_Stream.h"

// This class supports one UART. The single global HAL callback below
// (HAL_UART_RxCpltCallback) has no per-instance way to reach a C++ object,
// so begin() records the one active instance here for it to call back into.
static target_Stream* target_StreamInstance = NULL;

/**
 * Constructor, only initialises member variables.
 * Does NOT access the hardware, call begin() from setup() for that.
 */
target_Stream::target_Stream() {
  huart = NULL;
  rxByte = 0;
}

/**
 * Start using an already HAL_UART_Init()'d UART.
 * See target_Stream.h for the parameters.
 */
void target_Stream::begin(UART_HandleTypeDef *_huart, uint8_t *rxBuf, size_t rxBufSize) {
  rxRingBuffer.init(rxBuf, rxBufSize);
  target_StreamInstance = this;
  huart = _huart;
  HAL_UART_Receive_IT(huart, &rxByte, 1); // start the first interrupt driven receive
}

/**
 * Write one byte to the UART, blocking until it has been sent.
 * @param c the byte to send
 * @return 1 if the byte was sent, 0 if not (including if begin() has not been called).
 */
size_t target_Stream::write(uint8_t c) {
  if (!huart) {
    return 0;
  }
  return (HAL_UART_Transmit(huart, &c, 1, HAL_MAX_DELAY) == HAL_OK) ? 1 : 0;
}

/**
 * Write a block of bytes, blocking until they have been sent.
 * Sent in parts of at most 0xFFFF bytes, as HAL_UART_Transmit() takes a uint16_t size.
 * @param buf the bytes to send
 * @param size the number of bytes
 * @return the number of bytes sent, stopping at the first HAL error,
 *         0 if begin() has not been called
 */
size_t target_Stream::write(const uint8_t *buf, size_t size) {
  if (!huart) {
    return 0;
  }
  size_t sent = 0;
  while (sent < size) {
    size_t len = size - sent;
    if (len > 0xFFFF) {
      len = 0xFFFF;
    }
    if (HAL_UART_Transmit(huart, (uint8_t*)(buf + sent), (uint16_t)len, HAL_MAX_DELAY) != HAL_OK) {
      break;
    }
    sent += len;
  }
  return sent;
}

/**
 * Wait until all bytes previously written have been sent.
 * write() uses a blocking HAL_UART_Transmit(), which only returns after the
 * last byte has been transmitted, so there is nothing to wait for.
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
 * Called from HAL_UART_RxCpltCallback() (interrupt context) once the byte
 * requested by HAL_UART_Receive_IT() has arrived.
 * Stores it in the rx ring buffer (dropped if the buffer is full) and
 * requests the next byte.
 */
void target_Stream::onByteReceived() {
  rxRingBuffer.write(rxByte);
  HAL_UART_Receive_IT(huart, &rxByte, 1); // request the next byte
}

/**
 * HAL receive complete callback, overrides the HAL's weak default.
 * HAL_UART_IRQHandler() calls this once the byte requested by
 * HAL_UART_Receive_IT() has arrived. It is called for every UART, so check
 * the handle is the one this stream is using.
 * If the rest of your code also defines HAL_UART_RxCpltCallback(), merge
 * them into one.
 * @param huart the UART whose receive completed
 */
extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (target_StreamInstance && target_StreamInstance->getHandle() == huart) {
    target_StreamInstance->onByteReceived();
  }
}
