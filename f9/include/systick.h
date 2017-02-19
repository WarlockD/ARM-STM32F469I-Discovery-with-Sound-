#ifndef __SYSTICK_H_
#define __SYSTICK_H_

#include <stdint.h>
#include <stddef.h>

#include <f9_conf.h>

#ifndef CORE_CLOCK
#error CORE CLOCK NEEDS TO BE DEFINED
#endif

#ifndef SYSTICK_MAXRELOAD
#error SYSTICK MAX RELOAD NEEDS TO BE DEFINED
#endif



void init_systick(uint32_t tick_reload, uint32_t tick_next_reload);
void systick_disable(void);
uint32_t systick_now(void);
uint32_t systick_flag_count(void);

#endif
