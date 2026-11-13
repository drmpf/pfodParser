/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
#include "target_Stream.h"
#include <zephyr/kernel.h>
#include <bm/bm_irq.h>
#include <board-config.h>

// UARTE interrupt priority, 2 (highest) to 7 (lowest).
// Must be above 4 (the SoftDevice low priority) when the SoftDevice is enabled,
// as in the Bare Metal UARTE sample.
#ifndef PFOD_UARTE_IRQ_PRIO
#define PFOD_UARTE_IRQ_PRIO 5
#endif

// The board's application UARTE, from board-config.h
static nrfx_uarte_t uarteInst = NRFX_UARTE_INSTANCE(BOARD_APP_UARTE_INST);

// two alternating 1 byte receive DMA buffers, as in the Bare Metal UARTE sample
static uint8_t uarteRxDmaBuf[2][1];
static int uarteRxDmaBufIdx = 0;

// RAM copy buffer used by the driver for tx data that is not in RAM
static uint8_t uarteTxCache[32];

// The one target_Stream using uarteInst, set by begin(), for the event handler.
static target_Stream* target_StreamInstance = NULL;

/**
 * nrfx UARTE event handler, called from the UARTE interrupt.
 * RX_BUF_REQUEST: supplies the next 1 byte receive buffer.
 * RX_DONE: passes the received byte(s) to the target_Stream and re-enables
 * receive, as in the Bare Metal UARTE sample.
 * ERROR: (framing / overrun etc) ignored, the byte is lost, receive continues.
 * @param event the UARTE event
 * @param ctx the context from the config, unused
 */
static void target_StreamUarteEventHandler(const nrfx_uarte_event_t *event, void *ctx) {
  (void)ctx;
  switch (event->type) {
    case NRFX_UARTE_EVT_RX_DONE:
      if ((event->data.rx.length > 0) && (target_StreamInstance != NULL)) {
        target_StreamInstance->onReceived(event->data.rx.p_buffer, event->data.rx.length);
      }
      (void)nrfx_uarte_rx_enable(&uarteInst, 0); // keep receiving
      break;
    case NRFX_UARTE_EVT_RX_BUF_REQUEST:
      (void)nrfx_uarte_rx_buffer_set(&uarteInst, uarteRxDmaBuf[uarteRxDmaBufIdx], sizeof(uarteRxDmaBuf[0]));
      uarteRxDmaBufIdx = uarteRxDmaBufIdx ? 0 : 1;
      break;
    default:
      break;
  }
}

/**
 * UARTE direct interrupt handler, connected by begin().
 * Passes the interrupt to the nrfx UARTE driver, which calls the event handler.
 * @return 0, no rescheduling (no RTOS)
 */
ISR_DIRECT_DECLARE(target_StreamUarteIsr) {
  nrfx_uarte_irq_handler(&uarteInst);
  return 0;
}

/**
 * Constructor, only initialises member variables.
 * Does NOT access the hardware, call begin() from setup() for that.
 */
target_Stream::target_Stream() {
  started = false;
}

/**
 * Initialise the application UARTE and start receiving.
 * See target_Stream.h for the parameters.
 * started is only set true if the driver initialised and receive started.
 */
void target_Stream::begin(nrf_uarte_baudrate_t baudRate, uint8_t *rxBuf, size_t rxBufSize) {
  rxRingBuffer.init(rxBuf, rxBufSize);
  target_StreamInstance = this;

  nrfx_uarte_config_t uarteConfig = NRFX_UARTE_DEFAULT_CONFIG(BOARD_APP_UARTE_PIN_TX,
                                                              BOARD_APP_UARTE_PIN_RX);
  uarteConfig.baudrate = baudRate;
  uarteConfig.interrupt_priority = PFOD_UARTE_IRQ_PRIO;
  uarteConfig.tx_cache.p_buffer = uarteTxCache;
  uarteConfig.tx_cache.length = sizeof(uarteTxCache);

  // connect the IRQ ourselves, as in the Bare Metal UARTE sample
  BM_IRQ_DIRECT_CONNECT(NRFX_IRQ_NUMBER_GET(BOARD_APP_UARTE_INST), PFOD_UARTE_IRQ_PRIO,
                        target_StreamUarteIsr, 0);
  irq_enable(NRFX_IRQ_NUMBER_GET(BOARD_APP_UARTE_INST));

  if (nrfx_uarte_init(&uarteInst, &uarteConfig, target_StreamUarteEventHandler) != 0) {
    return; // not started
  }
  if (nrfx_uarte_rx_enable(&uarteInst, 0) != 0) {
    return; // not started
  }
  started = true;
}

/**
 * Write one byte, blocking until it has been transferred.
 * c is on the stack, i.e. in RAM, so EasyDMA can read it directly.
 * @param c the byte to send
 * @return 1 if sent, 0 if not (including if not started).
 */
size_t target_Stream::write(uint8_t c) {
  if (!started) {
    return 0;
  }
  return (nrfx_uarte_tx(&uarteInst, &c, 1, NRFX_UARTE_TX_BLOCKING) == 0) ? 1 : 0;
}

/**
 * Write a block of bytes, blocking until they have been transferred.
 * @param buf the bytes to send, need not be in RAM (the tx cache is used)
 * @param size the number of bytes
 * @return size if sent, 0 if not (including if not started).
 */
size_t target_Stream::write(const uint8_t *buf, size_t size) {
  if (!started) {
    return 0;
  }
  return (nrfx_uarte_tx(&uarteInst, buf, size, NRFX_UARTE_TX_BLOCKING) == 0) ? size : 0;
}

/**
 * Wait until all bytes previously written have been sent.
 * write() uses blocking nrfx_uarte_tx(), which only returns when the transfer
 * has finished, so there is nothing to wait for.
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
 * Called from the UARTE event handler (interrupt context) with received bytes.
 * Stores them in the rx ring buffer (dropped if the ring buffer is full).
 * @param data the received bytes
 * @param len the number of bytes
 */
void target_Stream::onReceived(const uint8_t *data, size_t len) {
  rxRingBuffer.write(data, len);
}
