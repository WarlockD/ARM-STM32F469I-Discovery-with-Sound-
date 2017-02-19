/* Copyright (c) 2014 The F9 Microkernel Project. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <f9_conf.h>
#include <platform/cortex_m.h>


void init_systick(uint32_t tick_reload, uint32_t tick_next_reload)
{
	/* 250us at 168Mhz */
	SysTick->LOAD= tick_reload - 1;
	SysTick->VAL=0;
	SysTick->CTRL=0x00000007; // decode this latter

	if (tick_next_reload)
		SysTick->LOAD = tick_next_reload - 1;
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
