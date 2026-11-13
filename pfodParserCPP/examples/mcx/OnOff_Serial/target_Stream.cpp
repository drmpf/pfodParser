/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
#include "target_Stream.h"

// The name of the LPUART's interrupt handler in the SDK's startup vector table,
// for the LPUART passed to begin(). Override for another LPUART.
#ifndef PFOD_LPUART_IRQHandler
#define PFOD_LPUART_IRQHandler LP_FLEXCOMM4_IRQHandler
#endif

// The started target_Stream, set by begin(), for the interrupt handler.
static target_Stream* target_StreamInstance = NULL;

/**
 * The LPUART's interrupt handler, replaces the weak default in the SDK's startup
 * vector table (as the SDK's lpuart interrupt example does). Passes the interrupt on
 * to the started target_Stream.
 */
extern "C" void PFOD_LPUART_IRQHandler(void) {
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
 * Start the LPUART. See target_Stream.h for the parameters.
 * base is only stored (i.e. the stream started) if LPUART_Init() succeeds.
 */
void target_Stream::begin(LPUART_Type *_base, IRQn_Type irq, uint32_t srcClockHz, uint32_t baudRate,
                          uint8_t *rxBuf, size_t rxBufSize) {
  lpuart_config_t config;
  LPUART_GetDefaultConfig(&config); // 8N1, no flow control, tx / rx not yet enabled
  config.baudRate_Bps = baudRate;
  config.enableTx = true;
  config.enableRx = true;
  if (LPUART_Init(_base, &config, srcClockHz) != kStatus_Success) {
    return; // not started, write() returns 0, read() returns -1
  }
  rxRingBuffer.init(rxBuf, rxBufSize);
  base = _base;
  target_StreamInstance = this;
  LPUART_EnableInterrupts(base, kLPUART_RxDataRegFullInterruptEnable | kLPUART_RxOverrunInterruptEnable);
  EnableIRQ(irq);
}

/**
 * Write one byte with LPUART_WriteBlocking(), which busy waits until the LPUART has room.
 * @param c the byte to send
 * @return 1 if the byte was sent, 0 if not started or the driver reported an error.
 */
size_t target_Stream::write(uint8_t c) {
  if (!base) {
    return 0;
  }
  return (LPUART_WriteBlocking(base, &c, 1) == kStatus_Success) ? 1 : 0;
}

/**
 * Wait until all bytes previously written have been sent, i.e. the LPUART's
 * transmission complete flag is set.
 * Does NOT discard received data.
 */
void target_Stream::flush() {
  if (!base) {
    return;
  }
  while ((LPUART_GetStatusFlags(base) & kLPUART_TransmissionCompleteFlag) == 0U) {
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
 * Called from the LPUART interrupt handler (interrupt context).
 * Moves the received bytes into the rx ring buffer (dropped if the ring buffer
 * is full). On the LPUART an overrun flag is not cleared by reading the data,
 * and the receiver stores no more bytes until it is cleared, so it is cleared here
 * (the bytes that arrived during the overrun were lost by the LPUART).
 */
void target_Stream::onUartInterrupt() {
  while ((LPUART_GetStatusFlags(base) & kLPUART_RxDataRegFullFlag) != 0U) {
    rxRingBuffer.write(LPUART_ReadByte(base));
  }
  if ((LPUART_GetStatusFlags(base) & kLPUART_RxOverrunFlag) != 0U) {
    (void)LPUART_ClearStatusFlags(base, kLPUART_RxOverrunFlag); // only fails for flags that cannot be cleared
  }
}
