/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ssd1306.h" // OLED display library
#include "ssd1306_fonts.h" // Fonts for the OLED Display

#include "ring_buffer.h" // Handles ring buffer for UART
#include "keypad.h" // Keypad Input Handler
#include "led.h" // LED Control Functions
#include "admin.h" // Admin Mode Functions
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

RTC_HandleTypeDef hrtc;

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
// USART Buffers for USART2 and USART3
#define USART2_BUFFER_SIZE 8
uint8_t usart2_buffer[USART2_BUFFER_SIZE]; //Data Buffer for USART2
ring_buffer_t usart2_rb; // Ring Buffer for USART2
uint8_t usart2_rx; // Stores byte received from USART2

#define USART3_BUFFER_SIZE 64
uint8_t usart3_buffer[USART2_BUFFER_SIZE]; // Data Buffer for USART3
ring_buffer_t usart3_rb; // Ring Buffer for USART3
uint8_t usart3_rx; // Stores byte received from USART3

// Variables to track system state
uint32_t left_toggles = 0; // Toggle counter for button
uint32_t left_last_press_tick = 0; // Last time button was pressed

// Security code settings
#define SECURITY_CODE_LENGTH 5
char security_code[SECURITY_CODE_LENGTH + 1] = "12345"; //Default security code
char input_code[SECURITY_CODE_LENGTH + 1] = {0}; // Buffer for user-entered code
int input_index = 0; // Tracks position in the input code

//System State Flags
volatile uint8_t system_blocked = 1; // Start with system blocked
volatile uint8_t display_incorrect_message = 0; // Flag to show incorrect code message
volatile uint8_t waiting_for_username = 0; // Waiting for username flag
volatile uint8_t waiting_for_password = 0; // Waiting for code on unlock/block flag
volatile uint8_t waiting_for_new_code = 0; // Waiting for new security code
volatile uint8_t waiting_for_code = 0; // Waiting for code on admin mode flag
volatile uint8_t admin_mode = 0; // Admin mode enabled
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_RTC_Init(void);
/* USER CODE BEGIN PFP */
// Function Prototypes
void block_system(void); // Block the system (Locks it)
void unblock_system(void); // Unblocks the system (Unlocks it)
void process_keypad_input(char key); //Process input from the keypad
void check_security_code(void); // Verifies the entered security code
void update_led_state(void); // Updates the LED state based on system status
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// Custom UART write function for printf output
int _write(int file, char *ptr, int len)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, 10);
  return len;
}

// UART receive complete callback function
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	 if (huart->Instance == USART2) {
		  usart2_rx = USART2->RDR; // Read received type
		  ring_buffer_write(&usart2_rb, usart2_rx); // Store it in buffer
		  ATOMIC_SET_BIT(USART2->CR1, USART_CR1_RXNEIE); //Enable interrupt for next byte
	 } else if (huart->Instance == USART3){
        ring_buffer_write(&usart3_rb, usart3_rx);
        HAL_UART_Receive_IT(&huart3, &usart3_rx, 1); // Restart interrupt reception
	 }
}

// Callback function for GPIO (button press)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	uint8_t key_pressed = keypad_scan(GPIO_Pin);
	if (key_pressed != 0xFF) {
		process_keypad_input(key_pressed); //Process Keypad Input if Valid
		return;
	}
	// Handle specific button presses (Block, Unlock, Admin mode)
    if (GPIO_Pin == BUTTON_BLOCK_Pin) {
        block_system();
    } else if (GPIO_Pin == BUTTON_UNLOCK_Pin) {
    	waiting_for_password = 1;
    }else if (GPIO_Pin == BUTTON_ADMIN_Pin) {
    	waiting_for_username = 1;
    	process_admin();
    }
}
// Clears the input code buffer
void clear_input_code() {
    memset(input_code, 0, sizeof(input_code)); //Clear the buffer
    input_index = 0; // Reset index
}
// Processes input from keypad
void process_keypad_input(char key) {
    if (waiting_for_password == 1) {
        if (key == '#') {
            check_security_code(); //Validate the entered code
        } else if (key == '*') {
            clear_input_code(); //Clear the entered code
        } else if (input_index < SECURITY_CODE_LENGTH) {
            input_code[input_index++] = key; //Add new character to the code
            printf("Code Entered: %s\r\n",input_code);
        }
    }
}
// Checks if the entered security code is correct
void check_security_code(void) {
    if (strcmp(input_code, security_code) == 0) {
        printf("\r\nCorrect code. System unlocked.\r\n");
        waiting_for_password = 0;
        system_blocked = 0;
        unblock_system(); // Unlock the system
        unblock_system_led(); // LEDs active when the system is unlocked correctly
    } else {
        printf("\r\nIncorrect code. System remains locked.\r\n");
        display_incorrect_message = 1; //Show Error Message
        system_blocked = 1;
        waiting_for_password = 0; // Clear the input code after checking
    }
    clear_input_code(); // Reset buffer
}

// Block (Locks) the system
void block_system() {
    system_blocked = 1;
    waiting_for_password = 0;
    prepare_for_code_entry(); // Turn off the active LEDs
    block_system_led(); // Turn on LEDs when the system is blocked correctly
    printf("System blocked\r\n");
}
// Unblocks (unlocks) the system
void unblock_system() {
    if (system_blocked) {
        prepare_for_code_entry(); // Turn off the active LEDs
        waiting_for_password = 1;
        system_blocked = 0;
        clear_input_code(); // Reset Buffer
    }
}

void low_power_mode()
{
#define AWAKE_TIME (30 * 1000) // 30 segundos
	static uint32_t sleep_tick = AWAKE_TIME;

	if (sleep_tick > HAL_GetTick()) {
		return;
	}
	printf("Sleeping\r\n");
	sleep_tick = HAL_GetTick() + AWAKE_TIME;

	RCC->AHB1SMENR  = 0x0;
	RCC->AHB2SMENR  = 0x0;
	RCC->AHB3SMENR  = 0x0;

	RCC->APB1SMENR1 = 0x0;
	RCC->APB1SMENR2 = 0x0;
	RCC->APB2SMENR  = 0x0;

	/*Suspend Tick increment to prevent wakeup by Systick interrupt.
	Otherwise the Systick interrupt will wake up the device within 1ms (HAL time base)*/
	HAL_SuspendTick();

	/* Enter Sleep Mode , wake up is done once User push-button is pressed */
	HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

	/* Resume Tick interrupt if disabled prior to SLEEP mode entry */
	HAL_ResumeTick();

	printf("Awake\r\n");
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */
  ssd1306_Init(); // Initialize the OLED display
  ring_buffer_init(&usart2_rb, usart2_buffer, USART2_BUFFER_SIZE); // Initialize USART2 buffer
  ring_buffer_init(&usart3_rb, usart3_buffer, USART3_BUFFER_SIZE); // Initialize USART3 buffer

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  printf("Starting...\r\n");
  ATOMIC_SET_BIT(USART2->CR1, USART_CR1_RXNEIE); // Start UART reception interrupt for USART2
  HAL_UART_Receive_IT(&huart3, &usart3_rx, 1); // Start UART reception interrupt for USART3
  while (1) {
	  update_led_state(); // Update LEDs based on system state

	  //Activation of Admin Mode (Process to change password)
      if(waiting_for_username == 1){
    	  read_user();
      }
      if (waiting_for_code == 1){
    	  read_code();
      }
      if (waiting_for_new_code == 1){
    	  read_newcode();
      }

      // Printing messages on OLED accord to the flags and current functions
      if (display_incorrect_message) {
          ssd1306_Fill(Black);
          ssd1306_SetCursor(12, 24);
          ssd1306_WriteString("INCORRECT CODE", Font_7x10, White);
          HAL_Delay(200);
          ssd1306_UpdateScreen();
          display_incorrect_message = 0;
          system_blocked = 1;
      } else if (system_blocked == 0) {
          ssd1306_Fill(Black);
          ssd1306_SetCursor(12, 24);
          ssd1306_WriteString("SYSTEM UNLOCKED", Font_7x10, White);
          ssd1306_UpdateScreen();
      } else if (waiting_for_password == 1) {
          ssd1306_Fill(Black);
          ssd1306_SetCursor(12, 24);
          ssd1306_WriteString("ENTER CODE:", Font_7x10, White);
          ssd1306_SetCursor(12, 40);
          ssd1306_WriteString(input_code, Font_7x10, White);
          ssd1306_UpdateScreen();
      } else if (admin_mode == 1){
          ssd1306_Fill(Black);
          ssd1306_SetCursor(12, 24);
          ssd1306_WriteString("ADMIN MODE", Font_7x10, White);
          HAL_Delay(250);
          ssd1306_UpdateScreen();
      } else {
          ssd1306_Fill(Black);
          ssd1306_SetCursor(12, 24);
          ssd1306_WriteString("SYSTEM BLOCKED", Font_7x10, White);
          HAL_Delay(500);
          ssd1306_UpdateScreen();
      }

      HAL_Delay(100);

	  low_power_mode();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00000E14;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LED_IN_Pin|LED_BLOCK_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ROW_1_GPIO_Port, ROW_1_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, ROW_2_Pin|ROW_4_Pin|ROW_3_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_UNLOCK_GPIO_Port, LED_UNLOCK_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : BUTTON_BLOCK_Pin BUTTON_UNLOCK_Pin */
  GPIO_InitStruct.Pin = BUTTON_BLOCK_Pin|BUTTON_UNLOCK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_IN_Pin LED_BLOCK_Pin */
  GPIO_InitStruct.Pin = LED_IN_Pin|LED_BLOCK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : BUTTON_ADMIN_Pin */
  GPIO_InitStruct.Pin = BUTTON_ADMIN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BUTTON_ADMIN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : COLUMN_1_Pin */
  GPIO_InitStruct.Pin = COLUMN_1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(COLUMN_1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : COLUMN_4_Pin */
  GPIO_InitStruct.Pin = COLUMN_4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(COLUMN_4_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : COLUMN_2_Pin COLUMN_3_Pin */
  GPIO_InitStruct.Pin = COLUMN_2_Pin|COLUMN_3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : ROW_1_Pin */
  GPIO_InitStruct.Pin = ROW_1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(ROW_1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ROW_2_Pin ROW_4_Pin ROW_3_Pin */
  GPIO_InitStruct.Pin = ROW_2_Pin|ROW_4_Pin|ROW_3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_UNLOCK_Pin */
  GPIO_InitStruct.Pin = LED_UNLOCK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_UNLOCK_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
