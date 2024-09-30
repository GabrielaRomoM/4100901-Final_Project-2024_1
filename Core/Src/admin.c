/*
 * admin.c
 *
 *  Created on: Sep 28, 2024
 *      Author: Gabriela
 */


#include "admin.h"
#include <string.h>
#include <stdio.h>
#include "ring_buffer.h"
#include "stm32l4xx_hal.h"

#define USART3_BUFFER_SIZE 64

extern UART_HandleTypeDef huart3; //UART3 Handle
extern ring_buffer_t usart3_rb; // Ring Buffer for UART3
extern char security_code[]; // Security Code '12345' (Default)

char username[ADMIN_USER_LENGTH + 1] = "gabriela"; // Default Admin User
char input_user[ADMIN_USER_LENGTH + 1] = {0}; // Buffer user for input
int user_index = 0; // Index to track user input position

char admin_code[SECURITY_CODE_LENGTH + 1] = {0}; // Buffer for admin mode
int admin_index = 0; // Index to track code input position

char new_code[SECURITY_CODE_LENGTH + 1] = {0}; // Buffer for new security code
int new_index = 0; // Index to track new code input position

/**
 * @brief This function activates the admin mode and solicit the username
 *
 * @param None
 *
 * @retval None
 */
void process_admin(void) {
    admin_mode = 1;
    if (waiting_for_username == 1) {
        HAL_UART_Transmit(&huart3, (uint8_t*)"ENTER USERNAME: \r\n", 18, 50);
    }
    waiting_for_username = 1;
}

/**
 *@brief Reads the username input from the buffer, processes it, and calls the username check.
 *
 * @param None
 *
 * @retval None
 */
void read_user(void) {
    uint8_t data;
    if (ring_buffer_read(&usart3_rb, &data) == 1) {
        char letter = (char)data;
        if ((letter == '\r') || (letter == '\n')) {
            input_user[user_index] = '\0';  // Null-terminate the string
            check_username(); // Validate username
        } else if (user_index < ADMIN_USER_LENGTH) {
            input_user[user_index++] = letter; // Add character to input buffer
            printf("%c", letter); // Print character on YAT
        }
    }
}

/**
 * @brief Validates the entered username with the stored admin username.
 *        Proceeds to code verification if correct, otherwise blocks the system.
 *
 * @retval None
 */
void check_username(void) {
    if (strcmp(input_user, username) == 0) {
        printf("\r\nCorrect username.\r\n");
        waiting_for_username = 0;
        waiting_for_code = 1;
        process_code(); // Proceed to code entry
    } else {
        printf("\r\nIncorrect username.\r\n");
        HAL_UART_Transmit(&huart3, (uint8_t*)"INCORRECT USER\r\n", 16, 50);
        display_incorrect_message = 1; // Flag to display incorrect username
        system_blocked = 1; // Block system
        waiting_for_code = 0;
    }
}

/**
 * @brief Prepares for the admin code entry by prompting the user.
 *
 * @retval None
 */
void process_code(void) {
    admin_mode = 1;
    if (waiting_for_code == 1) {
        HAL_UART_Transmit(&huart3, (uint8_t*)"ENTER CODE: \r\n", 14, 50);
    }
    waiting_for_code = 1;
    ring_buffer_reset(&usart3_rb); // Clear the buffer for new input
}

/**
 * @brief Reads the admin code from the buffer, processes it, and calls the code check.
 *
 * @retval None
 */
void read_code(void) {
    uint8_t data;
    if (ring_buffer_read(&usart3_rb, &data) == 1) {
        char code = (char)data;
        if ((code == '\r') || (code == '\n')) {
            admin_code[admin_index] = '\0';  // Null-terminate the string
            check_code(); // Validate code
        } else if (admin_index < SECURITY_CODE_LENGTH) {
            admin_code[admin_index++] = code; // Add character to input buffer
            printf("%c", code); // Print character on YAT
        }
    }
}

/**
 * @brief Validates the entered admin code with the stored security code.
 *        Proceeds to new code entry if correct, otherwise blocks the system.
 *
 * @retval None
 */
void check_code(void) {
    if (strcmp(admin_code, security_code) == 0) {
        printf("\r\nCorrect code.\r\n");
        waiting_for_code = 0;
        waiting_for_new_code = 1;
        process_newcode(); // Proceed to new code entry
    } else {
        printf("\r\nIncorrect code.\r\n");
        HAL_UART_Transmit(&huart3, (uint8_t*)"INCORRECT CODE\r\n", 16, 50);
        display_incorrect_message = 1; // Flag to display incorrect code
        system_blocked = 1; // Block system
        waiting_for_code = 0;
    }
}

/**
 * @brief Prepares for the new security code entry by prompting the user.
 *
 * @retval None
 */
void process_newcode(void) {
    admin_mode = 1;
    if (waiting_for_new_code == 1) {
        HAL_UART_Transmit(&huart3, (uint8_t*)"ENTER NEW CODE: \r\n", 18, 50);
    }
    waiting_for_new_code = 1;
    ring_buffer_reset(&usart3_rb); // Clear the buffer for new input
}

/**
 * @brief Reads the new security code from the buffer and updates the stored security code.
 *
 * @retval None
 */
void read_newcode(void) {
    uint8_t data;
    if (ring_buffer_read(&usart3_rb, &data) == 1) {
        char new = (char)data;
        if ((new == '\r') || (new == '\n')) {
            new_code[new_index] = '\0';  // Null-terminate the string
            update_security_code(new_code); // Update security code
            waiting_for_new_code = 0;  // Reset the waiting flag
            HAL_UART_Transmit(&huart3, (uint8_t*)"Succesfull Update.\r\n", 20, 50);
        } else if (new_index < SECURITY_CODE_LENGTH) {
            new_code[new_index++] = new; // Add character to input buffer
            printf("%c", new); // Print character on YAT
        }
    }
}

/**
 * @brief Updates the stored security code with the new code provided by the user.
 *
 * @param new_code: New security code to update.
 *
 * @retval None
 */
void update_security_code(const char* new_code) {
    strncpy(security_code, new_code, SECURITY_CODE_LENGTH); // Copy new code
    security_code[SECURITY_CODE_LENGTH] = '\0';  // Ensure null-termination
    admin_mode = 0; //Exit Admin Mode
}
