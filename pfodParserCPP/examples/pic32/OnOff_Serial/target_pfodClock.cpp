/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
/*
 * target_pfodClock.cpp -- micros() (required) and yield() (optional)
 * for PIC32 (MPLAB Harmony 3, xc32)
 *
 * pfodParserCPP does not implement micros(), it must be supplied by your program.
 * millis() (in pfodParserCPP/src/pfodClock.cpp) is derived from it.
 * std::chrono::steady_clock does not work with xc32 (it fails to link or
 * always returns 0), so this uses the MIPS core timer instead.
 * See "Clock: micros(), millis() and yield()" in pfodParserCPP/BUILDING.md
 *
 * The core timer (CP0 Count) is a free running 32 bit counter at half the CPU
 * clock. It must not be reset by other code (e.g. do not use the Harmony
 * CORETIMER PLIB in a mode that clears the count).
 * PFOD_CORE_TIMER_HZ defaults to CPU_CLOCK_FREQUENCY / 2, from the MCC generated
 * definitions.h. Check it for your device, and that it is a multiple of 1 MHz.
 */

#include "pfodClock.h"
#include <stdint.h>
#include <xc.h> // _CP0_GET_COUNT()
extern "C" {
#include "definitions.h" // CPU_CLOCK_FREQUENCY
}

#ifndef PFOD_CORE_TIMER_HZ
#define PFOD_CORE_TIMER_HZ (CPU_CLOCK_FREQUENCY / 2)
#endif

/**
 * Microseconds since the first call, from the core timer.
 * The core timer wraps (every ~43 seconds at a 200 MHz CPU clock) long before
 * a uS count would reach the maximum unsigned long, so its count cannot just be
 * divided down. Instead each call adds the core timer ticks elapsed since the
 * previous call to a 64 bit total, and returns total / ticks-per-uS cast to
 * unsigned long. That counts through the full unsigned long range and wraps from
 * its maximum value back to 0 (every ~71.6 minutes), as required.
 * It must be called at least once per core timer wrap period (~43 seconds at
 * 200 MHz), which pfodParser does every loop, via millis().
 * Not safe to call from an interrupt handler.
 * @return the current microsecond count
 */
unsigned long micros() {
  static bool started = false;
  static uint32_t lastCount = 0;
  static uint64_t totalTicks = 0;
  uint32_t count = _CP0_GET_COUNT();
  if (!started) {
    lastCount = count;
    started = true;
  }
  totalTicks += (uint32_t)(count - lastCount); // 32 bit unsigned subtraction handles the core timer wrap
  lastCount = count;
  return (unsigned long)(totalTicks / (PFOD_CORE_TIMER_HZ / 1000000UL));
}

/*
 * yield() -- optional.
 * Only called by pfodBufferedStream: by write() in blocking mode, while it
 * waits in 1 ms steps for transmit buffer space, and by flush(), while it
 * waits for the buffer to empty.
 * The library's default yield() (pfodClock.cpp) is weak and empty, which is
 * fine on bare metal, as nothing else needs to run.
 * If you have the watchdog enabled, clear it here, e.g.
 * with the WDT PLIB:
 * (Do not call SYS_Tasks() from yield(), it may call back into code that is
 * already waiting in pfodBufferedStream.)
 */
// void yield() { WDT_Clear(); }
