/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
#ifndef pfodClock_h
#define pfodClock_h
/*
 * Plain C++ replacements for the Arduino micros()/millis()/yield() functions.
 *
 * micros() is the only clock source. millis() is derived from it.
 * micros() is NOT implemented in the library -- your program must supply it,
 * e.g. in target_pfodClock.cpp alongside main.cpp. A std::chrono based
 * target_pfodClock.cpp is generated with the program; edit it to use your
 * board's timer if std::chrono does not work on your micro.
 * The millis() and yield() defaults in pfodClock.cpp are weak, so defining your
 * own replaces them at link time -- do not edit pfodClock.cpp.
 */

/**
 * Microseconds since startup.
 * NOT implemented in the library, your program must supply it
 * (e.g. target_pfodClock.cpp).
 * MUST count through the full unsigned long range and wrap from its maximum
 * value back to 0 (i.e. ~71.6 minutes for a 32 bit unsigned long), so that
 * (micros() - start) gives the correct elapsed time across a wrap.
 * @return the current microsecond count
 */
unsigned long micros();

/**
 * Milliseconds since startup, derived from micros().
 * Accumulates the elapsed micros() since the previous call, so it keeps
 * counting correctly when micros() wraps, and itself wraps from its
 * maximum value back to 0 (~49.7 days for a 32 bit unsigned long).
 * Must be called at least once per micros() wrap period (~71.6 minutes for
 * 32 bit), which pfodParser does every loop. Not safe to call from an interrupt.
 * The default is weak and can be replaced, but normally only micros() is replaced.
 * @return the current millisecond count
 */
unsigned long millis();

/**
 * Called while busy waiting, to let other tasks / the system run.
 * Only called by pfodBufferedStream: by write() in blocking mode, while it
 * waits in 1 ms steps for transmit buffer space, and by flush(), while it
 * waits for the buffer to empty.
 * The default implementation is a weak empty function, so it does nothing.
 * Define your own yield() (non-weak) in any .cpp file of your program for
 * boards that need it -- it replaces the weak default at link time.
 *
 * yield() is NOT defined by ESP-IDF, ESP8266_RTOS_SDK, FreeRTOS or Zephyr.
 * (Only the Arduino cores define one, and that is a C function, a different
 * symbol from this C++ yield(), so it would not replace the default anyway.)
 *
 * Example definitions:
 *
 *   ESP-IDF (ESP32) / ESP8266_RTOS_SDK / other FreeRTOS:
 *     #include "freertos/FreeRTOS.h"   // plain FreeRTOS: "FreeRTOS.h"
 *     #include "freertos/task.h"       // plain FreeRTOS: "task.h"
 *     void yield() { vTaskDelay(1); }  // block for 1 tick, lets every task run
 *   taskYIELD() only lets tasks of equal or higher priority run, NOT the
 *   lower priority IDLE task that resets the task watchdog, so use
 *   vTaskDelay(1) if the wait can be long enough to trip the watchdog.
 *   (With the ESP-IDF default 100 Hz tick, vTaskDelay(1) blocks for 10 ms.
 *   pfodBufferedStream releases at most one byte per yield() call, so a
 *   blocked write()/flush() then sends ~100 bytes/sec. Use a 1000 Hz tick,
 *   CONFIG_FREERTOS_HZ=1000, or taskYIELD() if that is too slow.)
 *     void yield() { taskYIELD(); }    // if only equal/higher priority tasks need to run
 *
 *   Zephyr:
 *     #include <zephyr/kernel.h>
 *     void yield() { k_yield(); }      // equal/higher priority threads only
 *     void yield() { k_msleep(1); }    // or sleep, so lower priority threads also run
 *
 *   ESP8266: only ESP8266_RTOS_SDK (above) is supported.
 *     The ESP8266 NonOS SDK is NOT supported -- its user code runs in SDK
 *     callbacks that must return before WiFi can run, so pfodBufferedStream's
 *     blocking write()/flush() would stall WiFi, and yield() cannot hand
 *     control back to the SDK.
 *
 *   Bare metal (no RTOS, e.g. STM32 HAL, nRF5 SDK, PIC32):
 *     Nothing else needs to run, so the default empty yield() is fine.
 *     If a hardware watchdog is enabled, feed it here, e.g.
 *     void yield() { HAL_IWDG_Refresh(&hiwdg); }   // STM32 HAL
 */
void yield();

#endif // pfodClock_h
