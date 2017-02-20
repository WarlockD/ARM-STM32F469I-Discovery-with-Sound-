/* Copyright (c) 2013 The F9 Microkernel Project. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <os/bitops.h>

#include <os/platform/armv7m.h>

#ifdef CONFIG_SMP

/* Atomic ops */
void atomic_set(atomic_t *atom, atomic_t newval)
{
	__asm__ __volatile__("mov r1, %0" : : "r"(newval));
	__asm__ __volatile__("atomic_try: ldrex r0, [%0]\n"
	                     "strex r0, r1, [%0]\n"
	                     "cmp r0, #0"
	                     :
	                     : "r"(atom));
	__asm__ __volatile__("bne atomic_try");
}

uint32_t atomic_get(atomic_t *atom)
{
	atomic_t result;

	__asm__ __volatile__("ldrex r0, [%0]"
	                     :
	                     : "r"(atom));
	__asm__ __volatile__("clrex");
	__asm__ __volatile__("mov %0, r0" : "=r"(result));

	return result;
}

#else	/* !CONFIG_SMP */

#if 0
#define unlikely(x) __builtin_expect((long)(x),0)
    static inline int atomic_LL(volatile void *addr) {
      int dest;

  __asm__ __volatile__("ldrex %0, [%1]" : "=r" (dest) : "r" (addr));
  return dest;
}

static inline int atomic_SC(volatile void *addr, int32_t value) {
  int dest;

  __asm__ __volatile__("strex %0, %2, [%1]" :
          "=&r" (dest) : "r" (addr), "r" (value) : "memory");
  return dest;
}
static inline int atomic_CAS(volatile void *addr, int32_t expected,
        int32_t store) {
  int ret;

  do {
    if (unlikely(atomic_LL(addr) != expected))
      return 1;
  } while (unlikely((ret = atomic_SC(addr, store))));
  return ret;

}
#endif
void atomic_set(atomic_t *atom, atomic_t newval)
{
	*atom = newval;
}

uint32_t atomic_get(atomic_t *atom)
{
	return *atom;
}

#endif	/* CONFIG_SMP */

uint32_t test_and_set_word(uint32_t *word)
{
	register uint32_t result;
	do {
		result = __LDREXW(word);
	} while (__STREXW(1,word));
	return result == 0;
}

uint32_t test_and_set_bit(uint32_t *word, int bitmask)
{
	register uint32_t result = 1;
#if 00
	__ASM volatile(
	    "mov r2, %[word]\n"
	    "ldrex r0, [r2]\n"		/* Load value [r2] */
	    "tst r0, %[bitmask]\n"	/* Compare value with bitmask */
	    "ittt eq\n"
	    "orreq r1, r0, %[bitmask]\n"	/* Set bit: r1 = r0 | bitmask */
	    "strexeq r0, r1, [r2]\n"		/* Write value back to [r2] */
	    "moveq %[result], r0\n"
	    : [result] "=r"(result)
	    : [word] "r"(word), [bitmask] "r"(bitmask)
	    : "r0", "r1", "r2");
#else
	register uint32_t new_value;
	do {
		new_value = __LDREXW(word);
		result = new_value & ~bitmask;
		new_value |= bitmask;
	} while (__STREXW(new_value,word));
#endif
	return result == 0;
}
