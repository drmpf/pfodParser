/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
/*
 * target_pfodClock.cpp -- micros() (required) and yield() (optional)
 * for Zephyr RTOS
 *
 * pfodParserCPP does not implement micros(), it must be supplied by your program.
 * millis() (in pfodParserCPP/src/pfodClock.cpp) is derived from it.
 * See "Clock: micros(), millis() and yield()" in pfodParserCPP/BUILDING.md
 */

#include "pfodClock.h"
#include <zephyr/kernel.h>

/**
 * Microseconds since boot, from the 64 bit kernel uptime tick count.
 * The resolution is one kernel tick (CONFIG_SYS_CLOCK_TICKS_PER_SEC, often
 * 10 kHz, i.e. 100 uS, or 32768 Hz on nRF), which is plenty for pfodParser.
 * Casting the 64 bit uS count to unsigned long truncates it, so it counts
 * through the full unsigned long range and wraps from its maximum value back
 * to 0 (every ~71.6 minutes for 32 bit), as required.
 * @return the current microsecond count
 */
unsigned long micros() {
  return (unsigned long)k_ticks_to_us_floor64(k_uptime_ticks());
}

/*
 * yield()
 * Only called by pfodBufferedStream: by write() in blocking mode, while it
 * waits in 1 ms steps for transmit buffer space, and by flush(), while it
 * waits for the buffer to empty.
 * The library's default yield() (pfodClock.cpp) is weak and empty. This
 * yield() replaces it at link time, so other threads keep running while it waits.
 * yield() is NOT defined by Zephyr.
 */

/**
 * Let every other thread run while pfodBufferedStream waits.
 * k_msleep(1) sleeps this thread, so lower priority threads run too.
 * (k_yield() would only let threads of equal or higher priority run.)
 */
void yield() {
  k_msleep(1);
}
