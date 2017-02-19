/* Copyright (c) 2014 The F9 Microkernel Project. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <f9_conf.h>

#include <platform/cortex_m.h>
#include <error.h>
#if 0
#include INC_PLAT(rcc.c)
#endif
#define HSE_STARTUP_TIMEOUT \
	(uint16_t) (0x0500)	/*!< Time out for HSE start up */

#if defined(STM32F4X)
	#define PLL_M	8	/*!< PLL_VCO = (HSE_VALUE or HSI_VALUE / PLL_M) * PLL_N */
	#define PLL_N	336
	#define PLL_P	2	/*!< SYSCLK = PLL_VCO / PLL_P */
	#define PLL_Q	7	/*!< USB OTG FS, SDIO and RNG Clock = PLL_VCO / PLLQ */

static __USER_DATA uint8_t APBAHBPrescTable[16] = {0, 0, 0, 0, 1, 2, 3, 4, 1, 2, 3, 4, 6, 7, 8, 9};
#elif defined(STM32F1X)
	#define PLL_MUL	6
#endif

/* RCC Flag Mask */
#define FLAG_MASK                 ((uint8_t)0x1F)


void sys_clock_init(void)
{

	SCB->SHCSR |= SCB_SHCSR_BUSFAULTENA;
	SCB->SHCSR |= SCB_SHCSR_USEFAULTENA;
}

#if defined(STM32F4X)
void __USER_TEXT RCC_AHB1PeriphClockCmd(uint32_t rcc_AHB1, uint8_t enable)
{
	/* TODO: assertion */

	if (enable != 0)
		RCC->AHB1ENR |= rcc_AHB1;
	else
		RCC->AHB1ENR &= ~rcc_AHB1;

#elif defined(STM32F1X)
void __USER_TEXT RCC_AHBPeriphClockCmd(uint32_t rcc_AHB, uint8_t enable)
{
	/* TODO: assertion */

	if (enable != 0)
		RCC->AHBENR |= rcc_AHB;
	else
		RCC->AHBENR &= ~rcc_AHB;

#endif
}

#if defined(STM32F4X)
void __USER_TEXT RCC_AHB1PeriphResetCmd(uint32_t rcc_AHB1, uint8_t enable)
{
	/* TODO: assertion */

	if (enable != 0)
		RCC->AHB1RSTR |= rcc_AHB1;
	else
		RCC->AHB1RSTR &= ~rcc_AHB1;

#elif defined(STM32F1X)
void __USER_TEXT RCC_AHBPeriphResetCmd(uint32_t rcc_AHB, uint8_t enable)
{
	/* TODO: assertion */

	if (enable != 0)
		RCC->AHBRSTR |= rcc_AHB;
	else
		RCC->AHBRSTR &= ~rcc_AHB;

#endif
}

void __USER_TEXT RCC_APB1PeriphClockCmd(uint32_t rcc_APB1, uint8_t enable)
{
	/* TODO: assertion */

	if (enable != 0)
		RCC->APB1ENR |= rcc_APB1;
	else
		RCC->APB1ENR &= ~rcc_APB1;
}

void __USER_TEXT RCC_APB1PeriphResetCmd(uint32_t rcc_APB1, uint8_t enable)
{
	/* TODO: assertion */

	if (enable != 0)
		RCC->APB1RSTR |= rcc_APB1;
	else
		RCC->APB1RSTR &= ~rcc_APB1;
}

void __USER_TEXT RCC_APB2PeriphClockCmd(uint32_t rcc_APB2, uint8_t enable)
{
	/* TODO: assertion */

	if (enable != 0)
		RCC->APB2ENR |= rcc_APB2;
	else
		RCC->APB2ENR &= ~rcc_APB2;
}

void __USER_TEXT RCC_APB2PeriphResetCmd(uint32_t rcc_APB2, uint8_t enable)
{
	/* TODO: assertion */

	if (enable != 0)
		RCC->APB2RSTR |= rcc_APB2;
	else
		RCC->APB2RSTR &= ~rcc_APB2;
}

uint8_t __USER_TEXT RCC_GetFlagStatus(uint8_t flag)
{
	uint32_t tmp = 0;
	uint32_t statusreg = 0;
	uint8_t bitstatus = 0;

	/* TODO: assertion */

	tmp = flag >> 5;
	if (tmp == 1)
		statusreg = RCC->CR;
	else if (tmp == 2)
		statusreg = RCC->BDCR;
	else
		statusreg = RCC->CSR;

	tmp = flag & FLAG_MASK;
	if ((statusreg & ((uint32_t)1 << tmp)) != (uint32_t)0)
		bitstatus = 1;
	else
		bitstatus = 0;

	return bitstatus;
}

