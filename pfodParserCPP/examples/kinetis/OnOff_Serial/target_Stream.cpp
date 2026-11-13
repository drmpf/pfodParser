/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
#include "target_Stream.h"

// The name of the UART's interrupt handler in the SDK's startup vector table,
// for the UART passed to begin(). Override for another UART.
#ifndef PFOD_UART_IRQHandler
#define PFOD_UART_IRQHandler UART0_RX_TX_IRQHandler
#endif

// The started target_Stream, set by begin(), for the interrupt handler.
static target_Stream* target_StreamInstance = NULL;

/**
 * The UART's receive / transmit interrupt handler, replaces the weak default in the
 * SDK's startup vector table. Passes the interrupt on to the started target_Stream.
 */
extern "C" void PFOD_UART_IRQHandler(void) {
  if (target_StreamInstance != NULL) {
    target_StreamInstance->onUartInterrupt();
  }
  SDK_ISR_EXIT_BARRIER; // the SDK's handlers end with this, see fsl_common_arm.h
}

/**
 * Constructor, only initialises member variables.
 * Does NOT access the hardware, call begin() from setup() for that.
 */
target_Stream::target_Stream() {
  base = NULL;
}

/**
 * Start the UART. See target_Stream.h for the parameters.
 * base is only stored (i.e. the stream started) if UART_Init() succeeds.
 */
void target_Stream::begin(UART_Type *_base, IRQn_Type irq, uint32_t srcClockHz, uint32_t baudRate,
                          uint8_t *rxBuf, size_t rxBufSize) {
  uart_config_t config;
  UART_GetDefaultConfig(&config); // 8N1, no flow control, tx / rx not yet enabled
  config.baudRate_Bps = baudRate;
  config.enableTx = true;
  config.enableRx = true;
  if (UART_Init(_base, &config, srcClockHz) != kStatus_Success) {
    return; // not started, write() returns 0, read() returns -1
  }
  rxRingBuffer.init(rxBuf, rxBufSize);
  base = _base;
  target_StreamInstance = this;
  UART_EnableInterrupts(base, kUART_RxDataRegFullInterruptEnable | kUART_RxOverrunInterruptEnable);
  EnableIRQ(irq);
}

/**
 * Write one byte with UART_WriteBlocking(), which busy waits until the UART has room.
 * @param c the byte to send
 * @return 1 if the byte was sent, 0 if not started or the driver reported an error.
 */
size_t target_Stream::write(uint8_t c) {
  if (!base) {
    return 0;
  }
  return (UART_WriteBlocking(base, &c, 1) == kStatus_Success) ? 1 : 0;
}

/**
 * Wait until all bytes previously written have been sent, i.e. the UART's
 * transmission complete flag is set.
 * Does NOT discard received data.
 */
void target_Stream::flush() {
  if (!base) {
    return;
  }
  while ((UART_GetStatusFlags(base) & kUART_TransmissionCompleteFlag) == 0U) {
    // wait for the last byte to be sent
  }
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
 * Called from the UART interrupt handler (interrupt context).
 * Moves the received bytes into the rx ring buffer (dropped if the ring buffer
 * is full). As in the SDK's uart_interrupt example, a byte is read whenever the
 * RX data register full or overrun flag is set; reading the status then the data
 * clears both flags (after an overrun the byte read is the last one received
 * before it, later ones were lost by the UART).
 */
void target_Stream::onUartInterrupt() {
  while ((UART_GetStatusFlags(base) & (kUART_RxDataRegFullFlag | kUART_RxOverrunFlag)) != 0U) {
    rxRingBuffer.write(UART_ReadByte(base));
  }
}
