/* Copyright (c) 2014 The F9 Microkernel Project. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <f9_conf.h>

#include <error.h>
#include <assert.h>

#if 0
#include INC_PLAT(gpio.c)

static GPIO_TypeDef * gpio_ports[] = {
		GPIOA,
		GPIOB,
		GPIOC,
		GPIOD,
		GPIOE,
		GPIOF,
		GPIOG,
		GPIOH,
		GPIOI,
		GPIOJ,
		GPIOK
};


void __USER_TEXT gpio_config(struct gpio_cfg *cfg)
{
	uint8_t port, pin, cfg_data;
	GPIO_InitTypeDef init;
	init.Alternate = cfg->
	port = cfg->port;
	pin = cfg->pin;

	*RCC_AHB1ENR |= (1 << port);

	/* pupd */
	cfg_data = cfg->pupd;
	gpio_pupdr(port, pin, cfg_data);

	/* mode type */
	cfg_data = cfg->type;
	gpio_moder(port, pin, cfg_data);

	if (cfg_data == GPIO_MODER_IN)
		return;

	/* Alternative function */
	if (cfg_data == GPIO_MODER_ALT) {
		uint8_t func = cfg->func;
		gpio_afr(port, pin, func);
	}

	/* Sets pin output type */
	cfg_data = cfg->o_type;
	gpio_otyper(port, pin, cfg_data);

	/* Speed */
	cfg_data = cfg->speed;
	gpio_ospeedr(port, pin, cfg_data);
}

void __USER_TEXT gpio_config_output(uint8_t port, uint8_t pin, uint8_t pupd, uint8_t speed)
{
	struct  gpio_cfg cfg = {
		.port = port,
		.pin = pin,
		.pupd = pupd,
		.type = GPIO_MODER_OUT,
		.func = 0,
		.o_type = GPIO_OTYPER_PP,
		.pupd = pupd,
		.speed = speed,
	};

	*RCC_AHB1ENR |= (1 << port);
	gpio_config(&cfg);
}

void __USER_TEXT gpio_config_input(uint8_t port, uint8_t pin, uint8_t pupd)
{
	struct  gpio_cfg cfg = {
		.port = port,
		.pin = pin,
		.pupd = pupd,
		.type = GPIO_MODER_IN,
		.func = 0,
		.o_type = 0,
		.pupd = pupd,
		.speed = 0,
	};

	*RCC_AHB1ENR |= (1 << port);
	gpio_config(&cfg);
}

void __USER_TEXT gpio_out_high(uint8_t port, uint8_t pin)
{
	*GPIO_ODR(port) |= (1 << pin);
}

void __USER_TEXT gpio_out_low(uint8_t port, uint8_t pin)
{
	*GPIO_ODR(port) &= ~(1 << pin);
}

uint8_t __USER_TEXT gpio_input_bit(uint8_t port, uint8_t pin)
{
	if (*GPIO_IDR(port) & (1 << pin))
		return 1;

	return 0;
}

void __USER_TEXT gpio_writebit(uint8_t port, uint8_t pin, uint8_t bitval)
{
	if (bitval != 0)
		*GPIO_BSRR(port) = GPIO_BSRR_BS(pin);
	else
		*GPIO_BSRR(port) = GPIO_BSRR_BR(pin);
}
#endif
