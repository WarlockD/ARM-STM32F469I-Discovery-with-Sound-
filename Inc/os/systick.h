#ifndef PLATFORM_STM32F4_SYSTICK_H_
#define PLATFORM_STM32F4_SYSTICK_H_


#include <stdint.h>
#include <sys\time.h>
//#define CORE_CLOCK		(0x0a037a00) /* 168MHz */
#define SYSTICK_MAXRELOAD	(0x00ffffff)

void init_systick(uint32_t tick_reload, uint32_t tick_next_reload);
void systick_disable(void);
uint32_t systick_now(void);
uint32_t systick_flag_count(void);
void init_systick_tm(struct timeval* reload, struct timeval* next_reload);
void systick_cfg();

#endif
