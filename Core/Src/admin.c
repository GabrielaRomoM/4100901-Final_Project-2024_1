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

extern UART_HandleTypeDef huart3;
extern ring_buffer_t usart3_rb;
extern char security_code[];

char username[ADMIN_USER_LENGTH + 1] = "gabriela";
char input_user[ADMIN_USER_LENGTH + 1] = {0};
int user_index = 0;

char admin_code[SECURITY_CODE_LENGTH + 1] = {0};
int admin_index = 0;

char new_code[SECURITY_CODE_LENGTH + 1] = {0};
int new_index = 0;

void process_admin(void) {
    admin_mode = 1;
    if (waiting_for_username == 1) {
        HAL_UART_Transmit(&huart3, (uint8_t*)"ENTER USERNAME: \r\n", 18, 50);
    }
    waiting_for_username = 1;
}

void read_user(void) {
    uint8_t data;
    if (ring_buffer_read(&usart3_rb, &data) == 1) {
        char letter = (char)data;
        if ((letter == '\r') || (letter == '\n')) {
            input_user[user_index] = '\0';  // Null-terminate the string
            check_username();
        } else if (user_index < ADMIN_USER_LENGTH) {
            input_user[user_index++] = letter;
            printf("%c", letter);
        }
    }
}

void check_username(void) {
    if (strcmp(input_user, username) == 0) {
        printf("\r\nCorrect username.\r\n");
        waiting_for_username = 0;
        waiting_for_code = 1;
        process_code();
    } else {
        printf("\r\nIncorrect username.\r\n");
        display_incorrect_message = 1;
        system_blocked = 1;
        waiting_for_code = 0;
    }
}

void process_code(void) {
    admin_mode = 1;
    if (waiting_for_code == 1) {
        HAL_UART_Transmit(&huart3, (uint8_t*)"ENTER CODE: \r\n", 14, 50);
    }
    waiting_for_code = 1;
    ring_buffer_reset(&usart3_rb);
}

void read_code(void) {
    uint8_t data;
    if (ring_buffer_read(&usart3_rb, &data) == 1) {
        char code = (char)data;
        if ((code == '\r') || (code == '\n')) {
            admin_code[admin_index] = '\0';  // Null-terminate the string
            check_code();
        } else if (admin_index < SECURITY_CODE_LENGTH) {
            admin_code[admin_index++] = code;
            printf("%c", code);
        }
    }
}

void check_code(void) {
    if (strcmp(admin_code, security_code) == 0) {
        printf("\r\nCorrect code.\r\n");
        waiting_for_code = 0;
        waiting_for_new_code = 1;
        process_newcode();
    } else {
        printf("\r\nIncorrect code.\r\n");
        display_incorrect_message = 1;
        system_blocked = 1;
        waiting_for_code = 0;
    }
}

void process_newcode(void) {
    admin_mode = 1;
    if (waiting_for_new_code == 1) {
        HAL_UART_Transmit(&huart3, (uint8_t*)"ENTER NEW CODE: \r\n", 18, 50);
    }
    waiting_for_new_code = 1;
    ring_buffer_reset(&usart3_rb);
}

void read_newcode(void) {
    uint8_t data;
    if (ring_buffer_read(&usart3_rb, &data) == 1) {
        char new = (char)data;
        if ((new == '\r') || (new == '\n')) {
            new_code[new_index] = '\0';  // Null-terminate the string
            update_security_code(new_code);
            waiting_for_new_code = 0;  // Reset the waiting flag
            HAL_UART_Transmit(&huart3, (uint8_t*)"Succesfull Update.\r\n", 20, 50);
        } else if (new_index < SECURITY_CODE_LENGTH) {
            new_code[new_index++] = new;
            printf("%c", new);
        }
    }
}

void update_security_code(const char* new_code) {
    strncpy(security_code, new_code, SECURITY_CODE_LENGTH);
    security_code[SECURITY_CODE_LENGTH] = '\0';  // Ensure null-termination
    admin_mode = 0;
}
