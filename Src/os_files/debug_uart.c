/* Copyright (c) 2013 The F9 Microkernel Project. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <os/f9_conf.h>
#include <os/platform/debug_uart.h>

#include <os/lib/queue.h>
#define BOARD_USART USART3

static struct dbg_uart_t dbg_uart;
static uint8_t dbg_uart_tx_buffer[SEND_BUFSIZE];
static uint8_t dbg_uart_rx_buffer[RECV_BUFSIZE];

enum { DBG_ASYNC, DBG_PANIC } dbg_state;

uint8_t console_uart_getc() {
	while((BOARD_USART->SR & USART_SR_RXNE)==0);
	return BOARD_USART->DR;
}
void console_uart_putc(uint8_t c) {
	while((BOARD_USART->SR & USART_SR_TXE)==0);
	BOARD_USART->DR = c;
}
//void USART3_IRQHandler()
void __uart_debug_irq_handler(void)
{
	if((BOARD_USART->CR1 & USART_CR1_TXEIE) && (BOARD_USART->SR & USART_SR_TXE)) {
		if(queue_is_empty(&(dbg_uart.tx))) {
			BOARD_USART->CR1 &= ~USART_SR_TXE;
			BOARD_USART->CR1 |= ~USART_CR1_TCIE;
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
		BOARD_USART->SR &= ~USART_SR_TC;

	}
}


static void dbg_async_putchar(char chr)
{
	/* If UART is busy, try to put chr into queue until slot is freed,
	 * else write directly into UART
	 */
	if (!dbg_uart.ready) {
		while (queue_push(&(dbg_uart.tx), chr) != QUEUE_OK)
			/* wait */ ;
	} else {
		console_uart_putc(chr);
		dbg_uart.ready = 0;
		BOARD_USART->CR1 |= USART_SR_TXE;
	}
}

static void dbg_sync_putchar(char chr)
{
	if (chr == '\n')
		dbg_sync_putchar('\r');

	console_uart_putc( chr);
	while((BOARD_USART->SR & USART_SR_TC)==0);
}

uint8_t dbg_uart_getchar(void)
{
	uint8_t chr = 0;

	if (queue_pop(&(dbg_uart.rx), &chr) == QUEUE_EMPTY)
		return 0;
	return chr;

}

void dbg_uart_putchar(uint8_t chr)
{
	/* During panic, we cannot use async dbg uart, so switch to
	 * synchronious mode
	 */
	if (dbg_state != DBG_PANIC)
		dbg_async_putchar(chr);
	else
		dbg_sync_putchar(chr);
}
static void dbg_uart_start_panic(void)
{
	unsigned char chr;

	/* In panic condition, we can be in interrupt context or
	 * not, so will write sequence synchronously */

	/* Flush remaining sequence  in async buffer */
	while (queue_pop(&(dbg_uart.tx), &chr) != QUEUE_EMPTY) {
		dbg_sync_putchar(chr);
	}

	dbg_state = DBG_PANIC;
}
UART_HandleTypeDef huart3;
#ifndef __CONCAT__
#define __CONCAT__(a,b) a ## b
#endif

#define DBG_USART_IRQ(a)
void dbg_uart_init(void)
{

  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  assert(HAL_UART_Init(&huart3) == HAL_OK);

	dbg_uart.ready = 1;
	queue_init(&(dbg_uart.tx), dbg_uart_tx_buffer, SEND_BUFSIZE);
	queue_init(&(dbg_uart.rx), dbg_uart_rx_buffer, RECV_BUFSIZE);

	dbg_state = DBG_ASYNC;
	BOARD_USART->CR1 |= USART_CR1_RXNEIE;

	  HAL_NVIC_SetPriority(USART3_IRQn, 0xf, 0);
	  HAL_NVIC_ClearPendingIRQ(USART3_IRQn);
	  HAL_NVIC_EnableIRQ(USART3_IRQn);
}
