/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
/*
 * target_pfodClock.cpp -- micros() (required) and yield() (optional)
 * for the Raspberry Pi RP2040 / RP2350 with the Pico SDK
 *
 * pfodParserCPP does not implement micros(), it must be supplied by your program.
 * millis() (in pfodParserCPP/src/pfodClock.cpp) is derived from it.
 * This uses the Pico SDK's time_us_32(), the low 32 bits of the chip's 1 MHz
 * 64 bit system timer, which the SDK's runtime starts before main().
 * See "Clock: micros(), millis() and yield()" in pfodParserCPP/BUILDING.md
 */

#include "pfodClock.h"
#include "hardware/timer.h"

/**
 * Microseconds since boot, from the 1 MHz system timer (time_us_32()).
 * 32 bit unsigned, so it counts through the full unsigned long range and wraps
 * from its maximum value back to 0 (every ~71.6 minutes), as required.
 * @return the current microsecond count
 */
unsigned long micros() {
  return (unsigned long)time_us_32();
}

/*
 * yield() -- optional.
 * Only called by pfodBufferedStream: by write() in blocking mode, while it
 * waits in 1 ms steps for transmit buffer space, and by flush(), while it
 * waits for the buffer to empty.
 * The library's default yield() (pfodClock.cpp) is weak and empty, which is
 * fine on bare metal, as nothing else needs to run.
 * If you have enabled the watchdog (watchdog_enable()), feed it here, e.g.
 */
// #include "hardware/watchdog.h"
// void yield() { watchdog_update(); }
