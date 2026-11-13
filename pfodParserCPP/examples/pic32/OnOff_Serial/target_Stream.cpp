/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
#include "target_Stream.h"

/**
 * Constructor, only initialises member variables.
 * Does NOT access the hardware, call begin() from setup() for that.
 */
target_Stream::target_Stream() {
  readFn = NULL;
  writeFn = NULL;
  readCountFn = NULL;
  writeCountFn = NULL;
  transmitCompleteFn = NULL;
  started = false;
  lookAhead = -1;
}

/**
 * Start using an MCC configured UART (Ring buffer mode).
 * See target_Stream.h for the parameters.
 */
void target_Stream::begin(ReadFn _readFn, WriteFn _writeFn, CountFn _readCountFn,
                          CountFn _writeCountFn, TransmitCompleteFn _transmitCompleteFn) {
  readFn = _readFn;
  writeFn = _writeFn;
  readCountFn = _readCountFn;
  writeCountFn = _writeCountFn;
  transmitCompleteFn = _transmitCompleteFn;
  lookAhead = -1;
  started = true;
}

/**
 * Write one byte into the PLIB's tx ring buffer.
 * UARTx_Write() returns the number of bytes queued, 0 when the tx ring buffer
 * is full; the UART interrupt is emptying it, so retry until there is space.
 * @param c the byte to send
 * @return 1 when the byte is queued, 0 if not started.
 */
size_t target_Stream::write(uint8_t c) {
  if (!started) {
    return 0;
  }
  while (writeFn(&c, 1) == 0) {
    // tx ring buffer full, wait for space
  }
  return 1;
}

/**
 * Wait until all bytes previously written have been sent:
 * first until the PLIB's tx ring buffer is empty, then until the UART has
 * finished sending the last byte.
 * Does NOT discard received data.
 */
void target_Stream::flush() {
  if (!started) {
    return;
  }
  while (writeCountFn() != 0) {
    // wait for the tx ring buffer to empty
  }
  while (!transmitCompleteFn()) {
    // wait for the last byte to be sent
  }
}

/**
 * Number of received bytes waiting to be read, 0 or 1.
 * If lookAhead is empty (-1), calls read() and stores the result in lookAhead.
 * @return 1 if a byte is available (held in lookAhead), 0 if none.
 */
int target_Stream::available() {
  if (lookAhead == -1) {
    lookAhead = read(); // lookAhead is empty so this reads from the PLIB
  }
  return (lookAhead == -1) ? 0 : 1;
}

/**
 * Read and remove the next received byte.
 * Returns the lookAhead byte first, if available()/peek() has stored one,
 * otherwise reads one byte from the PLIB's rx ring buffer, without blocking.
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
  if (readCountFn() == 0) {
    return -1; // nothing received
  }
  uint8_t b;
  return (readFn(&b, 1) == 1) ? (int)b : -1;
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
