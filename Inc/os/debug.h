/* Copyright (c) 2013 The F9 Microkernel Project. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#include <os/types.h>
#include <stdarg.h>

typedef enum {

	DL_EMERG	= 0x0000,
	DL_BASIC	= 0x8000,
	DL_KDB 		= 0x4000,
	DL_KTABLE	= 0x0001,
	DL_SOFTIRQ	= 0x0002,
	DL_THREAD	= 0x0004,
	DL_KTIMER	= 0x0008,
	DL_SYSCALL	= 0x0010,
	DL_SCHEDULE	= 0x0020,
	DL_MEMORY	= 0x0040,
	DL_IPC		= 0x0080
} dbg_layer_t;


#ifndef CONFIG_DEBUG
#define dbg_puts(x)		do {} while(0)
#define dbg_printf(...)		do {} while(0)
#define dbg_vprintf(...)	do {} while(0)

#else


/*
* Receive a character from debug port
*/


/*
* Send a character to debug port
*/
const char* dbg_irq_name(uint32_t irq);
void dbg_init();
uint8_t dbg_getchar(void);
void dbg_putchar(uint8_t chr);
void dbg_puts(const char* str);
void dbg_printf(dbg_layer_t layer, char *fmt, ...);
void dbg_vprintf(dbg_layer_t layer, char *fmt, va_list va);
void dbg_panic(const char*fmt,...); // switches to panic mode
void printk(const char *fmt, ...);
void vprintk(const char *fmt, va_list va);
#endif

#endif /* DEBUG_H_ */
