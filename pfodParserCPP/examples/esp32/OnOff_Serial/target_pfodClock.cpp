/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
/*
 * target_pfodClock.cpp -- micros() (required) and yield() (optional)
 * for ESP-IDF (ESP32) and ESP8266_RTOS_SDK
 *
 * pfodParserCPP does not implement micros(), it must be supplied by your program.
 * millis() (in pfodParserCPP/src/pfodClock.cpp) is derived from it.
 * See "Clock: micros(), millis() and yield()" in pfodParserCPP/BUILDING.md
 */

#include "pfodClock.h"
#include "esp_timer.h"

/**
 * Microseconds since boot, from esp_timer_get_time(), a 64 bit uS count.
 * Casting it to unsigned long truncates the count, so it counts through the
 * full unsigned long range and wraps from its maximum value back to 0
 * (every ~71.6 minutes for 32 bit), as required.
 * @return the current microsecond count
 */
unsigned long micros() {
  return (unsigned long)esp_timer_get_time();
}

/*
 * yield()
 * Only called by pfodBufferedStream: by write() in blocking mode, while it
 * waits in 1 ms steps for transmit buffer space, and by flush(), while it
 * waits for the buffer to empty.
 * The library's default yield() (pfodClock.cpp) is weak and empty. This
 * yield() replaces it at link time, so other tasks keep running while it waits.
 * yield() is NOT defined by ESP-IDF, ESP8266_RTOS_SDK or FreeRTOS.
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * Let every other task run while pfodBufferedStream waits.
 * vTaskDelay(1) blocks for 1 tick so every task runs, including the lowest
 * priority IDLE task that resets the task watchdog.
 * (taskYIELD() would only let tasks of equal or higher priority run.)
 * With the ESP-IDF default 100 Hz tick, it blocks for 10 ms, and as
 * pfodBufferedStream releases at most one byte per yield() call, a blocked
 * write()/flush() then sends ~100 bytes/sec. Set CONFIG_FREERTOS_HZ=1000
 * if that is too slow.
 */
void yield() {
  vTaskDelay(1);
}
