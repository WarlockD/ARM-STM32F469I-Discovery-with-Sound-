/* Copyright (c) 2013 The F9 Microkernel Project. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdarg.h>
#include <queue.h>
#include <stdio.h>
#include <os/debug.h>
#include <os/platform/debug_uart.h>


#include <os/f9_conf.h>
uint8_t dbg_uart_getchar(void);
void dbg_uart_putchar(uint8_t chr);

dbg_layer_t dbg_layer;

void dbg_printf(dbg_layer_t layer, char* fmt, ...)
{
//	va_list va;
//	va_start(va, fmt);
//	dbg_vprintf(layer, fmt, va);
//	va_end(va);
}
static const char* irq_names[] = {
		 "_estack",
		  "Reset_Handler",
		  "NMI_Handler",
		  "HardFault_Handler",
		  "MemManage_Handler",
		  "BusFault_Handler",
		  "UsageFault_Handler",
		  "Reserved7",
		  "Reserved8",
		  "Reserved9",
		  "Reserved10",
		  "SVC_Handler",
		  "DebugMon_Handler",
		  "Reserved13",
		  "PendSV_Handler",
		  "SysTick_Handler",

		  /* External Interrupts */
		  "WWDG_IRQHandler",                   /* Window WatchDog              */
		  "PVD_IRQHandler",                    /* PVD through EXTI Line detection */
		  "TAMP_STAMP_IRQHandler",             /* Tamper and TimeStamps through the EXTI line */
		  "RTC_WKUP_IRQHandler",               /* RTC Wakeup through the EXTI line */
		  "FLASH_IRQHandler",                  /* FLASH                        */
		  "RCC_IRQHandler",                    /* RCC                          */
		  "EXTI0_IRQHandler",                  /* EXTI Line0                   */
		  "EXTI1_IRQHandler",                  /* EXTI Line1                   */
		  "EXTI2_IRQHandler",                  /* EXTI Line2                   */
		  "EXTI3_IRQHandler",                  /* EXTI Line3                   */
		  "EXTI4_IRQHandler",                  /* EXTI Line4                   */
		  "DMA1_Stream0_IRQHandler",           /* DMA1 Stream 0                */
		  "DMA1_Stream1_IRQHandler",           /* DMA1 Stream 1                */
		  "DMA1_Stream2_IRQHandler",           /* DMA1 Stream 2                */
		  "DMA1_Stream3_IRQHandler",           /* DMA1 Stream 3                */
		  "DMA1_Stream4_IRQHandler",           /* DMA1 Stream 4                */
		  "DMA1_Stream5_IRQHandler",           /* DMA1 Stream 5                */
		  "DMA1_Stream6_IRQHandler",           /* DMA1 Stream 6                */
		  "ADC_IRQHandler",                    /* ADC1, ADC2 and ADC3s         */
		  "CAN1_TX_IRQHandler",                /* CAN1 TX                      */
		  "CAN1_RX0_IRQHandler",               /* CAN1 RX0                     */
		  "CAN1_RX1_IRQHandler",               /* CAN1 RX1                     */
		  "CAN1_SCE_IRQHandler",               /* CAN1 SCE                     */
		  "EXTI9_5_IRQHandler",                /* External Line[9:5]s          */
		  "TIM1_BRK_TIM9_IRQHandler",          /* TIM1 Break and TIM9          */
		  "TIM1_UP_TIM10_IRQHandler",          /* TIM1 Update and TIM10        */
		  "TIM1_TRG_COM_TIM11_IRQHandler",     /* TIM1 Trigger and Commutation and TIM11 */
		  "TIM1_CC_IRQHandler",                /* TIM1 Capture Compare         */
		  "TIM2_IRQHandler",                   /* TIM2                         */
		  "TIM3_IRQHandler",                   /* TIM3                         */
		  "TIM4_IRQHandler",                   /* TIM4                         */
		  "I2C1_EV_IRQHandler",                /* I2C1 Event                   */
		  "I2C1_ER_IRQHandler",                /* I2C1 Error                   */
		  "I2C2_EV_IRQHandler",                /* I2C2 Event                   */
		  "I2C2_ER_IRQHandler",                /* I2C2 Error                   */
		  "SPI1_IRQHandler",                   /* SPI1                         */
		  "SPI2_IRQHandler",                   /* SPI2                         */
		  "USART1_IRQHandler",                 /* USART1                       */
		  "USART2_IRQHandler",                 /* USART2                       */
		  "USART3_IRQHandler",                 /* USART3                       */
		  "EXTI15_10_IRQHandler",              /* External Line[15:10]s        */
		  "RTC_Alarm_IRQHandler",              /* RTC Alarm (A and B) through EXTI Line */
		  "OTG_FS_WKUP_IRQHandler",            /* USB OTG FS Wakeup through EXTI line */
		  "TIM8_BRK_TIM12_IRQHandler",         /* TIM8 Break and TIM12         */
		  "TIM8_UP_TIM13_IRQHandler",          /* TIM8 Update and TIM13        */
		  "TIM8_TRG_COM_TIM14_IRQHandler",     /* TIM8 Trigger and Commutation and TIM14 */
		  "TIM8_CC_IRQHandler",                /* TIM8 Capture Compare         */
		  "DMA1_Stream7_IRQHandler",           /* DMA1 Stream7                 */
		  "FMC_IRQHandler",                    /* FMC                          */
		  "SDIO_IRQHandler",                   /* SDIO                         */
		  "TIM5_IRQHandler",                   /* TIM5                         */
		  "SPI3_IRQHandler",                   /* SPI3                         */
		  "UART4_IRQHandler",                  /* UART4                        */
		  "UART5_IRQHandler",                  /* UART5                        */
		  "TIM6_DAC_IRQHandler",               /* TIM6 and DAC1&2 underrun errors */
		  "TIM7_IRQHandler",                   /* TIM7                         */
		  "DMA2_Stream0_IRQHandler",           /* DMA2 Stream 0                */
		  "DMA2_Stream1_IRQHandler",           /* DMA2 Stream 1                */
		  "DMA2_Stream2_IRQHandler",           /* DMA2 Stream 2                */
		  "DMA2_Stream3_IRQHandler",           /* DMA2 Stream 3                */
		  "DMA2_Stream4_IRQHandler",           /* DMA2 Stream 4                */
		  "ETH_IRQHandler",                    /* Ethernet                     */
		  "ETH_WKUP_IRQHandler",               /* Ethernet Wakeup through EXTI line */
		  "CAN2_TX_IRQHandler",                /* CAN2 TX                      */
		  "CAN2_RX0_IRQHandler",               /* CAN2 RX0                     */
		  "CAN2_RX1_IRQHandler",               /* CAN2 RX1                     */
		  "CAN2_SCE_IRQHandler",               /* CAN2 SCE                     */
		  "OTG_FS_IRQHandler",                 /* USB OTG FS                   */
		  "DMA2_Stream5_IRQHandler",           /* DMA2 Stream 5                */
		  "DMA2_Stream6_IRQHandler",           /* DMA2 Stream 6                */
		  "DMA2_Stream7_IRQHandler",           /* DMA2 Stream 7                */
		  "USART6_IRQHandler",                 /* USART6                       */
		  "I2C3_EV_IRQHandler",                /* I2C3 event                   */
		  "I2C3_ER_IRQHandler",                /* I2C3 error                   */
		  "OTG_HS_EP1_OUT_IRQHandler",         /* USB OTG HS End Point 1 Out   */
		  "OTG_HS_EP1_IN_IRQHandler",          /* USB OTG HS End Point 1 In    */
		  "OTG_HS_WKUP_IRQHandler",            /* USB OTG HS Wakeup through EXTI */
		  "OTG_HS_IRQHandler",                 /* USB OTG HS                   */
		  "DCMI_IRQHandler",                   /* DCMI                         */
		  "0",                                 /* Reserved                     */
		  "HASH_RNG_IRQHandler",               /* Hash and Rng                 */
		  "FPU_IRQHandler",                    /* FPU                          */
		  "UART7_IRQHandler",                  /* UART7                        */
		  "UART8_IRQHandler",                  /* UART8                        */
		  "SPI4_IRQHandler",                   /* SPI4                         */
		  "SPI5_IRQHandler",                   /* SPI5 						  */
		  "SPI6_IRQHandler",                   /* SPI6						  */
		  "SAI1_IRQHandler",                   /* SAI1						  */
		  "LTDC_IRQHandler",                   /* LTDC           		      */
		  "LTDC_ER_IRQHandler",                /* LTDC error          	      */
		  "DMA2D_IRQHandler",                  /* DMA2D                        */
		  "QUADSPI_IRQHandler",                /* QUADSPI             	      */
		  "DSI_IRQHandler",                    /* DSI                          */
};
const char* dbg_irq_name(uint32_t irq){
	return irq_names[irq];
}

#ifndef __CONCAT__
#define __CONCAT__(a,b) a ## b
#endif

#define DBG_USART_IRQ(a)

#define USARTx_IT_IRQ	  		  USART3_IRQn
#define USARTx_IT__IRQHandler     USART3_IRQHandler

UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_usart3_tx;

#define safe_irq_disable() uint32_t irq_state = __get_PRIMASK(); __disable_irq();
#define safe_irq_enable() if(!irq_state) __enable_irq();



#define BOARD_USART USART3
UART_HandleTypeDef huart3;
enum { DBG_ASYNC, DBG_PANIC } dbg_state;

#define LINELEN 126
typedef struct __dbg_buf_t {
	SIMPLEQ_ENTRY(__dbg_buf_t) queue;
	char data[LINELEN];
	bool lock;
	uint8_t len;
} dbg_buf_t;
dbg_buf_t line_buffers[16]; // lines
SIMPLEQ_HEAD(, __dbg_buf_t) line_cache = SIMPLEQ_HEAD_INITIALIZER(line_cache);
static bool usart_trasmiting = false;
static bool usart_sync_mode = false;


void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart){
	if(&huart3 == huart) {
		__disable_irq();
		dbg_buf_t* buf = SIMPLEQ_FIRST(&line_cache);
		SIMPLEQ_REMOVE_HEAD(&line_cache, queue);
		buf->lock = false;
		__enable_irq();
		if(SIMPLEQ_EMPTY(&line_cache) || usart_sync_mode)  // drop on syn
			usart_trasmiting = false;
		else {
			buf = SIMPLEQ_FIRST(&line_cache);
			HAL_UART_Transmit_DMA(huart, (uint8_t*)buf->data,buf->len);
		}
	}
}
uint8_t console_uart_getc() {
	while((BOARD_USART->SR & USART_SR_RXNE)==0);
	return BOARD_USART->DR;
}
void console_uart_putc(uint8_t c) {
	while((BOARD_USART->SR & USART_SR_TXE)==0);
	BOARD_USART->DR = c;
}
void dbg_sync_trasmit(dbg_buf_t* buf){
	if(buf->len ==0) return;
	usart_trasmiting = true;
	while(usart_trasmiting);
	for(uint32_t i=0;i < buf->len; i++){
		console_uart_putc(buf->data[i]);
	}
	while((BOARD_USART->SR & USART_SR_TC)==0);
	buf->lock = 0;
	usart_trasmiting = false;
}
void dbg_trasmit_buffer(dbg_buf_t* buf) {
	assert(buf->lock);
	if(usart_sync_mode) {
		dbg_sync_trasmit(buf);
	} else {
		safe_irq_disable();
		SIMPLEQ_INSERT_TAIL(&line_cache, buf, queue);
		if(!usart_trasmiting){
			usart_trasmiting = true;
			HAL_UART_Transmit_DMA(&huart3, (uint8_t*)buf->data,buf->len);
		}
		safe_irq_enable();
	}
}
void emergency_flush(bool enable_irq) {
	usart_sync_mode = true;
	while(usart_trasmiting);
	safe_irq_disable();
	if(!SIMPLEQ_EMPTY(&line_cache)) {
		dbg_buf_t* buf;
		SIMPLEQ_FOREACH(buf,&line_cache, queue){
			HAL_UART_Transmit(&huart3, (uint8_t*)buf->data,buf->len,500);
			buf->lock = false;
		}
		SIMPLEQ_INIT(&line_cache);
	}
	if(enable_irq) {	safe_irq_enable(); }
}

void dbg_panic(const char* fmt,...) {
	// switches to panic mode
	safe_irq_disable();
	usart_sync_mode = true;
	if(usart_trasmiting){
		HAL_UART_DMAStop(&huart3);
		usart_trasmiting = false;
	}
	if(!SIMPLEQ_EMPTY(&line_cache)) {
		dbg_buf_t* buf;
		SIMPLEQ_FOREACH(buf,&line_cache, queue){
			HAL_UART_Transmit(&huart3, (uint8_t*)buf->data,buf->len,500);
			buf->lock = false;
		}
		SIMPLEQ_INIT(&line_cache);
	}
	va_list va;
	va_start(va,fmt);
	vprintk(fmt,va);
	va_end(va);
	safe_irq_enable();
}


void USART3_IRQHandler()
{
	HAL_UART_IRQHandler(&huart3);
#if 0
	if((BOARD_USART->CR1 & USART_CR1_TXEIE) && (BOARD_USART->SR & USART_SR_TXE)) {
		if(queue_is_empty(&(dbg_uart.tx))) {
			__HAL_UART_DISABLE_IT(&huart3, UART_IT_TXE);
			__HAL_UART_ENABLE_IT(&huart3, UART_IT_TC);
			//BOARD_USART->CR1 &= ~USART_SR_TXE;
			//BOARD_USART->CR1 |= ~USART_CR1_TCIE;
		} else {
			uint8_t chr;
			queue_pop(&(dbg_uart.tx), &chr);
			BOARD_USART->DR = chr;
		}
		BOARD_USART->SR &= ~USART_SR_TXE;
	}
	if((BOARD_USART->CR1 & USART_CR1_RXNEIE) && (BOARD_USART->SR & USART_SR_RXNE)) {
		uint8_t chr = BOARD_USART->DR;
		/* Put sequence on queue */
		queue_push(&(dbg_uart.rx), chr);
		BOARD_USART->SR &= ~USART_SR_RXNE;
	}
	if((BOARD_USART->CR1 & USART_CR1_TCIE) && (BOARD_USART->SR & USART_SR_TC)) {
		BOARD_USART->CR1 &= ~USART_CR1_TCIE;
		dbg_uart.ready = 1;
		__HAL_UART_DISABLE_IT(&huart3, UART_IT_TC);
		//BOARD_USART->SR &= ~USART_SR_TC;

	}
#endif
}
#if 0
static void fill_dma_buffer() {

	dbg_uart_dma_tx_buffer
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	(void)huart;
	BSP_LED_On(LED_BLUE);
	uart_trasmiting = false;

	//fifo_fill_dma_buffer();
}
#endif

#include <gpio.h>

void HAL_USER_Puts(const char* str){
	HAL_UART_Transmit(&huart3,(uint8_t*)str,strlen(str),500);
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


void dbg_init(void)
{
	usart_sync_mode = true; // meh
//	queue_init(&(dbg_uart.tx), dbg_uart_tx_buffer, SEND_BUFSIZE);
//	queue_init(&(dbg_uart.rx), dbg_uart_rx_buffer, RECV_BUFSIZE);
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  assert(HAL_UART_Init(&huart3) == HAL_OK);
  HAL_USER_Puts("dbg_init Starting\r\n");

	 dbg_state = DBG_ASYNC;
	  HAL_NVIC_SetPriority(USART3_IRQn, 0xf, 0);
	  HAL_NVIC_ClearPendingIRQ(USART3_IRQn);
	  HAL_NVIC_EnableIRQ(USART3_IRQn);
}


dbg_buf_t* get_free_buffer() {
	do {
		dbg_buf_t* buf = NULL;
		safe_irq_disable();
		for(uint32_t i=0;i < 16;i++) {
			if(!line_buffers[i].lock) {
				buf = &line_buffers[i];
				buf->lock = true;
				buf->len = 0;
				break;
			}
		}
		safe_irq_enable();
		if(buf) return buf;
		for(uint32_t i=0;i < 10000U;i++); // short delay
	} while(1);
}

void push_dec(dbg_buf_t* b, const uint32_t val, const int width, const char pad)
{
	uint32_t divisor;
	int digits;

	/* estimate number of spaces and digits */
	for (divisor = 1, digits = 1; val / divisor >= 10; divisor *= 10, digits++)
		/* */ ;

	/* print spaces */
	for (; digits < width; digits++) b->data[b->len++] = pad;

	/* print digits */
	do {
		 b->data[b->len++] = (((val / divisor) % 10) + '0');
	} while (divisor /= 10);
}

static inline void push_char(dbg_buf_t* b, char c) {
	b->data[b->len++] = c;
}
void dbg_vprintf(dbg_layer_t layer, char *fmt, va_list va)
{
	if (layer != DL_EMERG && !(dbg_layer & layer))
		return;
	dbg_buf_t* buf = get_free_buffer();
	uint32_t size = vsnprintf(buf->data, LINELEN-1,fmt,va);
	__l4_vprintf(fmt, va);
}
#define ANSI_ESCAPE_STRING "\033"
#define ANSI_COLOR_FORGROUND_RED 	ANSI_ESCAPE_STRING "[41m"
#define ANSI_COLOR_FORGROUND_YELLOW ANSI_ESCAPE_STRING "[33m"
#define ANSI_COLOR_FORGROUND_WHITE  ANSI_ESCAPE_STRING "[37m"
#define ANSI_COLOR_RESET		    ANSI_ESCAPE_STRING "[0m"
#define IS_NEWLINE_CHAR(c) ((c) == '\r' || (c) == '\n')

void printk(const  char *fmt, ...) {
	va_list va;
	va_start(va,fmt);
	vprintk(fmt,va);
	va_end(va);
}
void vprintk(const char *fmt, va_list va) {
	struct timeval t1;
	gettimeofday(&t1, NULL);
	dbg_buf_t* buf = get_free_buffer();
	push_char(buf, '[');
	push_dec(buf,__get_IPSR(), 2, ' ');
	push_char(buf, ':');
	push_dec(buf,t1.tv_sec, 2, ' ');
	push_char(buf, '.');
	push_dec(buf,t1.tv_usec, 7, '0');
	push_char(buf, ']');
	push_char(buf, ' ');
	int len = vsnprintf(buf->data + buf->len, LINELEN - buf->len-3, fmt,va);
	assert(len>0);
	buf->len += len;
	char *b = buf->data[buf->len-2];
	if(b[1] == '\r') {
		if(b[0] != '\n') push_char(buf, '\n');
	} else if(b[1] == '\n') {
		if(b[0] != '\r') push_char(buf, '\r');
	} else {
		push_char(buf, '\r');
		push_char(buf, '\n');
	}
	dbg_trasmit_buffer(buf);
}
