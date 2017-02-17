/*
 * linux/include/asm-arm/atomic.h
 *
 * Copyright (c) 1996 Russell King.
 *
 * Changelog:
 *  27-06-1996	RMK	Created
 *  13-04-1997	RMK	Made functions atomic
 */
#ifndef __ASM_ARM_ATOMIC_H
#define __ASM_ARM_ATOMIC_H

#include <asm/system.h>

typedef int atomic_t;
#define ATOMIC_INIT(i)	{ (i) }

/*
 * On ARM, ordinary assignment (str instruction) doesn't clear the local
 * strex/ldrex monitor on some implementations. The reason we can use it for
 * atomic_set() is the clrex or dummy strex done on every exception return.
 */
#define atomic_read(v)	READ_ONCE((v)->counter)
#define atomic_set(v,i)	WRITE_ONCE(((v)->counter), (i))
#define ATOMIC_OP(op, c_op, asm_op)					\
static inline void atomic_##op(int i, atomic_t *v)			\
{									\
	unsigned long tmp;						\
	int result;							\
									\
									__builtin_prefetch(v);						\
	__asm__ __volatile__("@ atomic_" #op "\n"			\
"1:	ldrex	%0, [%3]\n"						\
"	" #asm_op "	%0, %0, %4\n"					\
"	strex	%1, %0, [%3]\n"						\
"	teq	%1, #0\n"						\
"	bne	1b"							\
	: "=&r" (result), "=&r" (tmp), "+Qo" (*v)		\
	: "r" (v), "Ir" (i)					\
	: "cc");							\
}									\

#define ATOMIC_OP_RETURN(op, c_op, asm_op)				\
static inline int atomic_##op##_return_relaxed(int i, atomic_t *v)	\
{									\
	unsigned long tmp;						\
	int result;							\
									\
									__builtin_prefetch(v);						\
									\
	__asm__ __volatile__("@ atomic_" #op "_return\n"		\
"1:	ldrex	%0, [%3]\n"						\
"	" #asm_op "	%0, %0, %4\n"					\
"	strex	%1, %0, [%3]\n"						\
"	teq	%1, #0\n"						\
"	bne	1b"							\
	: "=&r" (result), "=&r" (tmp), "+Qo" (*v)		\
	: "r" (v), "Ir" (i)					\
	: "cc");							\
									\
	return result;							\
}

#define atomic_add_return_relaxed	atomic_add_return_relaxed
#define atomic_sub_return_relaxed	atomic_sub_return_relaxed

extern __inline__  int atomic_cas(volatile int *dest, int old_value, int new_value){
	int ret;
	do {
		if(__builtin_expect(__LDREXW(dest) != old_value)) return 1;
	} while(__builtin_expect(ret=__STREXW(dest,new_value)));
	return ret;
}
extern __inline__ void atomic_add(atomic_t i, atomic_t *v)
{
	save_flags_cli (flags);
	*v += i;
	restore_flags (flags);
}

extern __inline__ void atomic_sub(atomic_t i, atomic_t *v)
{
	unsigned long flags;

	save_flags_cli (flags);
	*v -= i;
	restore_flags (flags);
}

extern __inline__ void atomic_inc(atomic_t *v)
{
	unsigned long flags;

	save_flags_cli (flags);
	*v += 1;
	restore_flags (flags);
}

extern __inline__ void atomic_dec(atomic_t *v)
{
	unsigned long flags;

	save_flags_cli (flags);
	*v -= 1;
	restore_flags (flags);
}

extern __inline__ int atomic_dec_and_test(atomic_t *v)
{
	unsigned long flags;
	int result;

	save_flags_cli (flags);
	*v -= 1;
	result = (*v == 0);
	restore_flags (flags);

	return result;
}

static inline unsigned long __xchg(unsigned long x, volatile void *ptr, int size)
{
	extern void __bad_xchg(volatile void *, int);
	unsigned long ret;
	unsigned int tmp;

	//prefetchw((const void *)ptr);
	__builtin_prefetch (ptr);

	switch (size) {
	case 1:
		asm volatile("@	__xchg1\n"
		"1:	ldrexb	%0, [%3]\n"
		"	strexb	%1, %2, [%3]\n"
		"	teq	%1, #0\n"
		"	bne	1b"
			: "=&r" (ret), "=&r" (tmp)
			: "r" (x), "r" (ptr)
			: "memory", "cc");
		break;
	case 2:
		asm volatile("@	__xchg2\n"
		"1:	ldrexh	%0, [%3]\n"
		"	strexh	%1, %2, [%3]\n"
		"	teq	%1, #0\n"
		"	bne	1b"
			: "=&r" (ret), "=&r" (tmp)
			: "r" (x), "r" (ptr)
			: "memory", "cc");
		break;
#endif
	case 4:
		asm volatile("@	__xchg4\n"
		"1:	ldrex	%0, [%3]\n"
		"	strex	%1, %2, [%3]\n"
		"	teq	%1, #0\n"
		"	bne	1b"
			: "=&r" (ret), "=&r" (tmp)
			: "r" (x), "r" (ptr)
			: "memory", "cc");
		break;
	default:
		/* Cause a link-time error, the xchg() size is not supported */
		__bad_xchg(ptr, size), ret = 0;
		break;
	}

	return ret;
}

#define xchg_relaxed(ptr, x) ({						\
	(__typeof__(*(ptr)))__xchg((unsigned long)(x), (ptr),		\
				   sizeof(*(ptr)));			\
})

#endif

