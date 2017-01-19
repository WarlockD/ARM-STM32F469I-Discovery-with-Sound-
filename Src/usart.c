/**
  ******************************************************************************
  * File Name          : USART.c
  * Description        : This file provides code for the configuration
  *                      of the USART instances.
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
#include "usart.h"

#include "gpio.h"

/* USER CODE BEGIN 0 */
#include <string.h>
#include <stdarg.h>
static char uart_buffer[256];
static UART_HandleTypeDef* s_chuart = NULL;
void uart_set_console_out(UART_HandleTypeDef* huart){
	s_chuart = huart;
}

/* USER CODE END 0 */

UART_HandleTypeDef huart3;

/* USART3 init function */

void MX_USART3_UART_Init(void)
{

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

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(uartHandle->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspInit 0 */
	  uart_set_console_out(&huart3);
  /* USER CODE END USART3_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();
  
    /**USART3 GPIO Configuration    
    PB10     ------> USART3_TX
    PB11     ------> USART3_RX 
    */
    GPIO_InitStruct.Pin = STLK_RX_Pin|STLK_TX_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN USART3_MspInit 1 */

  /* USER CODE END USART3_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspDeInit 0 */

  /* USER CODE END USART3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART3_CLK_DISABLE();
  
    /**USART3 GPIO Configuration    
    PB10     ------> USART3_TX
    PB11     ------> USART3_RX 
    */
    HAL_GPIO_DeInit(GPIOB, STLK_RX_Pin|STLK_TX_Pin);

  }
  /* USER CODE BEGIN USART3_MspDeInit 1 */

  /* USER CODE END USART3_MspDeInit 1 */
} 

/* USER CODE BEGIN 1 */
volatile bool s_newline_enable = true;
void uart_return_after_newline(bool enable) {
	s_newline_enable = enable;
}
void ManualPrintNumberComma(int32_t a) {
	if(a<0) { push_char('-'); a*=-1; }
	long c = 1;
	while((c*=1000)<a);
	while(c>1)
	{
	   int t = (a%c)/(c/1000);
	   if(!(((c>a)||(t>99)))) {
		   push_char('0');
		   if(t < 10) push_char('0');
	   }
	   writeout_number(t);
	   c/=1000;
	   if(!(c == 1)) push_char(',');
	}
}
#define IS_NEWLINECHAR(N) ((N)=='\r' || (N)=='\n')

void uart_putraw(char c) {
	while(!(__HAL_UART_GET_FLAG(s_chuart, UART_FLAG_TXE))); // trasmit empty
	s_chuart->Instance->DR = c;
}
void _uart_putc(char c){
	static char prev = 0;
	if(s_newline_enable) {
		//if(!prev && IS_NEWLINECHAR(c)) prev = c;
		if(prev == '\n' && c != '\r') uart_putraw('\r');
		else if(prev == '\r' && c != '\n') uart_putraw('\n');
		prev = c;
	}
	uart_putraw(c);
}

void uart_putc(char c){
	_uart_putc(c);
	 while(!(__HAL_UART_GET_FLAG(s_chuart, UART_FLAG_TC))); // trasmit complete
}

void uart_puts(const char* str){
	while(*str) _uart_putc(*str++);
	while(!(__HAL_UART_GET_FLAG(s_chuart, UART_FLAG_TC))); // trasmit complete
}
void uart_write(const uint8_t* data, size_t len){
	while(len--) _uart_putc(*data++);
	while(!(__HAL_UART_GET_FLAG(s_chuart, UART_FLAG_TC))); // trasmit complete
}

void uart_puti(unsigned long value, int width, t_uart_mode mode) {
	itoa(value,uart_buffer,10);
	uart_puts(uart_buffer);
	//char* buffer = uart_buffr +
	//void uart_puti(unsigned long value, int width, t_uart_mode mode);
}
void uart_print(const char* fmt,...){
	char buf[128];
	va_list va;
	va_start(va,fmt);
	vsprintf(buf,fmt,va);
	va_end(va);
	buf[127]=0;
	uart_puts(buf);
}

/* USER CODE END 1 */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
