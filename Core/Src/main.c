/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "DS3231.h"

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

TIM_HandleTypeDef htim2; 
I2C_HandleTypeDef hi2c1;

//NÚT BAM
#define BTN_UP    GPIO_PIN_12   
#define BTN_DOWN  GPIO_PIN_13   
#define BTN_MODE  GPIO_PIN_14   
#define BTN_SAVE  GPIO_PIN_15   

//LED ---
#define portb   GPIOB
#define ck      GPIO_PIN_1
#define en      GPIO_PIN_10
#define dat     GPIO_PIN_11

// Ma LED (Index 10 là ma tat 0xFF)
unsigned char code7seg[11] = {0x28, 0xeb, 0x32, 0xa2, 0xe1, 0xa4, 0x24, 0xea, 0x20, 0xa0, 0xFF};
unsigned char led[]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};	

volatile int led_index = 0; 
volatile int DisplayBuffer[6] = {0, 0, 0, 0, 0, 0}; 

// Bien thoi gian
uint8_t ds_sec, ds_min, ds_hour;
uint8_t ds_day, ds_date, ds_month, ds_year;

// Bien chinh gio
int che_do = 0; // 0: Xem, 1: Gio, 2: Phut
int temp_gio = 0;
int temp_phut = 0;

//	Flag danh dau: 0=Chua chinh gì, 1=Ðã bam nút Tang/Gi?m
int da_thay_doi = 0; 

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_I2C1_Init(void);

/* USER CODE BEGIN 0 */
// --- HAM GIAO TIEP 595 ---
void xuat_595(unsigned int value)
{
	int i;
	for(i=0;i<8;i++)
	{
		if((value & 0x80) == 0x80) HAL_GPIO_WritePin(portb, dat, GPIO_PIN_SET);
		else                       HAL_GPIO_WritePin(portb, dat, GPIO_PIN_RESET);
		
		HAL_GPIO_WritePin(portb, ck, GPIO_PIN_SET);			
		HAL_GPIO_WritePin(portb, ck, GPIO_PIN_RESET);			
		value = value << 1;
	}
}

void display(unsigned int led_name, unsigned int led_data)
{
	xuat_595(led_data);
	xuat_595(led_name);
	HAL_GPIO_WritePin(portb, en, GPIO_PIN_SET);			
	HAL_GPIO_WritePin(portb, en, GPIO_PIN_RESET);			
}

// --- HAM TACH SO ---
void CapNhatBufferThoiGian(int h, int m, int s)
{
    DisplayBuffer[0] = h / 10; DisplayBuffer[1] = h % 10;
    DisplayBuffer[2] = m / 10; DisplayBuffer[3] = m % 10;
    DisplayBuffer[4] = s / 10; DisplayBuffer[5] = s % 10;
}

// --- NGAT TIMER QUÉT LED ---
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == TIM2)
	{
		int so_can_hien = DisplayBuffer[led_index];
		if(so_can_hien > 10) so_can_hien = 10; 
		display(led[led_index], code7seg[so_can_hien]);

		led_index++;
		if(led_index > 5) led_index = 0;
	}
}

// HAM DOC NUT
int KiemTraNut(uint16_t GPIO_Pin)
{
    if(HAL_GPIO_ReadPin(GPIOB, GPIO_Pin) == GPIO_PIN_RESET) 
    {
        HAL_Delay(20); 
        if(HAL_GPIO_ReadPin(GPIOB, GPIO_Pin) == GPIO_PIN_RESET) return 1; 
    }
    return 0; 
}

// HAM LUU THOI GIAN THONG MINH
void LuuThoiGian(void)
{
    // Doc lai thoi gian thuc
    Get_Time(&ds_sec, &ds_min, &ds_hour, &ds_day, &ds_date, &ds_month, &ds_year);
    
    if(da_thay_doi == 1)
    {
        // CO CHINH SUA -> Reset giây ve 00
        Set_Time(0, temp_phut, temp_gio, ds_day, ds_date, ds_month, ds_year);
    }
    else
    {
        // KHONG CHINH SUA -> Luu lai giây dang chay
        Set_Time(ds_sec, temp_phut, temp_gio, ds_day, ds_date, ds_month, ds_year);
    }
}
/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_I2C1_Init();

  HAL_TIM_Base_Start_IT(&htim2); 

  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE BEGIN 3 */

    //DOC THOI GIAN
    static uint32_t last_read = 0;
    if(HAL_GetTick() - last_read > 200) 
    {
        Get_Time(&ds_sec, &ds_min, &ds_hour, &ds_day, &ds_date, &ds_month, &ds_year);
        last_read = HAL_GetTick();
    }

    //XU LY CHE DO (MODE)
    if(KiemTraNut(BTN_MODE)) 
    {
        che_do++;
        
        if(che_do == 1) // Vao chinh Gio
        {
            temp_gio = ds_hour;
            temp_phut = ds_min;
            da_thay_doi = 0; 
        }
        else if(che_do > 2) // Thoát
        {
            che_do = 0;
            LuuThoiGian(); 
        }
        
        while(HAL_GPIO_ReadPin(GPIOB, BTN_MODE) == GPIO_PIN_RESET);
    }

    //XU LY SAVE
    if(KiemTraNut(BTN_SAVE) && che_do != 0)
    {
         che_do = 0; 
         LuuThoiGian(); 
         while(HAL_GPIO_ReadPin(GPIOB, BTN_SAVE) == GPIO_PIN_RESET); 
    }

    //XU LY TANG/GIAM (DA FORMAT LAI)
    if(che_do == 1) // Chinh Gio
    {
        if(KiemTraNut(BTN_UP))
        {
            temp_gio++;
            if(temp_gio > 23) temp_gio = 0;
            HAL_Delay(200);
            da_thay_doi = 1;
        }

        if(KiemTraNut(BTN_DOWN))
        {
            temp_gio--;
            if(temp_gio < 0) temp_gio = 23;
            HAL_Delay(200);
            da_thay_doi = 1;
        }
    }
    else if(che_do == 2) // Chinh Phut
    {
        if(KiemTraNut(BTN_UP))
        {
            temp_phut++;
            if(temp_phut > 59) temp_phut = 0;
            HAL_Delay(200);
            da_thay_doi = 1;
        }

        if(KiemTraNut(BTN_DOWN))
        {
            temp_phut--;
            if(temp_phut < 0) temp_phut = 59;
            HAL_Delay(200);
            da_thay_doi = 1;
        }
    }

    //HIEN THI (DA FORMAT LAI)
    if(che_do == 0) // Xem gio
    {
        CapNhatBufferThoiGian(ds_hour, ds_min, ds_sec);
    }
    else // Che do chinh
    {
        // Nhap nháy 0.25s
        int trang_thai_tat = 0;
        if((HAL_GetTick() % 500) < 250)
        {
            trang_thai_tat = 1;
        } 
        
        // Hien thi Gio
        if(che_do == 1 && trang_thai_tat) 
        { 
            DisplayBuffer[0] = 10; 
            DisplayBuffer[1] = 10; 
        }
        else 
        { 
            DisplayBuffer[0] = temp_gio / 10; 
            DisplayBuffer[1] = temp_gio % 10; 
        }

        // Hien thi Phút
        if(che_do == 2 && trang_thai_tat) 
        { 
            DisplayBuffer[2] = 10; 
            DisplayBuffer[3] = 10; 
        }
        else 
        { 
            DisplayBuffer[2] = temp_phut / 10; 
            DisplayBuffer[3] = temp_phut % 10; 
        }

        // Tat Giây khi ch?nh
        DisplayBuffer[4] = 10; 
        DisplayBuffer[5] = 10; 
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
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
  hi2c1.Init.ClockSpeed = 100000;
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

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 71;
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
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1|GPIO_PIN_10|GPIO_PIN_11, GPIO_PIN_RESET);

  /*Configure GPIO pins : PB1 PB10 PB11 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_10|GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB12 PB13 PB14 PB15 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
