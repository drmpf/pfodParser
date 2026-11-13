/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
#include "pfodRandom.h"
#include "pfodClock.h"
#include <stdlib.h>

static bool _pfodRandomInitialized = false; // global

// return random as a long
long getRandom() {
  if (!_pfodRandomInitialized) {
    _pfodRandomInitialized = true;
    unsigned long uS = micros();
    srand(uS);
  }
  return rand(); // return rand result
}

// return lower 8 bits of random
uint8_t pfodRandom8() {
  return (uint8_t)getRandom(); // 0 to 255
}

// return lower 16 bits of random
uint16_t pfodRandom16() {
  return (uint16_t)getRandom(); 
}
