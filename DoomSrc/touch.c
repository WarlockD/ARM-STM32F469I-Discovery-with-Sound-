/*
 * touch.c
 *
 * Driver for STMPE811 touch sensor
 *
 *  Created on: 09.06.2014
 *      Author: Florian
 */


/*---------------------------------------------------------------------*
 *  include files                                                      *
 *---------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "stm32f4xx.h"
#include "stm32469i_discovery_ts.h"
#include "main.h"
#include "touch.h"

touch_state_t touch_state;

void touch_init (void)
{
	BSP_TS_Init(BSP_LCD_GetXSize(),BSP_LCD_GetYSize());

}

void touch_main (void)
{
	touch_read ();
#if 0
	if (int_received && ((systime - int_timestamp) > 3))
	{
		int_received = false;

		// clear interrupts
		i2c_write_byte (TOUCH_I2C_ADDR, TOUCH_REG_INT_STA, 0xFF);

		// read coordinates
		touch_read ();
	}
#endif
}

void touch_read (void)
{
	TS_StateTypeDef status;
	uint32_t x_diff, y_diff, x, y;
	static uint32_t _x = 0, _y = 0;
	BSP_TS_GetState(&status);

	touch_state.status = status.touchDetected ? TOUCH_PRESSED : TOUCH_RELEASED;

	if (touch_state.status == TOUCH_PRESSED)
	{
		//touch_read_xy (xy);

		x = status.touchX[0];
		y = status.touchY[0];

		x_diff = x > _x ? (x - _x) : (_x - x);
		y_diff = y > _y ? (y - _y) : (_y - y);

		if (x_diff + y_diff > 5)
		{
			_x = x;
			_y = y;
		}
	}

#ifdef LCD_UPSIDE_DOWN
	touch_state.x = LCD_MAX_X - _x;
	touch_state.y = LCD_MAX_Y - _y;
#else
	touch_state.x = _x;
	touch_state.y = _y;
#endif
}

uint8_t touch_read_temperature (void)
{
#if 0
	uint16_t temp;

	i2c_write_byte (TOUCH_I2C_ADDR, TOUCH_REG_TEMP_CTRL, 0x03);

	sleep_ms (10);

	temp = i2c_read_byte (TOUCH_I2C_ADDR, TOUCH_REG_TEMP_DATA) << 8;
	temp = temp | i2c_read_byte (TOUCH_I2C_ADDR, TOUCH_REG_TEMP_DATA + 1);

	printf ("%d\n", temp);
	return (uint8_t)(0); // is this correct?
#endif
	return 0;
}
// touch detection, skip

#if 0
void EXTI15_10_IRQHandler (void)
{
	if (EXTI_GetITStatus (EXTI_Line15) != RESET)
	{
		EXTI_ClearITPendingBit (EXTI_Line15);

		int_received = true;
		int_timestamp = systime;
	}
}
#endif
/*---------------------------------------------------------------------*
 *  eof                                                                *
 *---------------------------------------------------------------------*/
