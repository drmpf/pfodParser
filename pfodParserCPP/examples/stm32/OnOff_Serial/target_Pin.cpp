/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
/*
 * target_Pin.cpp -- the output pin the OnOff_Serial menu turns on and off,
 * for STM32 using the STM32Cube HAL GPIO driver.
 *
 * The default is PA5, the green user LED LD2 on Nucleo-64 boards (e.g. Nucleo-F401RE,
 * F446RE, L476RG), which is lit when the pin is high. These boards' virtual COM port
 * is USART2, the huart2 used in main.cpp.
 * Set PFOD_PIN_PORT, PFOD_PIN, PFOD_PIN_CLK_ENABLE() and PFOD_PIN_ON_LEVEL for your
 * board, e.g. add
 *   -DPFOD_PIN_PORT=GPIOB -DPFOD_PIN=GPIO_PIN_7 "-DPFOD_PIN_CLK_ENABLE()=__HAL_RCC_GPIOB_CLK_ENABLE()"
 * , OR edit the defines below,
 * to your build for LD2 (blue, PB7) on a Nucleo-144 board.
 * For an LED that is lit when the pin is low, set PFOD_PIN_ON_LEVEL to GPIO_PIN_RESET.
 * Do not use the UART's TX/RX pins (PA2/PA3 for USART2 on Nucleo-64).
 *
 * If CubeMX already configures this pin as an output (e.g. LD2 in MX_GPIO_Init()),
 * target_PinInit() just sets it up again, the same way.
 *
 * The HAL header is chosen by PFOD_STM32_HAL_INCLUDE, as in target_Stream.h.
 */
#include "target_Pin.h"

#ifndef PFOD_STM32_HAL_INCLUDE
#define PFOD_STM32_HAL_INCLUDE "stm32f4xx_hal.h"
#endif
#include PFOD_STM32_HAL_INCLUDE

#ifndef PFOD_PIN_PORT
#define PFOD_PIN_PORT GPIOA
#endif
#ifndef PFOD_PIN
#define PFOD_PIN GPIO_PIN_5
#endif
#ifndef PFOD_PIN_CLK_ENABLE
#define PFOD_PIN_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE() // the clock for PFOD_PIN_PORT
#endif
#ifndef PFOD_PIN_ON_LEVEL
#define PFOD_PIN_ON_LEVEL GPIO_PIN_SET // the pin level for on, GPIO_PIN_SET = high, GPIO_PIN_RESET = low
#endif

/**
 * Enable the port's clock, set the pin's output level to off, then configure
 * it as a push-pull output, no pull up/down, low speed.
 * The level is set before the pin becomes an output so it does not glitch on.
 * HAL_GPIO_Init() returns nothing, there is no failure to report.
 */
void target_PinInit() {
  PFOD_PIN_CLK_ENABLE();
  target_PinWrite(false);
  GPIO_InitTypeDef pinInit = {}; // zero any fields not set below (e.g. Alternate)
  pinInit.Pin = PFOD_PIN;
  pinInit.Mode = GPIO_MODE_OUTPUT_PP;
  pinInit.Pull = GPIO_NOPULL;
  pinInit.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(PFOD_PIN_PORT, &pinInit);
}

/**
 * Set PFOD_PIN to PFOD_PIN_ON_LEVEL for on, or the other level for off.
 * @param on true to turn the output on, false to turn it off
 */
void target_PinWrite(bool on) {
  GPIO_PinState offLevel = (PFOD_PIN_ON_LEVEL == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET;
  HAL_GPIO_WritePin(PFOD_PIN_PORT, PFOD_PIN, on ? PFOD_PIN_ON_LEVEL : offLevel);
}
