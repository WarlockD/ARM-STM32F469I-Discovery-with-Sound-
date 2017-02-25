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
#include <queue.h>
#include "usart.h"
#include "stm32469i_discovery.h"
#include "gpio.h"

#if 0
/* USER CODE BEGIN 0 */
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

#define LINE_SIZE (256)
#define BUFFER_COUNT 16
#define DMA_BUFFER 2048

#if 0
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
	SIMPLEQ_ENTRY(__t_ring_fifo) tx_fifo;
	__IO uint16_t wrIdx;
	__IO uint16_t rdIdx;
	uint8_t data[RINGFIFO_SIZE];
} t_ring_fifo;
#endif

// line buffer
#define FREE_BUFFER -1
typedef struct __t_linebuffer {
	SIMPLEQ_ENTRY(__t_linebuffer) tx_fifo;
	uint32_t length;
	uint32_t lock;
	char data[LINE_SIZE];
} t_linebuffer;

static bool uart_init = false;
static t_linebuffer buffers[BUFFER_COUNT];
static char dma_buffer[DMA_BUFFER];
static t_linebuffer* irq_lines[256];
SIMPLEQ_HEAD(t_linebuffer_queue, __t_linebuffer) tx_fifo = SIMPLEQ_HEAD_INITIALIZER(tx_fifo);

static bool uart_trasmiting = false;

#define IS_NEWLINECHAR(N) ((N)=='\r' || (N)=='\n')
//while(hdma_usart3_tx.Instance->NDTR>0);
UART_HandleTypeDef *s_chuart;
void uart_set_console_out(UART_HandleTypeDef* huart){
	s_chuart = huart;
	SIMPLEQ_INIT(&tx_fifo);
	memset(irq_lines,0,sizeof(t_linebuffer*)*256);
	memset(buffers,0,sizeof(t_linebuffer)*BUFFER_COUNT);
}

static inline bool CompareAndStore(uint32_t *dest, uint32_t new_value, uint32_t old_value)
{
  do
  {
    if (__LDREXW(dest) != old_value) return true; // Failure
  } while(__STREXW(new_value, dest));
  return false;
}
uint32_t atomic_test_and_set(uint32_t* dest) {
	uint32_t old = __LDREXW(dest);
	while(__STREXW(1, dest));
	return old;
}
void atomic_clear(uint32_t* dest) {
	do{
		if(__LDREXW(dest) == 0) break;
	} while (__STREXW(0, dest));
}
t_linebuffer* get_buffer(uint8_t irq){
	while(irq_lines[irq] == NULL) {
		for(uint16_t i=0; i < BUFFER_COUNT; i++) {
			t_linebuffer* q = &buffers[i];
			if(atomic_test_and_set(&q->lock)==0){
				irq_lines[irq] = q;
				q->length=0; // just in case
				__enable_irq(); // got to do this..mabye?
				return q;
			}
			//__enable_irq(); // got to do this..mabye?
		}
		HAL_Delay(1); // delay for a free buffer
	}
	return irq_lines[irq] ;
}
void fifo_add_to_tx(uint8_t irq){
	t_linebuffer* q = irq_lines[irq];
	irq_lines[irq]=NULL;
	__disable_irq(); // got to do this..mabye?
	SIMPLEQ_INSERT_TAIL(&tx_fifo,q, tx_fifo);
	__enable_irq();
}


void uart_putraw(uint8_t c) {
	if(!uart_init) return;
	while(!(__HAL_UART_GET_FLAG(s_chuart, UART_FLAG_TXE))); // trasmit empty
	s_chuart->Instance->DR = c;
}
void direct_uart_puts(const char* str) {
	if(!uart_init) return;
	while(*str)uart_putraw(*str++);
	while(!(__HAL_UART_GET_FLAG(s_chuart, UART_FLAG_TC))); // trasmit complete
}

void uart_raw_write(const uint8_t* data, size_t len){
	while(len--) uart_putc(*data++);
}


void fifo_fill_dma_buffer() {
	if(!uart_init) return;
	//__disable_irq();
	if(!uart_trasmiting) {// return if the dma is running
		size_t dma_length=0;
		t_linebuffer* q;
		while((q = SIMPLEQ_FIRST(&tx_fifo))) {
			if((dma_length + q->length) > DMA_BUFFER) break;
			memcpy(&dma_buffer[dma_length],q->data,q->length);
			dma_length+= q->length;
			SIMPLEQ_REMOVE_HEAD(&tx_fifo, tx_fifo);
			q->length = 0;
			atomic_clear(&q->lock);
		}
		if(dma_length>0) HAL_UART_Transmit_DMA(&huart3,(uint8_t*)dma_buffer, dma_length);
	}
	//__enable_irq();
}


void fifo_write(const uint8_t* data, size_t length) {
	uint8_t irq = __get_IPSR() & 0xFF;
	while(length){
		t_linebuffer*  buf = get_buffer(irq);
		while(length && buf->length < (LINE_SIZE-3)) {  // account for \r\n\0
			uint8_t c = *data++;
			if(IS_NEWLINECHAR(c)) {
				if(c != *data && IS_NEWLINECHAR(*data)) data++; // do we even need to skip?
				buf->data[buf->length++] = '\r';
				buf->data[buf->length++] = '\n';
				buf->data[buf->length++] = '\0';
				fifo_add_to_tx(irq);
				buf = get_buffer(irq);
				assert(buf);

			} else  buf->data[buf->length++] = c;
			length--;
		}
	}
}
#define USARTx_IT_IRQ	  		  USART3_IRQn
#define USARTx_IT__IRQHandler     USART3_IRQHandler
#if 0
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
#endif








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
	fifo_write(data,len);
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

void uart_raw_print(const char* fmt,...){
	char buf[128];
	va_list va;
	va_start(va,fmt);
	size_t len = vsprintf(buf,fmt,va);
	va_end(va);
	buf[127]=0;
	uart_raw_write((uint8_t*)buf,len);
}

void DebugMessage(LogLevelTypeDef level, const char* message){
	static const char* level_to_string[] = {
		ANSI_COLOR_FORGROUND_RED "ERROR" ANSI_COLOR_RESET ": ",
		ANSI_COLOR_FORGROUND_YELLOW "WARN" ANSI_COLOR_RESET ": ",
		ANSI_COLOR_FORGROUND_WHITE "USER " ANSI_COLOR_RESET ": ",
		ANSI_COLOR_FORGROUND_WHITE "DEBUG" ANSI_COLOR_RESET ": ",
	};
	struct timeval t1;
	char buf[128];
	gettimeofday(&t1, NULL);
	uint32_t irq = __get_IPSR();
	assert(message);
	int len=sprintf(buf,"[%2X:%lu.%07lu] %s: %s\r\n", irq, t1.tv_sec ,t1.tv_usec ,level_to_string[level],message);
	buf[127]=0; // sanity
	uart_write((uint8_t*)buf,len);
}
void PrintDebugMessage(LogLevelTypeDef level, const char* fmt, ...){
	char buf[128];
	va_list va;
	va_start(va,fmt);
	vsprintf(buf,fmt,va);
	va_end(va);
	buf[127]=0;
	DebugMessage(level,buf);
}
#endif
