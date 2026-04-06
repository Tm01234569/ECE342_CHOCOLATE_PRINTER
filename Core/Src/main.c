/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/*
 * SSD1306 Skeleton Code uses ECE342 Lab Code:
 *
 * ssd1306.c
 * fonts.h
 * ssd1306.h
 *
 * Author: ECE342 Labs
 *
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "image_data.h"
#include "imageConvert.h"
#include <stdbool.h>


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
I2C_HandleTypeDef hi2c2;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart3;

PCD_HandleTypeDef hpcd_USB_OTG_FS;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
static void MX_TIM2_Init(void);
static void MX_I2C2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define DIR_PIN_X GPIO_PIN_15
#define DIR_PORT_X GPIOF
#define STEP_PIN_X GPIO_PIN_13
#define STEP_PORT_X GPIOE
#define MS1_PORT_X GPIOF
#define MS1_PIN_X GPIO_PIN_13
#define MS2_PORT_X GPIOE
#define MS2_PIN_X GPIO_PIN_9
#define MS3_PORT_X GPIOE
#define MS3_PIN_X GPIO_PIN_11

#define DIR_PIN_Y GPIO_PIN_6
#define DIR_PORT_Y GPIOA
#define STEP_PIN_Y GPIO_PIN_5
#define STEP_PORT_Y GPIOA

#define DIR_PIN_EXTRUDE GPIO_PIN_12
#define DIR_PORT_EXTRUDE GPIOE
#define STEP_PIN_EXTRUDE GPIO_PIN_10
#define STEP_PORT_EXTRUDE GPIOE

#define ENDSTOPPER_PIN_INPUT_X GPIO_PIN_14
#define ENDSTOPPER_PORT_INPUT_X GPIOE

#define ENDSTOPPER_PIN_INPUT_Y GPIO_PIN_15
#define ENDSTOPPER_PORT_INPUT_Y GPIOE

bool run = false;

// Function to set servo angle
void Set_Servo_Angle(TIM_HandleTypeDef *htim, uint32_t channel, uint8_t angle)
{
    uint32_t pulse = 500 + (angle * (2500 - 500) / 180);
    __HAL_TIM_SET_COMPARE(htim, channel, pulse);
}

void microDelay (uint16_t delay)
{
  __HAL_TIM_SET_COUNTER(&htim1, 0);
  while (__HAL_TIM_GET_COUNTER(&htim1) < delay);
}

void step_X (int steps, uint8_t direction, uint16_t delay) {
 // Set the motor direction
 HAL_GPIO_WritePin(DIR_PORT_X, DIR_PIN_X, direction ? GPIO_PIN_SET : GPIO_PIN_RESET);
 for(int x = 0; x < steps; x++) {
   HAL_GPIO_WritePin(STEP_PORT_X, STEP_PIN_X, GPIO_PIN_SET);
   microDelay(10);
   HAL_GPIO_WritePin(STEP_PORT_X, STEP_PIN_X, GPIO_PIN_RESET);
   microDelay(delay);
 }
}

void step_Y (int steps, uint8_t direction, uint16_t delay) {
  HAL_GPIO_WritePin(DIR_PORT_Y, DIR_PIN_Y, direction ? GPIO_PIN_SET : GPIO_PIN_RESET);
  for(int x=0; x<steps; x++) {
    HAL_GPIO_WritePin(STEP_PORT_Y, STEP_PIN_Y, GPIO_PIN_SET);
    microDelay(10); // Fixed short pulse
    HAL_GPIO_WritePin(STEP_PORT_Y, STEP_PIN_Y, GPIO_PIN_RESET);
    microDelay(delay); // Speed controlstep_X(1, 0, 10);
  }
}

void step_EXTRUDE (int steps, uint8_t direction, uint16_t delay) {
  HAL_GPIO_WritePin(DIR_PORT_EXTRUDE, DIR_PIN_EXTRUDE, direction ? GPIO_PIN_SET : GPIO_PIN_RESET);
  for(int x=0; x<steps; x++) {
    HAL_GPIO_WritePin(STEP_PORT_EXTRUDE, STEP_PIN_EXTRUDE, GPIO_PIN_SET);
    microDelay(10); // Fixed short pulse
    HAL_GPIO_WritePin(STEP_PORT_EXTRUDE, STEP_PIN_EXTRUDE, GPIO_PIN_RESET);
    microDelay(delay); // Speed control
  }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */

void calibrate(void) // Calibrate motors to end stopper position
{
    static uint8_t calibrated = 0;
    if (calibrated) return;   // already calibrated, do nothing

    GPIO_PinState x_state, y_state;

    GPIO_PinState last_x_state = GPIO_PIN_RESET;
    GPIO_PinState last_y_state = GPIO_PIN_RESET;

    OLED_ShowCalibration(last_x_state, last_y_state);

    do
    {
    	x_state = HAL_GPIO_ReadPin(ENDSTOPPER_PORT_INPUT_X, ENDSTOPPER_PIN_INPUT_X);
        y_state = HAL_GPIO_ReadPin(ENDSTOPPER_PORT_INPUT_Y, ENDSTOPPER_PIN_INPUT_Y);

        if (x_state != last_x_state || y_state != last_y_state)
        {
        	OLED_ShowCalibration(x_state, y_state);
        	last_x_state = x_state;
        	last_y_state = y_state;
        }

        if (x_state == GPIO_PIN_RESET)
        {
            step_X(5, 0, 1000);
        }

        if (y_state == GPIO_PIN_RESET)
        {
            step_Y(5, 0, 1000);
        }

        HAL_Delay(10);

    } while (x_state == GPIO_PIN_RESET || y_state == GPIO_PIN_RESET);

    calibrated = 1;   // mark calibration complete
}

void initial(void){ // Initialize nozzle position
	const int X_step = 1010;
	const int Y_step = 550;

	static uint8_t initialized = 0;
	if (initialized) return;   // already initialized, do nothing
	step_X(X_step, 1, 2000);
	step_Y(Y_step, 1, 2000);

	initialized = 1;
}

void test(void){ // To eyeball how many steps
	const int X_test = 950;
		const int Y_test = 400;

		static uint8_t tested = 0;
		if (tested) return;   // already initialized, do nothing
		step_X(X_test, 0, 2000);
		step_Y(Y_test, 0, 2000);

		tested = 1;
}



void rowByRow_RLE(void)
{
    static uint8_t finish_printing = 0;

    const int pixel_step = 38;
    const int pixel_extrude = 5;

    if (finish_printing) return;

    for (int y = 0; y < IMAGE_HEIGHT; y++)
    {
        int current_x = 0;
        int row_x_moves = 0;

        for (int r = 0; r < run_counts[y]; r++)
        {
            int start  = image_runs[y][r].start;
            int length = image_runs[y][r].length;

            int move_pixels = start - current_x;
            if (move_pixels > 0)
            {
            	if (r > 0)
            	{
            		HAL_Delay(3000);   // only between runs, not before first run
            	}
                step_X(move_pixels * pixel_step, 0, 3000);
                row_x_moves += move_pixels * pixel_step;
                current_x = start;
                HAL_Delay(100);
            }

            for (int i = 0; i < length; i++)
            {
                step_EXTRUDE(pixel_extrude, 1, 10000);
                HAL_Delay(250); // Delay between extruding

                if (i < length - 1)
                {
                    step_X(pixel_step, 0, 5000);
                    row_x_moves += pixel_step;
                    current_x++;
                }


            }
        }

        if (row_x_moves > 0)
        {
            HAL_Delay(3000);
        }

        if (y < IMAGE_HEIGHT - 1)
        {
            HAL_Delay(500);

            if (row_x_moves > 0)
            {
                step_X(row_x_moves, 1, 1000);
            }

            HAL_Delay(100);
            step_Y(16, 0, 20000);
            HAL_Delay(100);
        }

        OLED_ShowProgress((y + 1) * 100 / IMAGE_HEIGHT);
    }

    finish_printing = 1;
}


void finish(void) // Move plate out of the way
{
    static uint8_t finished = 0;
    if (finished) return;   // already calibrated, do nothing

    GPIO_PinState x_state, y_state;


    do
    {
        x_state = HAL_GPIO_ReadPin(ENDSTOPPER_PORT_INPUT_X, ENDSTOPPER_PIN_INPUT_X);
        y_state = HAL_GPIO_ReadPin(ENDSTOPPER_PORT_INPUT_Y, ENDSTOPPER_PIN_INPUT_Y);



        if (x_state == GPIO_PIN_RESET)
        {
            step_X(5, 0, 1000);
        }

        if (y_state == GPIO_PIN_RESET)
        {
            step_Y(5, 0, 1000);
        }

        HAL_Delay(10);

    } while (x_state == GPIO_PIN_RESET || y_state == GPIO_PIN_RESET);

    finishX();

    finished = 1;   // mark calibration complete
}

void finishX(void){
	static uint8_t moved = 0;
	if (moved) return;   // already calibrated, do nothing

	step_X(1150, 1, 2000);

	moved = 1;
}





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
  MX_TIM1_Init();
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_TIM2_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */
  // Start the PWM signal generation
  	 HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
  // 1. Start the Timer hardware so microDelay works
    HAL_TIM_Base_Start(&htim1);

    // 2. Bring PD14 HIGH to wake up the A4988 driver
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);

    // Initialize all MS to high
//    HAL_GPIO_WritePin(MS1_PORT_X, MS1_PIN_X, GPIO_PIN_SET);
//    HAL_GPIO_WritePin(MS2_PORT_X, MS2_PIN_X, GPIO_PIN_SET);
//    HAL_GPIO_WritePin(MS3_PORT_X, MS3_PIN_X, GPIO_PIN_SET);


    // Give the driver a tiny moment to wake up
    HAL_Delay(10);
  /* USER CODE END 2 */
    SSD1306_Init();

    OLED_ShowStartScreen();

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	  if(run == true){
		  calibrate();
		  HAL_Delay(1000);
		  initial();
		  HAL_Delay(1000);
		  rowByRow_RLE();
		  finish();
		  OLED_ShowEndScreen();
	  }


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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 100000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 168-1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 168-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 20000-1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

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
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief USB_OTG_FS Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_OTG_FS_PCD_Init(void)
{

  /* USER CODE BEGIN USB_OTG_FS_Init 0 */

  /* USER CODE END USB_OTG_FS_Init 0 */

  /* USER CODE BEGIN USB_OTG_FS_Init 1 */

  /* USER CODE END USB_OTG_FS_Init 1 */
  hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
  hpcd_USB_OTG_FS.Init.dev_endpoints = 6;
  hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_OTG_FS.Init.Sof_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.vbus_sensing_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_OTG_FS_Init 2 */

  /* USER CODE END USB_OTG_FS_Init 2 */

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD1_Pin|LD3_Pin|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12
                          |GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : USER_Btn_Pin */
  GPIO_InitStruct.Pin = USER_Btn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USER_Btn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PA5 PA6 PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LD1_Pin LD3_Pin LD2_Pin */
  GPIO_InitStruct.Pin = LD1_Pin|LD3_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PF12 PF13 PF14 PF15 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : PE9 PE10 PE11 PE12
                           PE13 */
  GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12
                          |GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PE14 PE15 */
  GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PD14 PD15 */
  GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_OverCurrent_Pin */
  GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
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
#ifdef USE_FULL_ASSERT
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
