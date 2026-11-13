/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
/*
 * target_pfodClock.cpp -- micros() (required) and yield() (optional)
 * for NXP Kinetis (Cortex-M) with the MCUXpresso SDK
 *
 * pfodParserCPP does not implement micros(), it must be supplied by your program.
 * millis() (in pfodParserCPP/src/pfodClock.cpp) is derived from it.
 * std::chrono::steady_clock does not work with arm-none-eabi / newlib (it fails
 * to link or always returns 0), so this uses the Cortex-M SysTick timer.
 * SysTick is started, at a 1 ms interrupt rate from SystemCoreClock (CMSIS, set by
 * BOARD_BootClockRUN()), by the first micros() call, so call BOARD_BootClockRUN()
 * before anything that uses micros() / millis().
 * !! This defines SysTick_Handler(). Do not also use an RTOS (e.g. the SDK's FreeRTOS)
 * that uses SysTick; if you do, use its tick count here instead.
 * See "Clock: micros(), millis() and yield()" in pfodParserCPP/BUILDING.md
 */

#include "pfodClock.h"
#include "fsl_device_registers.h" // CMSIS: SysTick, SysTick_Config(), SystemCoreClock

// ms since SysTick was started, incremented by SysTick_Handler()
static volatile uint32_t sysTickMs = 0;
static bool sysTickStarted = false;

/**
 * SysTick interrupt handler, once per ms, counts the ms for micros().
 * Replaces the weak default handler in the SDK's startup vector table.
 */
extern "C" void SysTick_Handler(void) {
  sysTickMs++;
}

/**
 * Start SysTick at 1 ms, clocked from the core clock.
 * SysTick_Config() sets LOAD = SystemCoreClock / 1000 - 1, enables the
 * counter and its interrupt, at the lowest priority.
 */
static void startSysTick() {
  SysTick_Config(SystemCoreClock / 1000UL);
  sysTickStarted = true;
}

/**
 * Microseconds since the first micros() call, from the 1 ms SysTick count plus the
 * uS into the current SysTick period. SysTick counts down from LOAD to 0, once per ms,
 * so (LOAD - VAL) is the number of core clocks into the current ms; this scales that by
 * 1000 / (LOAD + 1), so the core clock need not be a multiple of 1 MHz.
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
 * If you have enabled the watchdog (fsl_wdog.h), feed it here, e.g.
 */
// #include "fsl_wdog.h"
// void yield() { WDOG_Refresh(WDOG); }
