/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
/*
 * target_Pin.cpp -- the output pin the OnOff_Serial menu turns on and off,
 * for Zephyr (including nRF Connect SDK), using the GPIO devicetree API as the
 * Zephyr blinky sample does. Needs CONFIG_GPIO=y in prj.conf.
 *
 * The default pin is the board's devicetree led0 alias, e.g. LED1 (P0.13) on the
 * nRF52840 DK, LED0 (P2.09) on the nRF54L15 DK, LD2 (PA5) on Nucleo-64 boards.
 * The pin's active level (GPIO_ACTIVE_LOW / GPIO_ACTIVE_HIGH) also comes from the
 * devicetree, so on always lights the LED.
 * For another pin set PFOD_PIN_NODE to a devicetree node with a gpios property, e.g. add
 *   zephyr_compile_definitions(PFOD_PIN_NODE=DT_ALIAS(led1))
 * to CMakeLists.txt , OR edit the define below, OR add a led0 alias in your board overlay.
 * Do not use the pfod UART's pins (see main.cpp).
 *
 * Compiled and linked for the nRF52840 DK against Zephyr v4.5.0 (build.sh) and the Zephyr in nRF Connect SDK v3.4.1, not tested on hardware -- check against your Zephyr version.
 */
#include "target_Pin.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#ifndef PFOD_PIN_NODE
#define PFOD_PIN_NODE DT_ALIAS(led0)
#endif

// the pin's GPIO device, pin number and devicetree flags (including the active level)
static const struct gpio_dt_spec pfodPin = GPIO_DT_SPEC_GET(PFOD_PIN_NODE, gpios);

// true once the pin has been configured as an output, target_PinWrite() does nothing until then
static bool pfodPinReady = false;

/**
 * Configure PFOD_PIN_NODE's pin as an output, initially off (inactive).
 * If the GPIO device is not ready or the configure fails, prints an error
 * with printk and leaves the pin unconfigured, so target_PinWrite() does nothing.
 */
void target_PinInit() {
  if (!gpio_is_ready_dt(&pfodPin)) {
    printk("target_PinInit: GPIO device %s is not ready\n", pfodPin.port->name);
    return;
  }
  int err = gpio_pin_configure_dt(&pfodPin, GPIO_OUTPUT_INACTIVE);
  if (err != 0) {
    printk("target_PinInit: gpio_pin_configure_dt failed, error %d\n", err);
    return;
  }
  pfodPinReady = true;
}

/**
 * Set the pin active for on, inactive for off. gpio_pin_set_dt() applies the
 * devicetree active level, so active is low for a GPIO_ACTIVE_LOW pin.
 * Does nothing if target_PinInit() failed (it has printed the error).
 * @param on true to turn the output on, false to turn it off
 */
void target_PinWrite(bool on) {
  if (!pfodPinReady) {
    return;
  }
  (void)gpio_pin_set_dt(&pfodPin, on ? 1 : 0); // only fails for an invalid port/pin, checked by target_PinInit()
}
