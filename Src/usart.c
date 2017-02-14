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
#include "stm32469i_discovery.h"
#include "gpio.h"

/* USER CODE BEGIN 0 */
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#define LINE_SIZE (256)
#define BUFFER_COUNT 16
#define DMA_QUEUE 2048

#define RINGFIFO_SIZE (1024)              /* serial buffer in bytes (power 2)   */
#define RINGFIFO_MASK (RINGFIFO_SIZE-1ul) /* buffer size mask                   */

/* Buffer read / write macros                                                 */
#define RINGFIFO_RESET(FIFO)      do {(FIFO)->rdIdx = (FIFO)->wrIdx = 0;}while(0)
#define RINGFIFO_WR(FIFO, dataIn) do { (FIFO)->data[RINGFIFO_MASK & (FIFO)->wrIdx++] = (dataIn); }while(0)
#define RINGFIFO_RD(FIFO, dataOut)do { dataOut = (FIFO)->data[RINGFIFO_MASK & ++(FIFO)->rdIdx]; }while(0)
#define RINGFIFO_BACK(FIFO)        (FIFO)->data[RINGFIFO_MASK & ((FIFO)->wrIdx-1)]
#define RINGFIFO_FRONT(FIFO)        (FIFO)->data[RINGFIFO_MASK & (FIFO)->rdIdx]
#define RINGFIFO_EMPTY(FIFO)      ((FIFO)->rdIdx == (FIFO)->wrIdx)
#define RINGFIFO_FULL(FIFO)       ((RINGFIFO_MASK & (FIFO)->rdIdx) == (RINGFIFO_MASK & ((FIFO)->wrIdx+1)))
#define RINGFIFO_COUNT(FIFO)      (RINGFIFO_MASK & ((FIFO)->wrIdx - (FIFO)->rdIdx))

/* buffer type                                                                */
typedef struct __t_ring_fifo {
	struct __t_ring_fifo* next;
	__IO uint16_t wrIdx;
	__IO uint16_t rdIdx;

} t_ring_fifo;
typedef struct __line_buffer{
	struct __line_buffer* next;
	uint16_t length;
	uint8_t data[LINE_SIZE];
}t_line_buffer;
typedef struct __t_line_buffer_fifo {
	t_line_buffer* front;
	t_line_buffer* back;
}_t_line_buffer_fifo;

static bool uart_init = false;
static t_line_buffer buffers[BUFFER_COUNT];
static t_line_buffer* irq_lines[256];
static _t_line_buffer_fifo tx_fifo = { NULL, NULL };
static bool uart_trasmiting = false;
static uint8_t dma_buffer[DMA_QUEUE];
UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_usart3_tx;

#define IS_NEWLINECHAR(N) ((N)=='\r' || (N)=='\n')
//while(hdma_usart3_tx.Instance->NDTR>0);

void fifo_add_to_tx(uint8_t irq){
	__disable_irq(); // got to do this
	assert(irq_lines[irq]);
	if(tx_fifo.front==NULL)  tx_fifo.front=irq_lines[irq];
	else tx_fifo.back->next=irq_lines[irq];
	tx_fifo.back=irq_lines[irq];
	irq_lines[irq] = NULL;
	__enable_irq(); // got to do this
}
t_line_buffer* get_buffer(uint8_t irq){
	while(irq_lines[irq] == NULL) {
		__disable_irq(); // got to do this
		for(uint16_t i=0; i < BUFFER_COUNT; i++) {
			if(buffers[i].length ==0){
				t_line_buffer* ret = irq_lines[irq] = &buffers[i];
				__enable_irq(); // got to do this
				return ret;
			}
		}
		__enable_irq(); // got to do this
		HAL_Delay(1);
	}
	return irq_lines[irq];
}
void fifo_fill_dma_buffer() {
	if(!uart_init) return;

	//__disable_irq(); // got to do this, I hope this is fast
	if(!uart_trasmiting) {// return if the dma is running
	//	__disable_irq(); // got to do this

		if(tx_fifo.front){
			t_line_buffer* buf =tx_fifo.front;
			tx_fifo.front = buf->next;
			uart_trasmiting= true;
			uart_raw_print("fifo_fill_dma_buffer %s\r\n",buf->data);

			//HAL_UART_Transmit_IT(&huart3,buf->data, buf->length);
			HAL_UART_Transmit_DMA(&huart3,buf->data, buf->length);
			buf->length=0; // free the buffer
			buf->next = NULL;
		}
	//	__enable_irq(); // got to do this
	}
}
void fifo_putc(char c) {
	if(c == 0 || c == '\r') return; // ignore 0 and \r
	uint8_t irq = __get_IPSR() & 0xFF;
	t_line_buffer*  buf = get_buffer(irq);
	buf->data[buf->length++] = c;
	if(c == '\n' || buf->length == LINE_SIZE) fifo_add_to_tx(irq);
}

void fifo_write(const uint8_t* data, size_t length) {
	uint8_t irq = __get_IPSR() & 0xFF;
	while(length){
		t_line_buffer*  buf = get_buffer(irq);
		while(length && buf->length < LINE_SIZE) {
			uint8_t c = *data++;
			if(c == 0 || c == '\r') continue;
			buf->data[buf->length++] = c;
			if(c == 0 || c == '\n') {
				buf->data[buf->length++] = '\r';
				buf->data[buf->length++] = 0;
				fifo_add_to_tx(irq);
				break;
			}
			length--;
		}
	}
}
#define USARTx_IT_IRQ	  		  USART3_IRQn
#define USARTx_IT__IRQHandler     USART3_IRQHandler
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	(void)huart;
	BSP_LED_On(LED_BLUE);
	uart_trasmiting = false;

	//fifo_fill_dma_buffer();
}
void USART3_IRQHandler(){ // { USARTx_IT__IRQHandler(){
	BSP_LED_On(LED_RED);
	HAL_UART_IRQHandler(&huart3);
}



UART_HandleTypeDef *s_chuart;
void uart_set_console_out(UART_HandleTypeDef* huart){
	s_chuart = huart;
	memset(irq_lines,0,sizeof(t_line_buffer*)*256);
	memset(buffers,0,sizeof(t_line_buffer)*BUFFER_COUNT);
}




void uart_putraw(char c) {
	if(!uart_init) return;
	while(!(__HAL_UART_GET_FLAG(s_chuart, UART_FLAG_TXE))); // trasmit empty
	s_chuart->Instance->DR = c;
}
void direct_uart_puts(const char* str) {
	if(!uart_init) return;
	while(*str)uart_putraw(*str++);
	while(!(__HAL_UART_GET_FLAG(s_chuart, UART_FLAG_TC))); // trasmit complete
}






void MX_USART3_UART_Init(void)
{
	uart_set_console_out(&huart3);

  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  assert(HAL_UART_Init(&huart3) == HAL_OK);

  HAL_NVIC_SetPriority(USARTx_IT_IRQ, 5, 0);
  HAL_NVIC_EnableIRQ(USARTx_IT_IRQ);
  uart_init = true;
}

void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(huart->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspInit 0 */

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
    /* DMA interrupt init */
     /* DMA1_Stream3_IRQn interrupt configuration */
     HAL_NVIC_SetPriority(USART3_IRQn, 5, 0);
     HAL_NVIC_EnableIRQ(USART3_IRQn);
    /* Peripheral DMA init*/
     //	RINGFIFO_RESET(gUartFifo);
	/* DMA controller clock enable */
	__HAL_RCC_DMA1_CLK_ENABLE();

	/* DMA interrupt init */
	/* DMA1_Stream3_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);

      hdma_usart3_tx.Instance = DMA1_Stream3;
      hdma_usart3_tx.Init.Channel = DMA_CHANNEL_4;
      hdma_usart3_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
      hdma_usart3_tx.Init.PeriphInc = DMA_PINC_DISABLE;
      hdma_usart3_tx.Init.MemInc = DMA_MINC_ENABLE;
      hdma_usart3_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
      hdma_usart3_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
      hdma_usart3_tx.Init.Mode = DMA_NORMAL;
      hdma_usart3_tx.Init.Priority = DMA_PRIORITY_LOW;
      hdma_usart3_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
      assert(HAL_DMA_Init(&hdma_usart3_tx) == HAL_OK);
      __HAL_LINKDMA(huart,hdmatx,hdma_usart3_tx);
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{

  if(huart->Instance==USART3)
  {
    __HAL_RCC_USART3_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOB, STLK_RX_Pin|STLK_TX_Pin);
    HAL_DMA_DeInit(huart->hdmatx);
  }
} 

bool s_newline_enable = true;
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
	if(!uart_init) return;
	_uart_putc(c);
	 while(!(__HAL_UART_GET_FLAG(s_chuart, UART_FLAG_TC))); // trasmit complete
}

void uart_puts(const char* str){
	size_t len = strlen(str);
	uart_write((const uint8_t*)str,len);
}

void uart_write(const uint8_t* data, size_t len){
	 // fifo_putc(*data++);
	  //while(len--) _uart_putc(*data++);
	fifo_write(data,len);
//	  while(len--) fifo_putc(*data++);
	//fifo_fill_dma_buffer();
#if 0
	//while(uart_trasmitting);
	//uart_trasmitting = true;
	//BSP_LED_On(LED_BLUE);
	//HAL_UART_Transmit_DMA(&huart3,(uint8_t*)data, len);

	while(buffer_level > 0){
		uart_send_buffer(&uart_buffer[buffer_level-1]);
		buffer_level--;
	}
__uart_write(data,len);
	//while(len--) _uart_putc(*data++);
	//while(!(__HAL_UART_GET_FLAG(s_chuart, UART_FLAG_TC))); // trasmit complete
#endif
}

void uart_puti(unsigned long value, int width, t_uart_mode mode) {
	char buf[65];
	itoa(value,buf,10);
	uart_puts(buf);
	//char* buffer = uart_buffr +
	//void uart_puti(unsigned long value, int width, t_uart_mode mode);
}

void uart_print(const char* fmt,...){
	char buf[128];
	va_list va;
	va_start(va,fmt);
	size_t len = vsprintf(buf,fmt,va);
	va_end(va);
	buf[127]=0;
	uart_write((uint8_t*)buf,len);
	//uart_puts(buf);
}
void uart_raw_write(const uint8_t* data, size_t len){
	while(len--) uart_putc(*data++);
}
void uart_raw_print(const char* fmt,...){
	char buf[128];
	va_list va;
	va_start(va,fmt);
	size_t len = vsprintf(buf,fmt,va);
	va_end(va);
	buf[127]=0;
	uart_raw_write((uint8_t*)buf,len);
}

/* USER CODE END 1 */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
