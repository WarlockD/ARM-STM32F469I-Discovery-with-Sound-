/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  *
  * Copyright (c) 2016 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_hal.h"
#include "dma2d.h"
#include "dsihost.h"
#include "fatfs.h"
#include "ltdc.h"
#include "sdio.h"
#include "usart.h"
#include "gpio.h"
#include "fmc.h"

/* USER CODE BEGIN Includes */
#include "stm32469i_discovery.h"
#include "stm32469i_discovery_sdram.h"
#include "stm32469i_discovery_lcd.h"
#include "stm32469i_discovery_sd.h"
#include "stm32469i_discovery_ts.h"
#include "stm32469i_discovery_audio.h"
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "usb_host.h"
#include "stm32469i_discovery.h"
#include "stm32469i_discovery_lcd.h"


/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void Error_Handler(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
const char booting_up[] = "We are booting up!\r\n";
void q_putstr(const char* str) {
	HAL_UART_Transmit(&huart3, (uint8_t*)str,(uint16_t)strlen(str),100);
}
uint32_t* layer0_address = NULL;

FATFS sd_fat;

void MountSD() {
	if(f_mount(&sd_fat,"0:",0)== FR_OK)
		printf("SD Mounted!\r\n");
	else
		printf("SD NotMouted!\r\n");
}
//void  D_DoomMain();
void D_DoomMain (int argc, char** argv);



/* Private function prototypes -----------------------------------------------*/
//static void SystemClock_Config(void);



void other_things() {
	//USBH_Process(&hUSBHost);
}

void Error_Handler(void);
void refreshUSB() {
    /* USB Host Background task */
    //USBH_Process(&hUSBHost);
}
void USB_HOST_Init();
void SetupUSB() {
	USB_HOST_Init();
	//  USBH_Init(NULL);
	  printf("SetupUSB Finished\r\n");

}
void testusb() {
	uint32_t start = HAL_GetTick()+1000;
	while(1){
		uint32_t tick = HAL_GetTick();
		if(tick > start){
			start +=1000;
		//	USBH_DbgLog(" TICK!");
		}

		USBH_Poll() ;
	}
}
void SystemTimerConfig();
/* USER CODE END 0 */

int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */


  /* Configure the system clock */
  SystemClock_Config(); //  SystemTimerConfig(); handled here
  HAL_Init();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  BSP_PB_Init(BUTTON_USER,BUTTON_MODE_GPIO);
  BSP_LED_Init(LED_RED);
  BSP_LED_Init(LED_BLUE);
  BSP_LED_Init(LED_ORANGE);
  BSP_LED_Init(LED_GREEN);
  BSP_LED_Off(LED_RED);
  BSP_LED_Off(LED_BLUE);
  BSP_LED_Off(LED_ORANGE);
  BSP_LED_Off(LED_GREEN);

  BSP_SD_Init();
  MX_USART3_UART_Init();
  MX_FATFS_Init();
 // MX_DSIHOST_DSI_Init();

  /* USER CODE BEGIN 2 */
 // q_putstr("RawUART is setup"\r\n");
 // BSP_SD_Init();
  BSP_SDRAM_Init();
 // layer0_address = malloc(800 * 480 * sizeof(uint32_t) * 2);
  BSP_LCD_Init();

  // clear the debug terminal
  uart_print("\033[2J\033[;H");
  DbgUsr("Starting LCD/TS Init");
   // OnError_Handler(lcd_status != LCD_OK);

    BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
    BSP_LCD_SelectLayer(0);
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    BSP_TS_Init(800,480);
    DbgUsr("Starting USB Init");
    SetupUSB();
   testusb() ;
   DbgUsr("Starting SD Init");
    printf("Then printf is setup\r\n");
   // test_usb_loop();
    // ok lets do audio!
    /* Try to Init Audio interface in diffrent config in case of failure */
 //  BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_AUTO, 299, I2S_AUDIOFREQ_48K);
//   BSP_AUDIO_OUT_SetAudioFrameSlot(CODEC_AUDIOFRAME_SLOT_02);

   // LCD_BriefDisplay();


 MountSD();
 //test_it();
 char* demo_test[] = { "-playdemo", "09EXIT" };
 D_DoomMain (3, demo_test);
 //D_DoomMain (0, NULL);
 // if((status=f_open(&fp,"0:test.txt",FA_WRITE|FA_CREATE_NEW)) == FR_OK){
//	  printf("Yea! File open for writing!\r\n");

 // } else {
//	  printf("Booo cannot open file: %i \r\n",status);
 // }
 printf("Exit of D_Doommain()\r\n");
 // MemoryTest();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */

}


/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 180000000
  *            HCLK(Hz)                       = 180000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 8
  *            PLL_N                          = 360
  *            PLL_P                          = 2
  *            PLL_Q                          = 7
  *            PLL_R                          = 2
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  *         The USB clock configuration from PLLSAI:
  *            PLLSAIM                        = 8
  *            PLLSAIN                        = 384
  *            PLLSAIP                        = 8
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();

  /* The voltage scaling allows optimizing the power consumption when the device is
     clocked below the maximum system frequency, to update the voltage scaling value
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
#if defined(USE_STM32469I_DISCO_REVA)
  RCC_OscInitStruct.PLL.PLLM = 25;
#else
  RCC_OscInitStruct.PLL.PLLM = 8;
#endif /* USE_STM32469I_DISCO_REVA */
  RCC_OscInitStruct.PLL.PLLN = 360;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  RCC_OscInitStruct.PLL.PLLR = 2;

  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /* Enable the OverDrive to reach the 180 Mhz Frequency */
  if(HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /* Select PLLSAI output as USB clock source */
  PeriphClkInitStruct.PLLSAI.PLLSAIQ = 7;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 384;
  PeriphClkInitStruct.PLLSAI.PLLSAIP = RCC_PLLSAIP_DIV8;
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_CK48;
  PeriphClkInitStruct.Clk48ClockSelection = RCC_CK48CLKSOURCE_PLLSAIP;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
  /**Configure the Systick interrupt time
  */
HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

  /**Configure the Systick
  */
HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

/* SysTick_IRQn interrupt configuration */
HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
}
/** System Clock Configuration
*/
void SystemClock_Config_Old(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

    /**Configure the main internal regulator output voltage 
    */
  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 360;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

    /**Activate the Over-Drive mode 
    */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

    /**Initializes the CPU, AHB and APB busses clocks 
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

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_SDIO
                              |RCC_PERIPHCLK_CLK48;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
  PeriphClkInitStruct.PLLSAI.PLLSAIR = 2;
  PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_2;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48CLKSOURCE_PLLQ;
  PeriphClkInitStruct.SdioClockSelection = RCC_SDIOCLKSOURCE_CLK48;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}


void DebugPrintBufferByte(uint8_t* data, size_t length) {
		while(length){
			if(length>8) {
				printf("%2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X \r\n",
						data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);
				length-=8;
				data+=8;
			} else {
				 while(length--) printf("%2.2X ", *data++);
				 printf("\r\n");
				 break;
			}
		}
}

/* USER CODE BEGIN 4 */
#if 0
static void LCD_BriefDisplay(void)
{
  BSP_LCD_Clear(LCD_COLOR_WHITE);
  BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
  BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
  BSP_LCD_FillRect(0, 0, 800, 112);
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_DisplayStringAtLine(1, (uint8_t *)"          LCD_DSI_VideoMode_SingleBuffer");
  BSP_LCD_SetFont(&Font16);
  BSP_LCD_DisplayStringAtLine(4, (uint8_t *)"This example shows how to display images on LCD DSI using same buffer");
  BSP_LCD_DisplayStringAtLine(5, (uint8_t *)"for display and for draw     ");

}
#endif
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler */
  /* User can add his own implementation to report the HAL error return state */
	while(1) {

			BSP_LED_Toggle(LED_RED);
			HAL_Delay(500);

		}
  /* USER CODE END Error_Handler */ 
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void __assert_func(const char * file, int line, const char * func, const char * value){
	uart_print("ASSERT(%s:%i): %s()  %s\r\n",file,line,func,value);
	while(1) {

		BSP_LED_Toggle(LED_RED);
		HAL_Delay(500);

	}
}
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
	uart_print("Wrong parameters value: file %s on line %lu\r\n", file, line);
  /* USER CODE END 6 */
	while(1) {

			BSP_LED_Toggle(LED_RED);
			HAL_Delay(500);

		}
}

#endif

/**
  * @}
  */ 

/**
  * @}
*/ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
