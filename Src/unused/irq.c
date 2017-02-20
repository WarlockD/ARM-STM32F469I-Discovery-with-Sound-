/* Copyright (c) 2013 The F9 Microkernel Project. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <os/platform/irq.h>

#include <os/f9_conf.h>
#include <os/link.h>
void irq_init(void)
{
	/* Set all 4-bit to pre-emption priority bit */
	//HAL_NVIC_PriorityGroupConfig(4);

	/* Set default priority from high to low */

	HAL_NVIC_SetPriority(DebugMonitor_IRQn, 0x0, 0);

	HAL_NVIC_SetPriority(MemoryManagement_IRQn, 0x1, 0);
	HAL_NVIC_SetPriority(BusFault_IRQn, 0x1, 0);
	HAL_NVIC_SetPriority(UsageFault_IRQn, 0x1, 0);

	HAL_NVIC_SetPriority(SysTick_IRQn, 0x3, 0);

	/* Priority 0xF - debug_uart */
	HAL_NVIC_SetPriority(SVCall_IRQn, 0xF, 0);
	HAL_NVIC_SetPriority(PendSV_IRQn, 0xF, 0);
}



