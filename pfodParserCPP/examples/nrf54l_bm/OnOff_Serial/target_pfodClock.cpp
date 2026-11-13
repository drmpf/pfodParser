/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
/*
 * target_pfodClock.cpp -- micros() (required) and yield() (optional)
 * for nRF54L Series with the nRF Connect SDK Bare Metal option
 *
 * pfodParserCPP does not implement micros(), it must be supplied by your program.
 * millis() (in pfodParserCPP/src/pfodClock.cpp) is derived from it.
 * See "Clock: micros(), millis() and yield()" in pfodParserCPP/BUILDING.md
 *
 * The Bare Metal boards start the GRTC (Global Real Time Counter) SYSCOUNTER at
 * boot (CONFIG_NRF_GRTC_START_SYSCOUNTER=y in the board defconfig). It is a
 * 52 bit counter at 1 MHz, which keeps running in all power modes, and it is
 * Zephyr's 64 bit hardware cycle counter (k_cycle_get_64()) on these boards.
 */

#include "pfodClock.h"
#include <zephyr/kernel.h>

/**
 * Microseconds since boot, from the GRTC SYSCOUNTER via k_cycle_get_64(),
 * converted to uS with k_cyc_to_us_floor64() (the GRTC already counts in uS,
 * so this is exact and does not depend on the kernel tick rate).
 * Casting the 64 bit uS count to unsigned long truncates it, so it counts
 * through the full unsigned long range and wraps from its maximum value back
 * to 0 (every ~71.6 minutes), as required.
 * @return the current microsecond count
 */
unsigned long micros() {
  return (unsigned long)k_cyc_to_us_floor64(k_cycle_get_64());
}

/*
 * yield()
 * Only called by pfodBufferedStream: by write() in blocking mode, while it
 * waits in 1 ms steps for transmit buffer space, and by flush(), while it
 * waits for the buffer to empty.
 * The library's default yield() (pfodClock.cpp) is weak and empty, which is
 * fine here: there is no RTOS, and the SoftDevice runs the radio from its own
 * higher priority interrupts, so it keeps running while pfodBufferedStream waits.
 * Do NOT sleep the CPU in yield() (e.g. k_cpu_idle()): the kernel is tickless,
 * so there may be no interrupt to wake it up again.
 * If you enable the watchdog, feed it here, e.g. with nrfx_wdt:
 */
// #include <nrfx_wdt.h>
// extern nrfx_wdt_t wdt;             // your nrfx_wdt instance
// extern nrfx_wdt_channel_id wdtChannel; // from nrfx_wdt_channel_alloc()
// void yield() { nrfx_wdt_channel_feed(&wdt, wdtChannel); }
