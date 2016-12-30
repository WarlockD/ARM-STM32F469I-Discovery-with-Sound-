/*
 * arm_atomic.h
 *
 *  Created on: Dec 28, 2016
 *      Author: Paul
 */

#ifndef ARM_ATOMIC_HPP_
#define ARM_ATOMIC_HPP_

#include "stm32f4xx_hal.h"

__attribute__((always_inline)) __STATIC_INLINE uint8_t atomic_test_and_set8(volatile uint8_t *addr){
	char result = __LDREXB(addr);
	result = 1;
	while(__STREXB(result,addr));
	return result;
}
__attribute__((always_inline)) __STATIC_INLINE void atomic_clear(volatile uint8_t *addr){
	while(__STREXB(0,addr));
}
class Semaphore
{
  enum { SemFree, SemTaken };
  // semaphore value
  uint32_t s;

public:
  // constructor
  Semaphore(): s(SemFree) {};

  // try to take the semaphore and return success
  // by default block until succeeded
  bool take(bool block = true)
  {
    int oldval;
    do {
      // read the semaphore value
      oldval = __LDREXW(&s);
      // loop again if it is locked and we are blocking
      // or setting it with strex failed
    }
    while ( (block && oldval == SemTaken) || __STREXW(SemTaken, &s) != 0 );
    if ( !block ) __CLREX(); // clear exclusive lock set by ldrex

    return oldval == SemFree;
  }


};

#endif /* ARM_ATOMIC_HPP_ */
