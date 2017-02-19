/* Copyright (c) 2014 The F9 Microkernel Project. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <f9_conf.h>

#if 0
#include INC_PLAT(hwtimer.c)
#endif

TIM_HandleTypeDef        hndle_hwtimer ;

void hwtimer_init()
{
	// enable clock	RCC->APB1ENR |= 0x00000001;
	//  TIM2->PSC = 0;
	//  TIM2->ARR = 0xFFFFFFFF;
	//  TIM2->CR1 = TIM_CR1_CEN; // counter enable
	__HAL_RCC_TIM2_CLK_ENABLE();
	hndle_hwtimer.Instance = TIM2;
	hndle_hwtimer.Init.Period = 0xFFFFFFFF;
	hndle_hwtimer.Init.Prescaler = 0;
	hndle_hwtimer.Init.ClockDivision = 0;
	hndle_hwtimer.Init.CounterMode = TIM_COUNTERMODE_UP;
	HAL_TIM_Base_Init(&hndle_hwtimer);
	__HAL_TIM_ENABLE(&hndle_hwtimer);

}

uint32_t hwtimer_now()
{
	return __HAL_TIM_GET_COUNTER(&hndle_hwtimer);
}
