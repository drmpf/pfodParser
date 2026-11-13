/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
/*
 * target_Pin.cpp -- the output pin the OnOff_Serial menu turns on and off,
 * for ESP32 (ESP-IDF), using driver/gpio.h.
 *
 * The default is GPIO2, the on board LED of many ESP32 DevKit boards (lit when high).
 * Set PFOD_PIN and PFOD_PIN_ON_LEVEL for your board, e.g. add
 *   -DPFOD_PIN=GPIO_NUM_5 -DPFOD_PIN_ON_LEVEL=0
 * , OR edit the defines below,
 * to your build for an LED on GPIO5 that is lit when the pin is low.
 * Do not use the UART pins passed to serialStream.begin() in main.cpp.
 */
#include "target_Pin.h"
#include "driver/gpio.h"
#include "esp_err.h"

#ifndef PFOD_PIN
#define PFOD_PIN GPIO_NUM_2
#endif
#ifndef PFOD_PIN_ON_LEVEL
#define PFOD_PIN_ON_LEVEL 1 // the pin level for on, 1 = high, 0 = low
#endif

/**
 * Configure PFOD_PIN as an output, with no pull up/down or interrupt, and turn it off.
 * ESP_ERROR_CHECK() aborts, with a message on the console, if gpio_config() fails,
 * e.g. if PFOD_PIN is not a valid output pin.
 */
void target_PinInit() {
  gpio_config_t pinConfig = {}; // zero any fields not set below
  pinConfig.pin_bit_mask = (1ULL << PFOD_PIN);
  pinConfig.mode = GPIO_MODE_OUTPUT;
  pinConfig.pull_up_en = GPIO_PULLUP_DISABLE;
  pinConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
  pinConfig.intr_type = GPIO_INTR_DISABLE;
  ESP_ERROR_CHECK(gpio_config(&pinConfig));
  target_PinWrite(false);
}

/**
 * Set PFOD_PIN to PFOD_PIN_ON_LEVEL for on, or the other level for off.
 * @param on true to turn the output on, false to turn it off
 */
void target_PinWrite(bool on) {
  gpio_set_level(PFOD_PIN, on ? PFOD_PIN_ON_LEVEL : !PFOD_PIN_ON_LEVEL);
}
