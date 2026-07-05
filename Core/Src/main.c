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
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "fonts.h"
#include "ssd1306.h"
#include <stdio.h>
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

TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
#define SAMPLE_SIZE 512
#define SAMPLE_RATE 8000.0f

uint32_t last_tap_time = 0;   // 마지막으로 버튼을 누른 시간
uint32_t interval_ms = 1000;  // 메트로놈 박자 간격 (초기값 1초 = 60 BPM)
uint32_t last_beep_time = 0;  // 마지막으로 메트로놈 소리가 난 시간
uint8_t metronome_en = 0;

static int beat_count = 0;      // 0, 1, 2, 3 박자 카운터
char oled_bpm_buf[20];          // "BPM: 120" 등을 담을 문자열 버퍼
char oled_beat_buf[20];         // "o - - -" 등을 담을 문자열 버퍼
uint32_t current_bpm = 0;  // 메트로놈 실행 상태 (0: 정지, 1: 작동)
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
void Beep(uint32_t ms);
void Display_Metronome_Status(uint32_t bpm, int beat);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
  SSD1306_Init();
  SSD1306_GotoXY (0,0);
  SSD1306_Puts ("-------------------", &Font_7x10, 1);
  SSD1306_GotoXY (39, 18);
  SSD1306_Puts ("WELCOME", &Font_7x10, 1);
  SSD1306_GotoXY (57, 30);
  SSD1306_Puts ("TO", &Font_7x10, 1);
  SSD1306_GotoXY (43, 42);
  SSD1306_Puts ("STempo", &Font_7x10, 1);
  SSD1306_GotoXY (0, 53);
  SSD1306_Puts ("------------------", &Font_7x10, 1);
  SSD1306_UpdateScreen();
  HAL_Delay (1000);
  SSD1306_ScrollRight(0,7);
  HAL_Delay(3000);
  SSD1306_ScrollLeft(0,7);
  HAL_Delay(3000);
  SSD1306_Stopscroll();
  SSD1306_Clear();


  SSD1306_GotoXY (25,20);
  SSD1306_Puts ("Are you", &Font_11x18, 1);
  SSD1306_GotoXY (31,40);
  SSD1306_Puts ("ready?", &Font_11x18, 1);
  SSD1306_UpdateScreen();
  HAL_Delay (100);
  SSD1306_Clear();

  SSD1306_GotoXY (7,27);
  SSD1306_Puts ("SHOW YOUR TEMPO!", &Font_7x10, 1);
  SSD1306_UpdateScreen();
  HAL_Delay (200);
  SSD1306_Clear();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  static uint32_t last_tap_time = 0;
  static uint32_t diff = 0;
  static int count = 0;
  uint32_t current_time = 0;

  static uint32_t avg_interval = 0;   // 평균 간격
  static uint32_t last_beep_time = 0; // 마지막으로 메트로놈 소리가 난 시간
  static uint8_t metronome_run = 0;   // 메트로놈 실행 유무 상태

  static uint32_t button_press_start_time = 0;
  static uint8_t long_press_triggered = 0;


  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  current_time = HAL_GetTick(); // 현재 시간

	  // 버튼이 처음 눌린 상황 (롱 프레스 포함)
	  if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_10) == GPIO_PIN_RESET)
	  {
		  // 버튼이 처음 눌린 상태인 경우
		  if (button_press_start_time == 0)
		  {
			  button_press_start_time = current_time;
			  long_press_triggered = 0;
		  }

		  // 버튼이 눌린 지 1초가 지났고, 롱 프레스 처리를 안 한 상태인 경우
		  if ((current_time - button_press_start_time >= 1000) && (long_press_triggered == 0))
		  {
			  // 초기화됨.
			  metronome_run = 0;
			  count = 0;
			  diff = 0;
			  avg_interval = 0;
			  last_tap_time = 0;
			  beat_count = -1;
			  current_bpm = 0;

			  long_press_triggered = 1; // 롱 프레스 중복 실행 방지

			  // 소리를 통해 멈춤을 알림.
			  Beep(300);

			  Display_Metronome_Status(0, -1);
		  }
	  }
	  else
	  {
		  // 버튼에서 손을 뗐을 떄
		  if (button_press_start_time > 0)
		  {
			  // 롱 프레스가 아닌 경우
			  if (long_press_triggered == 0)
			  {
				  // 첫 번째 버튼 입력 감지
				  if (count == 0)
				  {
					  metronome_run = 0;
					  last_tap_time = button_press_start_time;
					  count = 1;
					  Beep(50);
				  }
				  // 2번째, 3번째, 4번째 탭 누적
				  else if (count > 0 && count < 4)
				  {
					  diff += (button_press_start_time - last_tap_time);
					  last_tap_time = button_press_start_time;
					  count++;
					  Beep(50);
				  }

				  // 4번 다 눌렀을 때 BPM 최종 계산
				  if (count == 4)
				  {
					  avg_interval = diff / 3;
					  current_bpm = 60000 / avg_interval;
					  beat_count = 0;
					  last_beep_time = HAL_GetTick();
					  metronome_run = 1;


					  Display_Metronome_Status(current_bpm, beat_count);

					  count = 0;
					  diff = 0;
					  last_tap_time = 0;
				  }
			  }

			  // 손을 뗐으므로 버튼 관련 타이머 변수들 리셋
			  button_press_start_time = 0;
			  long_press_triggered = 0;
			  HAL_Delay(40);
		  }
	  }

	  //메트로놈 무한 반복 재생
	  if (metronome_run == 1 && avg_interval > 0)
	  {
		  if (HAL_GetTick() - last_beep_time >= avg_interval)
		  {
			  last_beep_time = HAL_GetTick();

			  beat_count = (beat_count + 1) % 4;
			  Display_Metronome_Status(current_bpm, beat_count);
			  Beep(32);
		  }
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
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
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

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
  htim2.Init.Prescaler = 83;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
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

  /*Configure GPIO pin : PC10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void Beep(uint32_t ms)
{
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 500);

    HAL_Delay(ms);

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
}

void Display_Metronome_Status(uint32_t bpm, int beat)
{
    // 박자 상태 문자열
    if (beat == 0)      strcpy(oled_beat_buf, "O  -  -  -");
    else if (beat == 1) strcpy(oled_beat_buf, "-  O  -  -");
    else if (beat == 2) strcpy(oled_beat_buf, "-  -  O  -");
    else if (beat == 3) strcpy(oled_beat_buf, "-  -  -  O");
    else                strcpy(oled_beat_buf, "-  -  -  -"); // 정지 상태 등

    // BPM 문자열
    if (bpm == 0) sprintf(oled_bpm_buf, "BPM: ---");
    else          sprintf(oled_bpm_buf, "BPM: %lu", bpm);

    // OLED 출력
    // SSD1306_Clear(); // 화면 전체 지우기

    SSD1306_GotoXY(7, 5);
    SSD1306_Puts("SHOW YOUR TEMPO!", &Font_7x10, 1);

    SSD1306_GotoXY(10, 22);
    SSD1306_Puts(oled_bpm_buf, &Font_11x18, 1);


    SSD1306_GotoXY(15, 45);
    SSD1306_Puts(oled_beat_buf, &Font_11x18, 1);

    SSD1306_UpdateScreen();
}
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
