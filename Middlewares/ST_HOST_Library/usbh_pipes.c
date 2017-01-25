/**
  ******************************************************************************
  * @file    usbh_pipes.c
  * @author  MCD Application Team
  * @version V3.2.2
  * @date    07-July-2015
  * @brief   This file implements functions for opening and closing Pipes
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "usbh_pipes.h"
#include <assert.h>




/**
  * @brief  USBH_Open_Pipe
  *         Open a  pipe
  * @param  phost: Host Handle
  * @param  pipe_num: Pipe Number
  * @param  dev_address: USB Device address allocated to attached device
  * @param  speed : USB device speed (Full/Low)
  * @param  ep_type: end point type (Bulk/int/ctl)
  * @param  mps: max pkt size
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_OpenPipe  (USBH_HandleTypeDef *phost,USBH_PipeHandleTypeDef* handle)
{
	// fifo_init_static(&handle->fifo,handle->Data, USBH_MAX_DATA_BUFFER);
	assert(handle && handle->state == PIPE_IDLE);
  USBH_LL_OpenPipe(phost,
		  handle->Pipe,
		  handle->Init.EpNumber,
		  handle->Init.Address,
		  handle->Init.Speed,
		  handle->Init.EpType,
		  handle->Init.PacketSize);
  return USBH_OK; 

}

/**
  * @brief  USBH_ClosePipe
  *         Close a  pipe
  * @param  phost: Host Handle
  * @param  pipe_num: Pipe Number
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_ClosePipe  (USBH_HandleTypeDef *phost,USBH_PipeHandleTypeDef* handle)
{
	assert(handle && handle->state == PIPE_IDLE);
  USBH_LL_ClosePipe(phost, handle->Pipe);
  return USBH_OK; 

}

/**
  * @brief  USBH_Alloc_Pipe
  *         Allocate a new Pipe
  * @param  phost: Host Handle
  * @param  ep_addr: End point for which the Pipe to be allocated
  * @retval Pipe number
  */
USBH_PipeHandleTypeDef* USBH_AllocPipe  (USBH_HandleTypeDef *phost, void** owner)
{
	USBH_PipeHandleTypeDef* pipe = NULL;
	for (uint8_t idx = 0 ; idx < 11 ; idx++)
		  if(phost->Pipes[idx].state == PIPE_NOTALLOCATED) {
			  USBH_PipeHandleTypeDef* pipe = &phost->Pipes[idx];
			  pipe->Pipe = idx;
			  if(pipe->Init.Direction)
			  if(owner) {
				  *owner = pipe;
				  pipe->owner = owner;
			  }
			  break;
		  }
	assert(pipe);
	return pipe;
}

/**
  * @brief  USBH_Free_Pipe
  *         Free the USB Pipe
  * @param  phost: Host Handle
  * @param  idx: Pipe number to be freed 
  * @retval USBH Status
  */
void USBH_FreePipe  (USBH_HandleTypeDef *phost, USBH_PipeHandleTypeDef* handle)
{
	if(handle) {
		assert(handle->state == PIPE_IDLE);
		if(handle->owner) { *handle->owner = NULL;handle->owner=NULL; }
		handle->state = PIPE_NOTALLOCATED;
		handle->Pipe = 0xFF;

		// all you have to do
	}
}

USBH_StatusTypeDef USBH_ProcessPipe(USBH_HandleTypeDef *phost,USBH_PipeHandleTypeDef* handle){
	assert(handle->state != PIPE_NOTALLOCATED);
	if(phost->port_state == HOST_PORT_ACTIVE) return USBH_BUSY;
	if(phost->port_state != HOST_PORT_IDLE) return USBH_FAIL;// fail on bad state
	switch(handle->state){
	case PIPE_COMPLETE:
			phost->port_state = HOST_PORT_IDLE;
			return handle->urb_state == USBH_URB_DONE ? USBH_OK : USBH_FAIL;
	case PIPE_IDLE:
		phost->port_state = HOST_PORT_ACTIVE;
		// if we are not an intruppt and sending data, do a ping
		handle->urb_state = USBH_URB_IDLE;
		handle->state = PIPE_WORKING;
		uint8_t do_ping = (handle->Init.Speed == USBH_SPEED_HIGH && handle->Init.Direction == PIPE_DIR_OUT)  ? 1 : 0;
		USBH_LL_SubmitURB (phost,
			handle->Pipe,             // Pipe index
			handle->Init.Direction,     // Direction : OUT
			handle->Init.EpType,		// EP type
			handle->Init.DataType,
			handle->Data,
			handle->Size,
			do_ping); /* do ping (HS Only)*/
		// no break
	case PIPE_WORKING: return USBH_BUSY; break;
	case PIPE_NOTALLOCATED:
			assert(handle->state != PIPE_NOTALLOCATED);
			// no break
	default: return USBH_FAIL;
	}
}
/**
  * @brief  USBH_GetFreePipe
  * @param  phost: Host Handle
  *         Get a free Pipe number for allocation to a device endpoint
  * @retval idx: Free Pipe number
  */

/**
* @}
*/ 

/**
* @}
*/ 

/**
* @}
*/

/**
* @}
*/ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/


