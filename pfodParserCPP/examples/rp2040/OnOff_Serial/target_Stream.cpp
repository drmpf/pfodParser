/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
#include "target_Stream.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

// The target_Stream using each UART (index 0 = uart0, 1 = uart1), set by begin(),
// for the interrupt handlers, which take no arguments.
static target_Stream* target_StreamInstances[2] = { NULL, NULL };

/**
 * uart0 interrupt handler, installed by begin() when it is given uart0.
 * Passes the interrupt on to the target_Stream using uart0.
 */
static void target_StreamUart0Isr() {
  target_StreamInstances[0]->onUartInterrupt();
}

/**
 * uart1 interrupt handler, installed by begin() when it is given uart1.
 * Passes the interrupt on to the target_Stream using uart1.
 */
static void target_StreamUart1Isr() {
  target_StreamInstances[1]->onUartInterrupt();
}

/**
 * Constructor, only initialises member variables.
 * Does NOT access the hardware, call begin() from setup() for that.
 */
target_Stream::target_Stream() {
  uart = NULL;
}

/**
 * Start the UART. See target_Stream.h for the parameters.
 * The pins are set to the UART function with UART_FUNCSEL_NUM(), which on the
 * RP2350 picks the UART_AUX function for the pins that need it.
 * uart_init() sets 8N1 and enables the FIFOs. The RX interrupt fires when the rx
 * FIFO is half full or, with the receive timeout, when bytes have been waiting
 * for 32 bit periods, so single bytes are not delayed.
 */
void target_Stream::begin(uart_inst_t *_uart, unsigned int txPin, unsigned int rxPin, unsigned int baudRate,
                          uint8_t *rxBuf, size_t rxBufSize) {
  rxRingBuffer.init(rxBuf, rxBufSize);
  uart = _uart;
  unsigned int idx = uart_get_index(uart);
  target_StreamInstances[idx] = this;

  uart_init(uart, baudRate);
  gpio_set_function(txPin, UART_FUNCSEL_NUM(uart, txPin));
  gpio_set_function(rxPin, UART_FUNCSEL_NUM(uart, rxPin));
  uart_set_hw_flow(uart, false, false);
  uart_set_format(uart, 8, 1, UART_PARITY_NONE);

  unsigned int irqNum = UART_IRQ_NUM(uart);
  irq_set_exclusive_handler(irqNum, (idx == 0) ? target_StreamUart0Isr : target_StreamUart1Isr);
  irq_set_enabled(irqNum, true);
  uart_set_irq_enables(uart, true, false); // RX (and RX timeout) interrupt on, TX interrupt off
}

/**
 * Write one byte with uart_putc_raw(), which busy waits until the tx FIFO has room.
 * @param c the byte to send
 * @return 1 if the byte was sent, 0 if not started.
 */
size_t target_Stream::write(uint8_t c) {
  if (!uart) {
    return 0;
  }
  uart_putc_raw(uart, (char)c);
  return 1;
}

/**
 * Wait until all bytes previously written have been sent, i.e. the tx FIFO
 * and shift register are empty.
 * Does NOT discard received data.
 */
void target_Stream::flush() {
  if (!uart) {
    return;
  }
  uart_tx_wait_blocking(uart);
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
 * Moves all the bytes in the UART rx FIFO into the rx ring buffer
 * (dropped if the ring buffer is full). Emptying the FIFO clears the
 * RX and RX timeout interrupts.
 */
void target_Stream::onUartInterrupt() {
  while (uart_is_readable(uart)) {
    rxRingBuffer.write((uint8_t)uart_getc(uart));
  }
}
