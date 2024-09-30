/*
 * led.c
 *
 *  Created on: Sep 28, 2024
 *      Author: Gabriela
 */

#include "led.h"
#include "main.h"

static GPIO_TypeDef* blinking_led_port = NULL; // Port of the currently blinking LED

/**
 * @brief Updates the LED state by toggling it if the toggle period has elapsed.
 *        The function checks if the LED should toggle, and turns it off after
 *        the required number of toggles.
 *
 * @retval None
 */
void update_led_state(void) {
    uint32_t current_tick = HAL_GetTick();

    if (led_toggles > 0 && (current_tick - last_toggle_tick) >= TOGGLE_PERIOD) {
        HAL_GPIO_TogglePin(blinking_led_port, blinking_led_pin); //Toggle LED
        led_toggles--; // Decrease the toggle count
        last_toggle_tick = current_tick; // Update the last toggle tick

        if (led_toggles == 0) {
            // Blinking finished, reset the blinking LED port and pin
            blinking_led_port = NULL;
            blinking_led_pin = 0;
        }
    }
}

/**
 * @brief Starts the blinking process for a given LED, initializing the
 *        toggle count and setting the LED state to start blinking.
 *
 * @param gpio_port: GPIO port of the LED.
 * @param gpio_pin: GPIO pin number of the LED.
 *
 * @retval None
 */
void start_led_blinking(GPIO_TypeDef* gpio_port, uint16_t gpio_pin) {
    led_toggles = BLINK_COUNT;
    last_toggle_tick = HAL_GetTick();
    blinking_led_port = gpio_port;
    blinking_led_pin = gpio_pin;
    HAL_GPIO_WritePin(gpio_port, gpio_pin, GPIO_PIN_RESET); // Start with LED on
}

/**
 * @brief Initiates the blinking of the block system LED and keeps it on
 *        after the blinking process finishes.
 *
 * @retval None
 */
void block_system_led(void) {
    start_led_blinking(LED_BLOCK_GPIO_Port, LED_BLOCK_Pin);
    HAL_GPIO_WritePin(LED_BLOCK_GPIO_Port, LED_BLOCK_Pin, GPIO_PIN_RESET); // Keep block LED on after blinking
    HAL_GPIO_WritePin(LED_UNLOCK_GPIO_Port, LED_UNLOCK_Pin, GPIO_PIN_SET); // Ensure unlock LED is off
}

/**
 * @brief Initiates the blinking of the unlock system LED and keeps it on
 *        after the blinking process finishes.
 *
 * @retval None
 */
void unblock_system_led(void) {
    start_led_blinking(LED_UNLOCK_GPIO_Port, LED_UNLOCK_Pin);
    HAL_GPIO_WritePin(LED_UNLOCK_GPIO_Port, LED_BLOCK_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_BLOCK_GPIO_Port, LED_BLOCK_Pin, GPIO_PIN_SET); // Ensure block LED is off
}

/**
 * @brief Turns off all LEDs to indicate an incorrect system state.
 *
 * @retval None
 */
void incorrect_system_led(void){
    HAL_GPIO_WritePin(LED_UNLOCK_GPIO_Port, LED_UNLOCK_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_BLOCK_GPIO_Port, LED_BLOCK_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_IN_GPIO_Port, LED_IN_Pin, GPIO_PIN_RESET);

}

/**
 * @brief Prepares the system for code entry by turning on all LEDs
 *        as an indication that the system is ready to receive input.
 *
 * @retval None
 */
void prepare_for_code_entry(void) {
    HAL_GPIO_WritePin(LED_BLOCK_GPIO_Port, LED_BLOCK_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_UNLOCK_GPIO_Port, LED_UNLOCK_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED_IN_GPIO_Port, LED_IN_Pin, GPIO_PIN_SET);
}
