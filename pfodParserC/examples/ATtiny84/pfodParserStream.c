/*
 * File:   pfodParserStream_ATtiny84.c
 *
 * Interrupt driven software serial implementation of the pfodParserStream
 * interface for ATtiny84, which has no hardware UART (only USI).
 * Also provides micros(), see pfodParserStream_ATtiny84.h
 *
 * Timer1 free runs at the CPU clock and times every serial bit, so this code
 * uses Timer1 and PCINT0. Timer0 is left free. Do not change the Timer1 settings.
 * Each compare value is the previous one plus one bit time, so a late interrupt
 * only delays that one bit and the timing error does not build up.
 * Sending and receiving can happen at the same time.
 *
 * Defaults below -- change to suit your wiring:
 *   F_CPU  8MHz internal RC, no prescaling
 *   BAUD   9600
 *   TX     PA1
 *   RX     PA2 (pin change interrupt PCINT2)
 *
 * pfodParserStream_init() is called from pfodParser_pfodParser()
 */
#ifndef F_CPU
#define F_CPU 8000000UL
#endif
#include "pfodParserStream.h"
#include "pfodParserStream_ATtiny84.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>

#if F_CPU != 8000000UL
#error "This code needs F_CPU 8MHz, as pfodParserStream_init() sets the clock to 8MHz and micros() assumes 8 timer counts per uS"
#endif
#define TIMER1_COUNTS_PER_US_SHIFT 3 // 8 counts per uS
#define OVERFLOW_US_SHIFT (16 - TIMER1_COUNTS_PER_US_SHIFT) // uS per Timer1 overflow, as a shift

#define SOFT_SERIAL_BAUD 9600UL

#define TX_DDR    DDRA
#define TX_PORT   PORTA
#define TX_PIN    PA1

#define RX_DDR    DDRA
#define RX_PORT   PORTA
#define RX_PIN_IN PINA
#define RX_PIN    PA2

#define BIT_TICKS ((uint16_t) ((F_CPU + SOFT_SERIAL_BAUD / 2) / SOFT_SERIAL_BAUD)) // Timer1 counts per bit
#define TX_START_TICKS 100 // Timer1 counts until an idle transmitter starts, long enough to set the compare value safely
// Cycles from the start edge to reading TCNT1 in PCINT0, plus from each compare match to reading RX in
// TIM1_COMPA, counted from the avr-gcc -Os output. Removed so RX is sampled at the middle of each bit.
#define RX_LATENCY_TICKS 48
#define RX_DATA_BITS 8
#define TX_FRAME_BITS 10 // start bit + 8 data bits + stop bit

#define RX_BUF_SIZE 16
static volatile uint8_t rxBuf[RX_BUF_SIZE];
static volatile uint8_t rxHead;
static volatile uint8_t rxTail;
static uint8_t rxData; // byte being received, only used in the receive interrupts
static uint8_t rxBitCount; // data bits received so far, only used in the receive interrupts

#define TX_BUF_SIZE 16
static volatile uint8_t txBuf[TX_BUF_SIZE];
static volatile uint8_t txHead;
static volatile uint8_t txTail;
static volatile uint8_t txActive; // 1 while the transmit interrupt is running
static uint16_t txFrame; // bits still to send, lowest first, only used in the transmit interrupt
static uint8_t txBitsLeft; // only used in the transmit interrupt

static volatile unsigned long timer1Overflows; // used by micros()

/**
 * Do any Stream/UART setup required, called from pfodParser_pfodParser()
 * Sets the clock to 8MHz, whether or not the factory CKDIV8 fuse is still set,
 * starts Timer1 and enables global interrupts.
 */
void pfodParserStream_init(void) {
    clock_prescale_set(clock_div_1);
    TX_DDR |= (1 << TX_PIN);
    TX_PORT |= (1 << TX_PIN); // idle high
    RX_DDR &= (uint8_t) ~(1 << RX_PIN); // input
    RX_PORT |= (1 << RX_PIN); // pull-up, idle high
    TCCR1A = 0; // normal mode, OC1A and OC1B pins not used
    TCCR1B = (1 << CS10); // free run at the CPU clock
    TIMSK1 = (1 << TOIE1); // count overflows for micros()
    PCMSK0 |= (1 << RX_PIN); // enable pin change interrupt on RX pin
    GIMSK |= (1 << PCIE0);
    sei();
}

/**
 * Fires on the falling edge start bit. Stops looking for edges while this byte
 * is received and sets Timer1 compare A to the middle of the first data bit.
 * Only the RX pin is enabled for this interrupt.
 */
ISR(PCINT0_vect) {
    if (RX_PIN_IN & (1 << RX_PIN)) {
        return; // rising edge, not a start bit
    }
    PCMSK0 &= (uint8_t) ~(1 << RX_PIN); // stop re-triggering while we sample this byte
    OCR1A = TCNT1 + BIT_TICKS + BIT_TICKS / 2 - RX_LATENCY_TICKS;
    rxData = 0;
    rxBitCount = 0;
    TIFR1 = (1 << OCF1A); // clear any old match, writing 1 clears the flag
    TIMSK1 |= (1 << OCIE1A);
}

/**
 * Fires in the middle of each received bit, data bits first (lowest first), then the
 * stop bit. A byte with a low stop bit is bad data, as is one that does not fit in
 * the buffer, and is dropped. Starts looking for the next start bit from the middle
 * of the stop bit, so that it is not missed if the sender is running a little fast.
 */
ISR(TIM1_COMPA_vect) {
    uint8_t rxHigh = RX_PIN_IN & (1 << RX_PIN);
    OCR1A += BIT_TICKS;
    if (rxBitCount < RX_DATA_BITS) {
        rxData >>= 1;
        if (rxHigh) {
            rxData |= 0x80;
        }
        rxBitCount++;
        return;
    }
    if (rxHigh) {
        uint8_t next = (uint8_t) ((rxHead + 1) % RX_BUF_SIZE);
        if (next != rxTail) {
            rxBuf[rxHead] = rxData;
            rxHead = next;
        }
    }
    TIMSK1 &= (uint8_t) ~(1 << OCIE1A);
    PCMSK0 |= (1 << RX_PIN); // re-arm for the next start bit
}

/**
 * Fires once per transmitted bit. Outputs the next bit of the current frame.
 * When a frame is finished it starts the next byte in the buffer, or stops if there are none.
 */
ISR(TIM1_COMPB_vect) {
    OCR1B += BIT_TICKS;
    if (txBitsLeft == 0) { // the stop bit has now been sent for its full time
        if (txHead == txTail) {
            TIMSK1 &= (uint8_t) ~(1 << OCIE1B);
            txActive = 0;
            return;
        }
        txFrame = (uint16_t) (((uint16_t) txBuf[txTail] << 1) | (1U << (TX_FRAME_BITS - 1))); // low start bit, high stop bit
        txTail = (uint8_t) ((txTail + 1) % TX_BUF_SIZE);
        txBitsLeft = TX_FRAME_BITS;
    }
    if (txFrame & 1) {
        TX_PORT |= (1 << TX_PIN);
    } else {
        TX_PORT &= (uint8_t) ~(1 << TX_PIN);
    }
    txFrame >>= 1;
    txBitsLeft--;
}

/**
 * Counts Timer1 overflows, every 8.192mS, for micros()
 */
ISR(TIM1_OVF_vect) {
    timer1Overflows++;
}

/**
 * Microseconds since Timer1 started, see pfodParserStream_ATtiny84.h
 * Interrupts are blocked while reading, as the overflow count and TCNT1 must match,
 * and a read of TCNT1 shares a temporary register with the 16 bit writes in the interrupts.
 * If an overflow is pending and not yet counted, TCNT1 has only just rolled over
 * so it is small, which separates it from an overflow that happened just after reading TCNT1.
 */
unsigned long micros(void) {
    uint8_t sreg = SREG;
    cli();
    unsigned long overflows = timer1Overflows;
    uint16_t count = TCNT1;
    if ((TIFR1 & (1 << TOV1)) && (count < 0x8000U)) {
        overflows++;
    }
    SREG = sreg;
    return (overflows << OVERFLOW_US_SHIFT) + (count >> TIMER1_COUNTS_PER_US_SHIFT);
}

/**
 * @return bytes available for pfodParser_read()
 */
size_t pfodParser_RXavailable(void) {
    return (size_t) ((rxHead - rxTail + RX_BUF_SIZE) % RX_BUF_SIZE);
}

/**
 * Read an input byte from the receive buffer. Note: spins if none available.
 * @return the byte read
 */
int pfodParser_read(void) {
    while (pfodParser_RXavailable() == 0) {
        // spin
    }
    uint8_t c = rxBuf[rxTail];
    rxTail = (uint8_t) ((rxTail + 1) % RX_BUF_SIZE);
    return (int) c;
}

/**
 * @return space available in the transmit buffer for pfodParser_write()
 */
size_t pfodParser_TXfree(void) {
    return (size_t) (TX_BUF_SIZE - 1 - ((txHead - txTail + TX_BUF_SIZE) % TX_BUF_SIZE));
}

/**
 * Add a byte to the transmit buffer and start the transmit interrupt if it is idle.
 * Note: spins if the buffer is full, so do not call this from an interrupt.
 * @param c the byte to send
 * @return 1, the number of bytes written
 */
int pfodParser_write(uint8_t c) {
    while (pfodParser_TXfree() == 0) {
        // spin
    }
    uint8_t sreg = SREG;
    cli();
    txBuf[txHead] = c;
    txHead = (uint8_t) ((txHead + 1) % TX_BUF_SIZE);
    if (!txActive) {
        txActive = 1;
        txBitsLeft = 0; // the first interrupt loads this byte
        OCR1B = TCNT1 + TX_START_TICKS;
        TIFR1 = (1 << OCF1B); // clear any old match, writing 1 clears the flag
        TIMSK1 |= (1 << OCIE1B);
    }
    SREG = sreg;
    return 1;
}
