/*
 * linux/include/asm-arm/semaphore.h
 */
#ifndef __ASM_PROC_SEMAPHORE_H
#define __ASM_PROC_SEMAPHORE_H

/*
 * This is ugly, but we want the default case to fall through.
 * "__down" is the actual routine that waits...
 */
struct semaphore;
extern inline void down(struct semaphore * sem)
{
	(void)sem;
//to do
}

/*
 * This is ugly, but we want the default case to fall through.
 * "__down_interruptible" is the actual routine that waits...
 */
extern inline int down_interruptible (struct semaphore * sem)
{
	//int result;
	(void)sem;
// to do
	//return result;
	return 0;
}

/*
 * Note! This is subtle. We jump to wake people up only if
 * the semaphore was negative (== somebody was waiting on it).
 * The default case (no contention) will result in NO
 * jumps for both down() and up().
 */
extern inline void up(struct semaphore * sem)
{
// to do
	(void)sem;
}

#endif
