/*
 * callbacks.h
 *
 *  Created on: Feb 22, 2017
 *      Author: Paul
 */

#ifndef CALLBACKS_H_
#define CALLBACKS_H_

#include <queue.h>
#include <stm32f4xx_hal.h>

typedef struct __callback_t {
	LIST_HEAD(,__callback_t) list;
	int (*callback)(void*);
	void* udata;
	int priority;
} callback_t;

#define safe_lock_begin(NAME) do { uint32_t NAME = __get_PRIMASK();  __irq_disable();
#define safe_lock_end(NAME)  if(!(NAME)) __irq_enable(); } while(0)

int RegesterCallback(IRQn_Type t, int(*func)(void*), void* udata);
void IrqCallbackInits();

#endif /* CALLBACKS_H_ */
