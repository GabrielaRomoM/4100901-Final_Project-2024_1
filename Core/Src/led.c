/*
 * led.c
 *
 *  Created on: Sep 28, 2024
 *      Author: Gabriela
 */

#include "led.h"
#include "main.h"

static GPIO_TypeDef* blinking_led_port = NULL; // Port of the currently blinking LED

void update_led_state(void) {
    uint32_t current_tick = HAL_GetTick();

    if (led_toggles > 0 && (current_tick - last_toggle_tick) >= TOGGLE_PERIOD) {
        HAL_GPIO_TogglePin(blinking_led_port, blinking_led_pin);
        led_toggles--;
        last_toggle_tick = current_tick;

        if (led_toggles == 0) {
            // Blinking finished
            blinking_led_port = NULL;
            blinking_led_pin = 0;
        }
    }
}

void start_led_blinking(GPIO_TypeDef* gpio_port, uint16_t gpio_pin) {
    led_toggles = BLINK_COUNT;
    last_toggle_tick = HAL_GetTick();
    blinking_led_port = gpio_port;
    blinking_led_pin = gpio_pin;
    HAL_GPIO_WritePin(gpio_port, gpio_pin, GPIO_PIN_RESET); // Start with LED on
}

void block_system_led(void) {
    start_led_blinking(LED_BLOCK_GPIO_Port, LED_BLOCK_Pin);
    HAL_GPIO_WritePin(LED_BLOCK_GPIO_Port, LED_BLOCK_Pin, GPIO_PIN_RESET); // Keep block LED on after blinking
    HAL_GPIO_WritePin(LED_UNLOCK_GPIO_Port, LED_UNLOCK_Pin, GPIO_PIN_SET); // Ensure unlock LED is off
}

void unblock_system_led(void) {
    start_led_blinking(LED_UNLOCK_GPIO_Port, LED_UNLOCK_Pin);
    HAL_GPIO_WritePin(LED_UNLOCK_GPIO_Port, LED_BLOCK_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_BLOCK_GPIO_Port, LED_BLOCK_Pin, GPIO_PIN_SET); // Ensure block LED is off
}

void incorrect_system_led(void){
    HAL_GPIO_WritePin(LED_UNLOCK_GPIO_Port, LED_UNLOCK_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_BLOCK_GPIO_Port, LED_BLOCK_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_IN_GPIO_Port, LED_IN_Pin, GPIO_PIN_RESET);

}

void prepare_for_code_entry(void) {
    HAL_GPIO_WritePin(LED_BLOCK_GPIO_Port, LED_BLOCK_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_UNLOCK_GPIO_Port, LED_UNLOCK_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED_IN_GPIO_Port, LED_IN_Pin, GPIO_PIN_SET);
}
