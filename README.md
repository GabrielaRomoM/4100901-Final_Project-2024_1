#  Simple Smart Lock System (Sistema de Candado/Bloqueo Inteligente sencillo)

Sistema de bloqueo y desbloqueo controlado de manera manual, integrando perifericos fisicos (botones, LEDs, teclado hexadecimal), utilizando modulo ESP01 para comunicación en red permitiendo cambio de clave de acceso, y visualización de información en una pantalla OLED SSD1306.

Manually controlled locking and unlocking system, integrating physical peripherals (buttons, LEDs, hexadecimal keyboard), using ESP01 module for network communication allowing password change, and information display on an SSD1306 OLED screen.

## Functional Features:

### Notification LEDs:
* Implementation of a LED system to notify visually user after do some actions (Block, Correct Unlock, Incorrect Unlock).
* LEDs are controlled by physical buttons and remote comands (UART).
* Implementation realized with a library called Led.

### Hexadecimal Keyboard: 
* Allow the entry of specific commands to unlock the system through a predefined code, and its validation (#) or reset (*).

### Serial Console:
* Interact with the system through a console (YAT) on the PC using USART2.
* Shows on the YAT the most relevant events ocurring between the OLED and the ESP01.

### OLED SSD1306 Screen:
* Shows the actual state of the lock (UNLOCKED, BLOCKED or ADMIN MODE).
* Shows the code entered by the user when is trying to UNLOCK the system and notify if the code its incorret.

### Wi-fi Conection (ESP01):
* ESP01 connected with ESP-Link to connect to a Wi-fi network.
* Remote control of the system through comands sent for Wi-fi through ESP01.
* To Commands refers to Admin Mode, who let to user change the code after verify their identity (user and actual code).
* This implementation is realized on a library called Admin. 

## Non-Functional Features:

### Response Time:
* System reacts to remote commands in at least 2 seconds (it may take longer if it is by ESP01).

### Maintenance:
* Code is organized with some libraries to facilite future modifications or improves (libraries include: Keypad, Buffer, LEDs, Admin Mode).

### Security:
* System implements validations through an USER and CODE to allow changes on the code lock (ADMIN MODE).
