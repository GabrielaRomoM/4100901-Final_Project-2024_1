/*
 * led.h
 *
 *  Created on: Sep 28, 2024
 *      Author: Gabriela
 */

#ifndef INC_LED_H_
#define INC_LED_H_

#include "stm32l4xx_hal.h"

#define TOGGLE_PERIOD 500 // Period for toggling in milliseconds
#define BLINK_COUNT 6     // Number of toggles (3 on-off cycles)

// Static volatile variables to manage LED blinking
static volatile uint32_t led_toggles = 0;   // Counter for LED toggles
static volatile uint32_t last_toggle_tick = 0; // Last tick count when LED was toggled
static volatile uint16_t blinking_led_pin = 0; // Pin of the currently blinking LED

// Function prototypes
void update_led_state(void);  // Updates the LED state based on toggling logic
void start_led_blinking(GPIO_TypeDef* gpio_port, uint16_t gpio_pin); // Starts the LED blinking process
void block_system_led(void); // Activates the block system LED
void unblock_system_led(void); // Activates the unblock system LED
void incorrect_system_led(void); // Activates the incorrect input LED
void prepare_for_code_entry(void); // Prepares the system LED for code entry

#endif /* INC_LED_H_ */
