/*
 * semaphore.h
 *
 *  Created on: Feb 21, 2017
 *      Author: Paul
 */

#ifndef OS_SEMAPHORE_H_
#define OS_SEMAPHORE_H_

#include <os/types.h>
#include <os/atomic.h>

//#include <linux/linkage.h>


struct semaphore {
	int count;
	int waking;
	int lock;
	struct wait_queue * wait;
};

#define MUTEX ((struct semaphore) { 1, 0, 0, NULL })
#define MUTEX_LOCKED ((struct semaphore) { 0, 0, 0, NULL })

static inline void semaphore_down (struct semaphore *sem) {

}
static inline void semaphore_up (struct semaphore *sem) {

}
extern int __down_interruptible (struct semaphore *sem);

#endif /* OS_SEMAPHORE_H_ */
