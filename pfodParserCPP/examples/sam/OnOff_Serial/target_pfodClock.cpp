/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
/*
 * target_pfodClock.cpp -- micros() (required) and yield() (optional)
 * for Microchip SAM (Cortex-M) with MPLAB Harmony 3
 *
 * pfodParserCPP does not implement micros(), it must be supplied by your program.
 * millis() (in pfodParserCPP/src/pfodClock.cpp) is derived from it.
 * std::chrono::steady_clock does not work with arm-none-eabi / newlib (it fails
 * to link or always returns 0), so this uses the Cortex-M SysTick timer, which every
 * SAM part has, so nothing needs to be configured in MCC for it.
 * SysTick is started, at a 1 ms interrupt rate from CPU_CLOCK_FREQUENCY (from the MCC
 * generated definitions.h), by the first micros() call.
 * !! This defines SysTick_Handler(). Do not also add MCC's SYSTICK PLIB (or an RTOS that
 * uses SysTick); if you need those, use their tick count here instead.
 * See "Clock: micros(), millis() and yield()" in pfodParserCPP/BUILDING.md
 */

#include "pfodClock.h"
extern "C" {
#include "definitions.h" // CPU_CLOCK_FREQUENCY, and via device.h the CMSIS SysTick definitions
}

// ms since SysTick was started, incremented by SysTick_Handler()
static volatile uint32_t sysTickMs = 0;
static bool sysTickStarted = false;

/**
 * SysTick interrupt handler, once per ms, counts the ms for micros().
 * Replaces the weak default handler in Harmony's interrupts.c vector table.
 */
extern "C" void SysTick_Handler(void) {
  sysTickMs++;
}

/**
 * Start SysTick at 1 ms, clocked from the CPU clock.
 * SysTick_Config() sets LOAD = CPU_CLOCK_FREQUENCY / 1000 - 1, enables the
 * counter and its interrupt, at the lowest priority.
 */
static void startSysTick() {
  SysTick_Config(CPU_CLOCK_FREQUENCY / 1000UL);
  sysTickStarted = true;
}

/**
 * Microseconds since the first micros() call, from the 1 ms SysTick count plus the
 * uS into the current SysTick period. SysTick counts down from LOAD to 0, once per ms,
 * so (LOAD - VAL) is the number of CPU clocks into the current ms; this scales that by
 * 1000 / (LOAD + 1), as CPU_CLOCK_FREQUENCY need not be a multiple of 1 MHz (e.g. the
 * SAM D21's 47.972352 MHz DFLL).
 * Re-reads if the ms count changed during the calculation, i.e. if the SysTick
 * interrupt happened in between.
 * Uses 32 bit unsigned arithmetic, so it counts through the full unsigned long
 * range and wraps from its maximum value back to 0 (every ~71.6 minutes), as required.
 * @return the current microsecond count
 */
unsigned long micros() {
  if (!sysTickStarted) {
    startSysTick();
  }
  uint32_t ms;
  uint32_t ticks;
  do {
    ms = sysTickMs;
    ticks = SysTick->LOAD - SysTick->VAL;
  } while (ms != sysTickMs);
  return (unsigned long)(ms * 1000UL + (ticks * 1000UL) / (SysTick->LOAD + 1UL));
}

/*
 * yield() -- optional.
 * Only called by pfodBufferedStream: by write() in blocking mode, while it
 * waits in 1 ms steps for transmit buffer space, and by flush(), while it
 * waits for the buffer to empty.
 * The library's default yield() (pfodClock.cpp) is weak and empty, which is
 * fine on bare metal, as nothing else needs to run.
 * If you have enabled the watchdog (WDT PLIB in MCC), feed it here, e.g.
 */
// void yield() { WDT_Clear(); }
