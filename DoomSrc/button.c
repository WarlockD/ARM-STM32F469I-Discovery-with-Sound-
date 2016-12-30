/*
 * button.c
 *
 *  Created on: 15.02.2015
 *      Author: Florian
 */


/*---------------------------------------------------------------------*
 *  include files                                                      *
 *---------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx.h"
#include "button.h"
#include "stm32469i_discovery.h"
/*---------------------------------------------------------------------*
 *  local definitions                                                  *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  external declarations                                              *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  public data                                                        *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  private data                                                       *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  private functions                                                  *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  public functions                                                   *
 *---------------------------------------------------------------------*/

void button_init (void)
{
	BSP_PB_Init(BUTTON_WAKEUP, BUTTON_MODE_GPIO);
}

bool button_read (void)
{
	return BSP_PB_GetState(BUTTON_WAKEUP) == PB_SET ?  true : false;
}

/*---------------------------------------------------------------------*
 *  eof                                                                *
 *---------------------------------------------------------------------*/
