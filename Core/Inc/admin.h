/*
 * admin.h
 *
 *  Created on: Sep 28, 2024
 *      Author: Gabriela
 */

#ifndef INC_ADMIN_H_
#define INC_ADMIN_H_

#include <stdint.h>

#define ADMIN_USER_LENGTH 8
#define SECURITY_CODE_LENGTH 5

extern volatile uint8_t waiting_for_username;
extern volatile uint8_t waiting_for_new_code;
extern volatile uint8_t admin_mode;
extern volatile uint8_t waiting_for_code;
extern volatile uint8_t display_incorrect_message;
extern volatile uint8_t system_blocked;

void process_admin(void);
void read_user(void);
void check_username(void);
void process_code(void);
void read_code(void);
void check_code(void);
void process_newcode(void);
void read_newcode(void);
void update_security_code(const char* new_code);

#endif /* INC_ADMIN_H_ */
