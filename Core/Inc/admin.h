/*
 * admin.h
 *
 *  Created on: Sep 28, 2024
 *      Author: Gabriela
 */

#ifndef INC_ADMIN_H_
#define INC_ADMIN_H_

#include <stdint.h>

#define ADMIN_USER_LENGTH 8 // Max length for admin username
#define SECURITY_CODE_LENGTH 5 // Max length for security code

// External volatile variables for system states
extern volatile uint8_t waiting_for_username; // Indicates if waiting for username input
extern volatile uint8_t waiting_for_new_code; // Indicates if waiting for new code input
extern volatile uint8_t admin_mode; // Indicates if in admin mode
extern volatile uint8_t waiting_for_code;  // Indicates if waiting for security code input
extern volatile uint8_t display_incorrect_message; // Flag to display incorrect message
extern volatile uint8_t system_blocked; // Indicates if the system is blocked

// Function prototypes
void process_admin(void); // Starts admin mode, asks for username
void read_user(void); // Reads the username input
void check_username(void); // Checks if the entered username is correct
void process_code(void); // Prepares to enter the security code
void read_code(void); // Reads the security code input
void check_code(void); // Checks if the entered security code is correct
void process_newcode(void); // Prepares to enter a new security code
void read_newcode(void); // Reads the new security code input
void update_security_code(const char* new_code); // Updates the system's security code

#endif /* INC_ADMIN_H_ */
