/*
 * File:   pfodParserStream_ATtiny84.h
 *
 * ATtiny84 specific additions to the pfodParserStream interface.
 */
#ifndef PFODPARSERSTREAM_ATTINY84_H
#define PFODPARSERSTREAM_ATTINY84_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Microseconds since pfodParserStream_init() started Timer1.
 * Returns 0 before that, as Timer1 is not running.
 * Safe to call from main code or from an interrupt.
 *
 * @return count in 1uS steps, with 1/8uS timer resolution truncated.
 *   Rolls over about every 71.6 minutes. Use (micros() - start) to measure
 *   an interval, which stays correct across the rollover.
 *   The accuracy is that of the 8MHz internal RC oscillator.
 */
unsigned long micros(void);

#ifdef __cplusplus
}
#endif

#endif /* PFODPARSERSTREAM_ATTINY84_H */
