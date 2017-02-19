/* Copyright (c) 2014 The F9 Microkernel Project. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <f9_conf.h>
#include <stm32f4xx.h>

#include <platform/cortex_m.h>
#include <platform/irq.h>
#if 0
#include INC_PLAT(nvic.h)
#include INC_PLAT(nvic.c)
#endif
#define AIRCR_VECTKEY_MASK    ((uint32_t) 0x05FA0000)
#define DEFAULT_IRQ_VEC(n)						\
	void nvic_handler##n(void)					\
		__attribute__((weak, alias("_undefined_handler")));

#define IRQ_VEC_N_OP	DEFAULT_IRQ_VEC
#if 0
#include INC_PLAT(nvic_private.h)
#endif
#undef IRQ_VEC_N_OP

void _undefined_handler(void)
{
	while (1)
		/* wait */ ;
}

/* for the sake of saving space, provide default device IRQ handler with
 * weak alias.
 */

extern void (* const g_pfnVectors[])(void);
int nvic_is_setup(int irq)
{
	return !(g_pfnVectors[irq + 16] == _undefined_handler);
}
#if 0
void NVIC_PriorityGroupConfig(uint32_t NVIC_PriorityGroup)
{
	if (!(NVIC_PriorityGroup == NVIC_PriorityGroup_0 ||
	      NVIC_PriorityGroup == NVIC_PriorityGroup_1 ||
	      NVIC_PriorityGroup == NVIC_PriorityGroup_2 ||
	      NVIC_PriorityGroup == NVIC_PriorityGroup_3 ||
	      NVIC_PriorityGroup == NVIC_PriorityGroup_4))
		return;
	NVIC_SetPriorityGrouping(NVIC_PriorityGroup>>16);
}

void NVIC_SetPriority(IRQn_Type IRQn, uint8_t group_priority,
                      uint8_t sub_priority)
{
	uint8_t priority = 0x0, group_shifts = 0x0, sub_shifts = 0x0;

	sub_shifts = (0x700 - ((*SCB_AIRCR) & (uint32_t) 0x700)) >> 0x08;
	group_shifts = 0x4 - sub_shifts;

	priority = (group_priority << group_shifts) |
	           (sub_priority & (0xf >> sub_shifts));

	if (IRQn < 0)
		((volatile uint8_t *) SCB_SHPR)[
		        (((uint32_t) IRQn) & 0xf) - 4] = priority << 0x4;
	else
		NVIC->IP[IRQn] = priority << 0x4;
}
#endif
