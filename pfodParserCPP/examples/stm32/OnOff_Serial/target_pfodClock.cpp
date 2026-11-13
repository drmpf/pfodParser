/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
/*
 * target_pfodClock.cpp -- micros() (required) and yield() (optional)
 * for STM32 (STM32Cube HAL)
 *
 * pfodParserCPP does not implement micros(), it must be supplied by your program.
 * millis() (in pfodParserCPP/src/pfodClock.cpp) is derived from it.
 * std::chrono::steady_clock does not work with arm-none-eabi / newlib (it fails
 * to link or always returns 0), so this uses the HAL's 1 ms SysTick instead.
 * See "Clock: micros(), millis() and yield()" in pfodParserCPP/BUILDING.md
 *
 * The HAL header to include is chosen by PFOD_STM32_HAL_INCLUDE, as for
 * target_Stream.h, default "stm32f4xx_hal.h".
 */

#include "pfodClock.h"

#ifndef PFOD_STM32_HAL_INCLUDE
#define PFOD_STM32_HAL_INCLUDE "stm32f4xx_hal.h"
#endif
#include PFOD_STM32_HAL_INCLUDE

/**
 * Microseconds since HAL_Init(), from the HAL's 1 ms SysTick count
 * (HAL_GetTick()) plus the uS into the current SysTick period.
 * SysTick counts down from LOAD to 0, once per ms, so (LOAD - VAL) is the
 * number of SysTick clocks into the current ms.
 * Re-reads if the ms tick changed during the calculation, i.e. if the SysTick
 * interrupt happened in between.
 * Uses 32 bit unsigned arithmetic, so it counts through the full unsigned long
 * range and wraps from its maximum value back to 0 (every ~71.6 minutes), as required.
 * Assumes the default HAL SysTick setup: 1 kHz tick, clocked at SystemCoreClock
 * (a multiple of 1 MHz). Returns 0..999 until HAL_Init() has started SysTick.
 * @return the current microsecond count
 */
unsigned long micros() {
  uint32_t ms;
  uint32_t ticks;
  do {
    ms = HAL_GetTick();
    ticks = SysTick->LOAD - SysTick->VAL;
  } while (ms != HAL_GetTick());
  return (unsigned long)(ms * 1000UL + ticks / (SystemCoreClock / 1000000UL));
}

/*
 * yield() -- optional.
 * Only called by pfodBufferedStream: by write() in blocking mode, while it
 * waits in 1 ms steps for transmit buffer space, and by flush(), while it
 * waits for the buffer to empty.
 * The library's default yield() (pfodClock.cpp) is weak and empty, which is
 * fine on bare metal, as nothing else needs to run.
 * If you have the independent watchdog enabled,
 * feed it here, e.g.
 */
// extern IWDG_HandleTypeDef hiwdg; // from your CubeMX generated code
// void yield() { HAL_IWDG_Refresh(&hiwdg); }
