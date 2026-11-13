/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
#ifndef pfodRandom_h
#define pfodRandom_h
#include <stdint.h>

// return random as a long
long pfodRandom();

// return lower 8 bits of random
uint8_t pfodRandom8();

// return lower 16 bits of random
uint16_t pfodRandom16();

#endif // pfodRandom_h