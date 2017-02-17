/*
 * linux/include/asm-arm/proc-armv/system.h
 *
 * Copyright (C) 1996 Russell King
 */

#ifndef __ASM_PROC_SYSTEM_H
#define __ASM_PROC_SYSTEM_H

extern const char xchg_str[];

#include "stm32f469xx.h"
#include <cmsis_gcc.h>

#ifndef mb
#define mb() do { __asm volatile( "dsb" );__asm volatile( "isb" ); } while(0)
#endif



//#define CONTEXT_SWITCH()	do { * ( ( volatile uint32_t * ) 0xe000ed04 ) ) |= SCB_ICSR_PENDSVSET_Msk; } while(0)
#define CONTEXT_SWITCH()	do { SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; mb(); } while(0)


extern __inline__ unsigned long __xchg(unsigned long x, volatile void *ptr, int size)
{
	switch (size) {
		case 1:	__asm__ __volatile__ ("swpb %0, %1, [%2]" : "=r" (x) : "r" (x), "r" (ptr) : "memory");
			break;
		case 2:	abort ();
		case 4:	__asm__ __volatile__ ("swp %0, %1, [%2]" : "=r" (x) : "r" (x), "r" (ptr) : "memory");
			break;
		default: arm_invalidptr(xchg_str, size);
	}
	return x;
}

/*-----------------------------------------------------------
 * Enable IRQs
 */
#define sti() do { mb();  __asm volatile ("cpsie i" : : : "memory"); } while(0)
#define cli() do { mb();  __asm volatile ("cpsid i" : : : "memory"); } while(0)



/*
 * This processor does not need anything special before reset,
 * but RPC may do...
 */
extern __inline__ void proc_hard_reset(void)
{
}

/*
 * We can wait for an interrupt...
 */
#if 0
#define proc_idle()			\
	do {				\
	__asm__ __volatile__(		\
"	mcr	p15, 0, %0, c15, c8, 2"	\
	  : : "r" (0));			\
	} while (0)
#else
#define proc_idle()
#endif
/*
 * A couple of speedups for the ARM
 */

/*
 * save current IRQ & FIQ state
 */
#define save_flags(x)	do { \
		__asm volatile ("MRS %0, primask" : "=r" (x) ); \
		} while (0)

#define restore_flags(x)	do { \
	uint32_t tmp;				\
	__asm volatile ("MRS %0, primask" : "=r" (tmp) ); \
	if(((uint32_t)(x)) != tmp) { if(x) cli(); else sti(); } \
} while (0)

/*
 * Save the current interrupt enable state & disable IRQs
 */
// we could just change the basepri to 0 and max
#define save_flags_cli(x) do {\
		save_flags(x); \
		if(!x) cli();  \			\
	} while (0)



#endif
