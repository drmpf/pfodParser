/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
/*
 * target_pfodClock.cpp -- micros() (required) and yield() (optional)
 * for Nordic nRF52 (nRF5 SDK, bare metal)
 *
 * pfodParserCPP does not implement micros(), it must be supplied by your program.
 * millis() (in pfodParserCPP/src/pfodClock.cpp) is derived from it.
 * std::chrono::steady_clock does not work with arm-none-eabi / newlib (it fails
 * to link or always returns 0), so this uses a TIMER peripheral instead.
 * See "Clock: micros(), millis() and yield()" in pfodParserCPP/BUILDING.md
 *
 * This uses TIMER2 (PFOD_MICROS_TIMER), as a free running 32 bit counter at 1 MHz.
 * The SoftDevice reserves TIMER0, and some SDK libraries use TIMER1, so check
 * TIMER2 is not used elsewhere in your project, or change PFOD_MICROS_TIMER.
 * The TIMER runs from the 16 MHz HFCLK, which it requests automatically.
 */

#include "pfodClock.h"
#include "nrf.h"

#ifndef PFOD_MICROS_TIMER
#define PFOD_MICROS_TIMER NRF_TIMER2
#endif

/**
 * Microseconds since the first call.
 * On the first call, starts PFOD_MICROS_TIMER as a 32 bit timer with
 * prescaler 4, i.e. 16 MHz / 2^4 = 1 MHz.
 * Each call captures the running count into CC[0] and returns it.
 * The 32 bit count counts through the full unsigned long range and wraps from
 * its maximum value back to 0 (every ~71.6 minutes), as required.
 * Not safe to call from an interrupt handler (the capture uses CC[0], and the
 * first call starts the timer).
 * @return the current microsecond count
 */
unsigned long micros() {
  static bool started = false;
  if (!started) {
    PFOD_MICROS_TIMER->TASKS_STOP = 1;
    PFOD_MICROS_TIMER->MODE = TIMER_MODE_MODE_Timer;
    PFOD_MICROS_TIMER->BITMODE = TIMER_BITMODE_BITMODE_32Bit;
    PFOD_MICROS_TIMER->PRESCALER = 4; // 16 MHz / 2^4 = 1 MHz
    PFOD_MICROS_TIMER->TASKS_CLEAR = 1;
    PFOD_MICROS_TIMER->TASKS_START = 1;
    started = true;
  }
  PFOD_MICROS_TIMER->TASKS_CAPTURE[0] = 1; // copy the current count to CC[0]
  return (unsigned long)PFOD_MICROS_TIMER->CC[0];
}

/*
 * yield() -- optional.
 * Only called by pfodBufferedStream: by write() in blocking mode, while it
 * waits in 1 ms steps for transmit buffer space, and by flush(), while it
 * waits for the buffer to empty.
 * The library's default yield() (pfodClock.cpp) is weak and empty, which is
 * fine on bare metal, as nothing else needs to run.
 * With a SoftDevice, the radio keeps running while pfodBufferedStream waits,
 * see the nRF52 SoftDevice note in pfodParserCPP/BUILDING.md.
 * Do NOT call app_sched_execute() from yield().
 * If you have the watchdog enabled, feed it here, e.g.
 */
// #include "nrf_drv_wdt.h"
// extern nrf_drv_wdt_channel_id wdtChannel; // from your nrf_drv_wdt_channel_alloc()
// void yield() { nrf_drv_wdt_channel_feed(wdtChannel); }
