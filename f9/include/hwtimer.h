#ifndef PLATFORM_STM32F4_HWTIMER_H_
#define PLATFORM_STM32F4_HWTIMER_H_

#include <f9_conf.h>
#include <stdint.h>

void hwtimer_init(void);
uint32_t hwtimer_now(void);

#endif /* PLATFORM_STM32F4_HWTIMER_H_ */
