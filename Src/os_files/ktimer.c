/* Copyright (c) 2013 The F9 Microkernel Project. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <os/ktimer.h>

#include <stm32f4xx.h>
#include <os/platform/armv7m.h>
#include <queue.h>

#include <os/systick.h>
#include <os/bitops.h>
#include <os/debug.h>
#include <os/lib/ktable.h>
#include <os/platform/irq.h>
#if defined(CONFIG_KTIMER_TICKLESS) && defined(CONFIG_KTIMER_TICKLESS_VERIFY)
#include <tickless-verify.h>
#endif

#include <stm32f4xx.h>
#include <stm32f4xx_hal.h>
#include <sys\time.h>


//#define __HAL_RCC_SYSCLK_CONFIG(__RCC_SYSCLKSOURCE__) MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, (__RCC_SYSCLKSOURCE__))
/**
  * @brief  Configures the SysTick clock source.
  * @param  CLKSource: specifies the SysTick clock source.
  *          This parameter can be one of the following values:
  *             @arg SYSTICK_CLKSOURCE_HCLK_DIV8: AHB clock divided by 8 selected as SysTick clock source.
  *             @arg SYSTICK_CLKSOURCE_HCLK: AHB clock selected as SysTick clock source.
  * @retval None
*/

static uint32_t ms_systick=0;
static uint32_t us_systick=0;
static uint32_t ktick_scale = 0;
static uint64_t ktimer_now;
static bool ktimer_enabled = false;
static uint32_t ktimer_delta = 0;
static long long ktimer_time = 0;
_TAILQ_HEAD(ktimer_queue_t,ktimer_event_t,);

static struct ktimer_queue_t ktimer_queue;
static struct ktimer_queue_t ktimer_event_queue;

DECLARE_KTABLE(ktimer_event_t, ktimer_event_table, CONFIG_MAX_KT_EVENTS);
/* Next chain of events which will be executed */

void init_systick(uint32_t tick_reload, uint32_t tick_next_reload)
{
	/* 250us at 168Mhz */
	SysTick->LOAD= tick_reload - 1;
	SysTick->VAL=0;
	SysTick->CTRL=0x00000007; // decode this latter but I know enable is in there
	//SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_CLKSOURCE_Msk
	if (tick_next_reload)
		SysTick->LOAD = tick_next_reload - 1;
}
void systick_cfg() {
	ms_systick = HAL_RCC_GetHCLKFreq() /1000U;
	us_systick = HAL_RCC_GetHCLKFreq() /1000000U;
	ktick_scale = HAL_RCC_GetHCLKFreq()/100000; // 100us
	init_systick(ms_systick,0);

  /* SysTick_IRQn interrupt configuration */
  //HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

void init_systick_tm(struct timeval* reload, struct timeval* next_reload){
	uint32_t tick_reload = (reload->tv_sec * ms_systick) + (reload->tv_usec * us_systick);
	uint32_t tick_next_reload = 0;
	if(next_reload)tick_next_reload = (next_reload->tv_sec * ms_systick) + (next_reload->tv_usec * us_systick);
	init_systick(tick_reload,tick_next_reload);
}
void systick_disable()
{
	SysTick->CTRL = 0x00000000;
}

uint32_t systick_now()
{
	return SysTick->VAL;
}

uint32_t systick_flag_count()
{
	return (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) >> SysTick_CTRL_COUNTFLAG_Pos;
}


/*
 * Simple ktimer implementation
 */




static void ktimer_init(void)
{
	systick_cfg();
	TAILQ_INIT(&ktimer_queue);
	TAILQ_INIT(&ktimer_event_queue);
	ktimer_enabled = false;
	ktimer_time=0;
	init_systick(CONFIG_KTIMER_HEARTBEAT, 0);
}


void SysTick_Handler() __NAKED;
void SysTick_Handler()
//void __ktimer_handler(void)
{
	ktimer_event_t* event, *next_event;
	uint32_t next_time = 0;
	TAILQ_FOREACH_SAFE(event, &ktimer_event_queue, queue,next_event) {
		TAILQ_REMOVE(&ktimer_event_queue, event,queue);
		uint32_t ret = event->handler(event);
		if(ret == 0)
			ktable_free(&ktimer_event_table, event);
		else
			ktimer_event_schedule(ret, event);
	}
	if(!TAILQ_EMPTY(&ktimer_queue)){
		next_time = TAILQ_FIRST(&ktimer_queue)->delta;
		TAILQ_FOREACH_SAFE(event, &ktimer_queue, queue,next_event) {
			event->delta -= next_time;
			if(event->delta==0){
				TAILQ_REMOVE(&ktimer_queue, event, queue);
				TAILQ_INSERT_TAIL(&ktimer_event_queue, event,queue);
			}
		}
		SysTick->LOAD = next_time -1U;
	} else {
		SysTick->LOAD = ms_systick * 100; // 100 ms from now,
	}
}



int ktimer_event_schedule(uint32_t ticks, ktimer_event_t *kte)
{
	ktimer_event_t *event = NULL, *next_event = NULL;

	if (!ticks) return -1;
	kte->delta = ticks;
	if(!TAILQ_EMPTY(&ktimer_queue)) {
		kte->delta += TAILQ_FIRST(&ktimer_queue)->delta;
		kte->delta = ticks;
		TAILQ_FOREACH_SAFE(event,&ktimer_queue,queue,next_event) {
			if(kte->delta  < event->delta){ // found it

				TAILQ_INSERT_BEFORE(event, kte,queue);
				return 0;
			}
		}
	}
	TAILQ_INSERT_TAIL(&ktimer_queue, kte,queue);
	return 0;
}

ktimer_event_t *ktimer_event_create(uint32_t ticks,
	                                ktimer_event_handler_t handler,
	                                void *data)
{
	ktimer_event_t *kte = NULL;

	if (!handler)
		goto ret;

	kte = (ktimer_event_t *) ktable_alloc(&ktimer_event_table);

	/* No available slots */
	if (kte == NULL)
		goto ret;

	kte->handler = handler;
	kte->data = data;

	if (ktimer_event_schedule(ticks, kte) == -1) {
		ktable_free(&ktimer_event_table, kte);
		kte = NULL;
	}

ret:
	return kte;
}


void ktimer_event_init()
{
	//ktable_init(&ktimer_event_table);
	ktimer_init();
	//softirq_register(KTE_SOFTIRQ, ktimer_event_handler);
}


#ifdef CONFIG_KDB
void kdb_dump_events(void)
{
	ktimer_event_t *event = event_queue;

	dbg_puts("\nktimer events: \n");
	dbg_printf(DL_KDB, "%8s %12s\n", "EVENT", "DELTA");

	while (event) {
		dbg_printf(DL_KDB, "%p %12d\n", event, event->delta);

		event = event->next;
	}
}


#ifdef CONFIG_KTIMER_TICKLESS

#define KTIMER_MAXTICKS (SYSTICK_MAXRELOAD / CONFIG_KTIMER_HEARTBEAT)

static uint32_t volatile ktimer_tickless_compensation = CONFIG_KTIMER_TICKLESS_COMPENSATION;
static uint32_t volatile ktimer_tickless_int_compensation = CONFIG_KTIMER_TICKLESS_INT_COMPENSATION;

void ktimer_enter_tickless()
{
	uint32_t tickless_delta;
	uint32_t reload;

	irq_disable();

	if (ktimer_enabled && ktimer_delta <= KTIMER_MAXTICKS) {
		tickless_delta = ktimer_delta;
	} else {
		tickless_delta = KTIMER_MAXTICKS;
	}

	/* Minus 1 for current value */
	tickless_delta -= 1;

	reload = CONFIG_KTIMER_HEARTBEAT * tickless_delta;

	reload += systick_now() - ktimer_tickless_compensation;

	if (reload > 2) {
		init_systick(reload, CONFIG_KTIMER_HEARTBEAT);

#if defined(CONFIG_KDB) && \
    defined(CONFIG_KTIMER_TICKLESS) && defined(CONFIG_KTIMER_TICKLESS_VERIFY)
		tickless_verify_count();
#endif
	}

	wait_for_interrupt();

	if (!systick_flag_count()) {
		uint32_t tickless_rest = (systick_now() / CONFIG_KTIMER_HEARTBEAT);

		if (tickless_rest > 0) {
			int reload_overflow;

			tickless_delta = tickless_delta - tickless_rest;

			reload = systick_now() % CONFIG_KTIMER_HEARTBEAT - ktimer_tickless_int_compensation;
			reload_overflow = (int)reload < 0 ? 1: 0;
			reload += reload_overflow * CONFIG_KTIMER_HEARTBEAT;

			init_systick(reload, CONFIG_KTIMER_HEARTBEAT);

			if (reload_overflow) {
				tickless_delta++;
			}

#if defined(CONFIG_KDB) && \
    defined(CONFIG_KTIMER_TICKLESS) && defined(CONFIG_KTIMER_TICKLESS_VERIFY)
			tickless_verify_count_int();
#endif
		}
	}

	ktimer_time += tickless_delta;
	ktimer_delta -= tickless_delta;
	ktimer_now += tickless_delta;

	irq_enable();
}
#endif
#endif /* CONFIG_KTIMER_TICKLESS */
