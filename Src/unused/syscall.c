/* Copyright (c) 2013, 2014 The F9 Microkernel Project. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <os/syscall.h>
#include <os/softirq.h>
#include <os/thread.h>
#include <os/ipc.h>
#include <os/l4/utcb.h>
//#include <os/memory.h>
#include <os/init_hook.h>

#include <os/platform/armv7m.h>
#include <os/debug.h>
#include <os/f9_conf.h>
#include <os/platform/irq.h>


tcb_t *caller;
#if 0
void __svc_handler(void)
{
	extern tcb_t *kernel;

	/* Kernel requests context switch, satisfy it */
	if (thread_current() == kernel)
		return;

	caller = thread_current();
	caller->state = T_SVC_BLOCKED;

	softirq_schedule(SYSCALL_SOFTIRQ);
}

IRQ_HANDLER(SVC_Handler, __svc_handler);
#endif




void syscall_init()
{
	softirq_register(SYSCALL_SOFTIRQ, syscall_handler);
}

INIT_HOOK(syscall_init, INIT_LEVEL_KERNEL);

static void sys_thread_control(uint32_t *param1, uint32_t *param2)
{
	l4_thread_t dest = param1[REG_R0];
	l4_thread_t space = param1[REG_R1];
	l4_thread_t pager = param1[REG_R3];

	if (space != L4_NILTHREAD) {
		/* Creation of thread */
		void *utcb = (void *) param2[0];	/* R4 */

#ifdef CONFIG_MEMPOOLS
		mempool_t *utcb_pool = mempool_getbyid(mempool_search((memptr_t) utcb,
		                                       UTCB_SIZE));

		if (!utcb_pool || !(utcb_pool->flags & (MP_UR | MP_UW))) {
			/* Incorrect UTCB relocation */
			return;
		}
#endif
		tcb_t *thr = thread_create(dest, utcb);
		thread_space(thr, space, utcb);
		thr->utcb->t_pager = pager;
		param1[REG_R0] = 1;
	} else {
		/* Removal of thread */
		tcb_t *thr = thread_by_globalid(dest);
		thread_free_space(thr);
		thread_destroy(thr);
	}
}
#if 0
.word  SVC_Handler
.word  DebugMon_Handler
.word  0
.word  PendSV_Handler
.word  SysTick_Handler

void SVC_Handler() __attribute((naked));
void SVC_Handler() {

}
#endif


void __svc_handler()
{
	uint32_t *svc_param1 = (uint32_t *) caller->ctx.sp;
	uint32_t svc_num = ((char *) svc_param1[REG_PC])[-2];
	uint32_t *svc_param2 = caller->ctx.regs;

	if (svc_num == SYS_THREAD_CONTROL) {
		/* Simply call thread_create
		 * TODO: checking globalid
		 * TODO: pagers and schedulers
		 */
		sys_thread_control(svc_param1, svc_param2);
		caller->state = T_RUNNABLE;
	} else if (svc_num == SYS_IPC) {
		sys_ipc(svc_param1);
	} else {
		dbg_printf(DL_SYSCALL,
		           "SVC: %d called [%d, %d, %d, %d]\n", svc_num,
		           svc_param1[REG_R0], svc_param1[REG_R1],
		           svc_param1[REG_R2], svc_param1[REG_R3]);
		caller->state = T_RUNNABLE;
	}
}
IRQ_HANDLER(SVC_Handler, __svc_handler);


