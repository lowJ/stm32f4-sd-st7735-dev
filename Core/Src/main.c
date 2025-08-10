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
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "SEGGER_RTT.h"
#include "st7735.h"
#include "fonts.h"
#include "testimg.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define CYCLE_MAX_NUM_FILES 10
#define CYCLE_MAX_FILE_NAME 255

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SD_HandleTypeDef hsd;

SPI_HandleTypeDef hspi4;
DMA_HandleTypeDef hdma_spi4_tx;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SDIO_SD_Init(void);
static void MX_SPI4_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t BSP_SD_IsDetected(void)
{
  return SD_PRESENT;
}

volatile bool drawing_in_progress = false;
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  drawing_in_progress = false;

#define ST7735_CS_Pin        GPIO_PIN_12
#define ST7735_CS_GPIO_Port  GPIOB
  HAL_GPIO_WritePin(ST7735_CS_GPIO_Port, ST7735_CS_Pin, GPIO_PIN_SET);
}

bool isButtonPressed()
{
  /* active low, pulled up */
  if (HAL_GPIO_ReadPin( GPIOA, GPIO_PIN_0 ) == GPIO_PIN_SET )
  {
    return false;

  }
  else
  {
    return true;
  }
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
  MX_DMA_Init();
  MX_SDIO_SD_Init();
  MX_FATFS_Init();
  MX_SPI4_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  SEGGER_RTT_ConfigUpBuffer(0, NULL, NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
  SEGGER_RTT_ConfigUpBuffer(1, NULL, NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
  SEGGER_RTT_ConfigUpBuffer(2, NULL, NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_SKIP);

  SEGGER_RTT_printf(0, "SD INFO\r\n");
  SEGGER_RTT_printf(0, "Block size %lu\r\n", hsd.SdCard.BlockSize);
  SEGGER_RTT_printf(0, "Block num %lu\r\n", hsd.SdCard.BlockNbr);
  SEGGER_RTT_printf(0, "Card size %lu GB\r\n", ((uint64_t)hsd.SdCard.BlockSize * (uint64_t)hsd.SdCard.BlockNbr) / 1000000000 );

  uint32_t last_toggle_ms = HAL_GetTick();
  uint32_t blink_duration_ms = 1000;
  
  if( f_mount(&SDFatFS, (TCHAR const*) SDPath, 0) != FR_OK ) {
    SEGGER_RTT_printf(0, "Could not mount disk\r\n");
    Error_Handler();
  }
  else
  {
    SEGGER_RTT_printf(0, "FS Mount SD!\r\n");
  }

  uint64_t bytes_read = 0;
  uint64_t last_bytes_read = 0;
  uint32_t last_total_frames = 0;
  uint32_t total_frames = 0;

  /* 4 byte aligned for DMA */
  /* alternate*/
  uint8_t buf_swap = 0;
  uint8_t buf[2][50000] __attribute__((aligned(4)));

  ST7735_Init();
  bool buttonPressedEvent = false;
  FRESULT res;

  /* get list of videos */
  const char* cycle_path = "cycle";
  char cycle_files[CYCLE_MAX_NUM_FILES][CYCLE_MAX_FILE_NAME] = { 0 };
  size_t cycle_files_num_files = 0;
  size_t cycle_files_current_file = 0;

    DIR dir;
 
 
    res = f_opendir(&dir, cycle_path);

    if (res == FR_OK)
    {
      int n = 0;
      while( 1 )
      {
          FILINFO fno;
          res = f_readdir(&dir, &fno);
 
          /* exit if we listed all the files */
          if ((res != FR_OK) || (fno.fname[0] == 0))
            break;
          
          if ( n >= CYCLE_MAX_NUM_FILES )
          {
            SEGGER_RTT_printf(0 , "Warning: could not process all files in cycle folder\r\n");
            break;
          }
        

 
          SEGGER_RTT_printf( 0,  "%s\r\n", fno.fname);
          strncpy( &cycle_files[n], fno.fname, CYCLE_MAX_FILE_NAME);
          cycle_files_num_files++;
          n++;
      }
    }

  /* filename + buffer for path */
  char filename[CYCLE_MAX_FILE_NAME + 50];
  memset( filename, 0, sizeof(filename) );
  strcpy(filename, cycle_path);
  strcat(filename, "/");
  strcat(filename, &cycle_files[cycle_files_current_file]);
  res = f_open(&SDFile,filename, FA_OPEN_EXISTING | FA_READ );
  if(res == FR_OK)
  {
    SEGGER_RTT_printf(0, "File opened");
  }
  else
  {
    SEGGER_RTT_printf(0, "could not open file %d", res);
    Error_Handler();
  }



  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint32_t time_ms = HAL_GetTick();

  /* detect press event */
  static bool prevButtonState;
  bool currentButtonState = isButtonPressed();
  if(prevButtonState == true && currentButtonState == false )
  {
    buttonPressedEvent = true;
    SEGGER_RTT_printf(0, "buttonPressed");
  }
  prevButtonState = currentButtonState;

  /* service button press event */
  if(buttonPressedEvent )
  {
    /* clear event */
    buttonPressedEvent = false;
    f_close( &SDFile ); /* close file */

    /* get new file name */
    /* if file already open, close it, then open new file */

    cycle_files_current_file = (cycle_files_current_file + 1) % cycle_files_num_files;
    memset( filename, 0, sizeof(filename) );
    strcpy(filename, cycle_path);
    strcat(filename, "/");
    strcat(filename, &cycle_files[cycle_files_current_file]);
    res = f_open(&SDFile,filename, FA_OPEN_EXISTING | FA_READ );
  }

    
    UINT br;
    //uint32_t start_ms = HAL_GetTick();  
    res = f_read( &SDFile, buf[buf_swap], 2*128*160, &br);
    //SEGGER_RTT_printf(0, "s%dms\r\n", HAL_GetTick() - start_ms );
    // sd read is about 22-23ms
    if(res == FR_OK)
    {
      if(br != 2*128*160)
      {
        SEGGER_RTT_printf(0, "EOF? Read %dKB", bytes_read / 1000 );
        f_close( &SDFile ); /* close file */

        FRESULT res = f_open(&SDFile,filename, FA_OPEN_EXISTING | FA_READ );
        if(res == FR_OK)
        {
          SEGGER_RTT_printf(0, "File opened");
        }
        else
        {
          SEGGER_RTT_printf(0, "could not open file %d", res);
          Error_Handler();
        }
        //Error_Handler();
      }
      bytes_read += br;
    }
    else
    {
      SEGGER_RTT_printf(0, "error occured reading file%d\r\n", res);
      Error_Handler();
    }

  /* make sure */
  while( drawing_in_progress );

  //start_ms = HAL_GetTick();
  drawing_in_progress = true;
  ST7735_DrawImage(0, 0, 160, 128, (uint16_t*)(buf[buf_swap]));
  //SEGGER_RTT_printf(0, "f%dms\r\n", HAL_GetTick() - start_ms );
  // frame render is 20ms
  buf_swap = (buf_swap + 1) % 2;
  total_frames++;

    /* 1s task */
    if( ( time_ms - last_toggle_ms) >= blink_duration_ms )
    {
	    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
	    //SEGGER_RTT_printf(0, "HELLO WORLD %d\r\n", time_ms);
      SEGGER_RTT_printf(0, "%uFPS\r\n", total_frames - last_total_frames);
      SEGGER_RTT_printf(0, "Read %uKBps\r\n", (bytes_read - last_bytes_read)/1000);
      last_toggle_ms = time_ms;
      last_bytes_read = bytes_read;
      last_total_frames = total_frames;
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 5;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SDIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDIO_SD_Init(void)
{

  /* USER CODE BEGIN SDIO_Init 0 */

  /* USER CODE END SDIO_Init 0 */

  /* USER CODE BEGIN SDIO_Init 1 */

  /* USER CODE END SDIO_Init 1 */
  hsd.Instance = SDIO;
  hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
  hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
  hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
  hsd.Init.BusWide = SDIO_BUS_WIDE_4B;
  hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd.Init.ClockDiv = 8;
  /* USER CODE BEGIN SDIO_Init 2 */
  hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
  if (HAL_SD_Init(&hsd) != HAL_OK ){
    Error_Handler();
  }

  if ( HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_4B)!= HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE END SDIO_Init 2 */

}

/**
  * @brief SPI4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI4_Init(void)
{

  /* USER CODE BEGIN SPI4_Init 0 */

  /* USER CODE END SPI4_Init 0 */

  /* USER CODE BEGIN SPI4_Init 1 */

  /* USER CODE END SPI4_Init 1 */
  /* SPI4 parameter configuration*/
  hspi4.Instance = SPI4;
  hspi4.Init.Mode = SPI_MODE_MASTER;
  hspi4.Init.Direction = SPI_DIRECTION_2LINES;
  hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi4.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi4.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi4.Init.NSS = SPI_NSS_SOFT;
  hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi4.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi4.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi4.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi4.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI4_Init 2 */

  /* USER CODE END SPI4_Init 2 */

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
  huart2.Init.Mode = UART_MODE_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(led_GPIO_Port, led_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, tft_rs_Pin|tft_rst_Pin|tft_cs_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : led_Pin */
  GPIO_InitStruct.Pin = led_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(led_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : button_Pin */
  GPIO_InitStruct.Pin = button_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(button_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : tft_rs_Pin tft_rst_Pin tft_cs_Pin */
  GPIO_InitStruct.Pin = tft_rs_Pin|tft_rst_Pin|tft_cs_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : buttonB6_Pin */
  GPIO_InitStruct.Pin = buttonB6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(buttonB6_GPIO_Port, &GPIO_InitStruct);

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
