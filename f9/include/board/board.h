/* Copyright (c) 2013-2014 The F9 Microkernel Project. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DISCOVERYF4_BOARD_H_
#define DISCOVERYF4_BOARD_H_

#include <stm32f4xx.h>
extern struct usart_dev console_uart;

#if defined(CONFIG_DBGPORT_USE_USART1)

#define BOARD_UART_DEVICE	USART1_IRQn
#define BOARD_UART_HANDLER	USART1_HANDLER
#define BOARD_USART_FUNC	af_usart1
#define BOARD_USART_CONFIGS			\
	.base = USART1_BASE,			\
	.rcc_apbenr = RCC_USART1_APBENR,	\
	.rcc_reset = RCC_APB2RSTR_USART1RST,
#define BOARD_USART_TX_IO_PORT	GPIOA
#define BOARD_USART_TX_IO_PIN	9
#define BOARD_USART_RX_IO_PORT	GPIOA
#define BOARD_USART_RX_IO_PIN	10


#elif defined(CONFIG_DBGPORT_USE_USART2)

#define BOARD_UART_DEVICE	USART2_IRQn
#define BOARD_UART_HANDLER	USART2_HANDLER
#define BOARD_USART_FUNC	af_usart2
#define BOARD_USART_CONFIGS			\
	.base = USART2_BASE,			\
	.rcc_apbenr = RCC_USART2_APBENR,	\
	.rcc_reset = RCC_APB1RSTR_USART2RST,
#define BOARD_USART_TX_IO_PORT	GPIOA
#define BOARD_USART_TX_IO_PIN	2
#define BOARD_USART_RX_IO_PORT	GPIOA
#define BOARD_USART_RX_IO_PIN	3

#else	/* default: USART4 */
	/* CONFIG_DBGPORT_USE_USART4 */

#define BOARD_USART USART3
#endif

#endif	/* DISCOVERYF4_BOARD_H_ */
