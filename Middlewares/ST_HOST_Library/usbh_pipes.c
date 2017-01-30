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
#include "usbh_ioreq.h"
#include "stm32469i_discovery.h"
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
	assert(handle);
	if(handle->state != PIPE_IDLE){
		printf("Something is weird %i\r\n", handle->state);
		assert(handle->state == PIPE_IDLE);
	}
	uint8_t ep_type = EP_TYPE_CTRL;
	switch(handle->Init.Type){
	case PIPE_SETUP:
	case PIPE_CONTROL_IN:
	case PIPE_CONTROL_OUT:
		ep_type = EP_TYPE_CTRL;
		break;
	case PIPE_ISO_IN:
	case PIPE_ISO_OUT:
		ep_type = EP_TYPE_ISOC;
		break;
	case PIPE_BULK_IN:
	case PIPE_BULK_OUT:
		ep_type = EP_TYPE_BULK;
		break;
	case PIPE_INTERRUPT_IN:
	case PIPE_INTERRUPT_OUT:
		ep_type = EP_TYPE_INTR;
		break;
  }
  USBH_LL_OpenPipe(phost,
		  handle->Pipe,
		  handle->Init.EpNumber,
		  handle->Init.Address,
		  handle->Init.Speed,
		  ep_type,
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
			  pipe = &phost->Pipes[idx];
			  memset(pipe, 0, sizeof(USBH_PipeHandleTypeDef));
			  pipe->state = PIPE_IDLE;
			  pipe->Pipe = idx;
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

#define ENUM_TO_STRING_START(TYPE) \
		ENUM_TO_STRING_DEFINE(TYPE) { \
		static const char* tbl[] = {

#define ENUM_TO_STRING_ADD(ENUM) #ENUM,

#define ENUM_TO_STRING_END }; return tbl[(uint8_t)t]; }
ENUM_TO_STRING_START(USBH_PipeDataTypeDef)
	ENUM_TO_STRING_ADD(PIPE_SETUP)
	ENUM_TO_STRING_ADD(PIPE_CONTROL_IN)
	ENUM_TO_STRING_ADD(PIPE_CONTROL_OUT)
	ENUM_TO_STRING_ADD(PIPE_ISO_IN)
	ENUM_TO_STRING_ADD(PIPE_ISO_OUT)
	ENUM_TO_STRING_ADD(PIPE_BULK_IN)
	ENUM_TO_STRING_ADD(PIPE_BULK_OUT)
	ENUM_TO_STRING_ADD(PIPE_INTERRUPT_IN)
	ENUM_TO_STRING_ADD(PIPE_INTERRUPT_OUT)
ENUM_TO_STRING_END

ENUM_TO_STRING_START(USBH_SpeedTypeDef)
	ENUM_TO_STRING_ADD(USBH_SPEED_HIGH)
	ENUM_TO_STRING_ADD(USBH_SPEED_FULL)
	ENUM_TO_STRING_ADD(USBH_SPEED_LOW)
ENUM_TO_STRING_END

void USBH_PipeInitDebug(USBH_PipeInitTypeDef* handle){
	USBH_DbgLog("type=%s speed=%s  address=%i pkgsize=%i epnum=%i"
			, ENUM_TO_STRING(USBH_PipeDataTypeDef,handle->Type)
			, ENUM_TO_STRING(USBH_SpeedTypeDef,handle->Speed)
			, (int)handle->Address
			, (int)handle->PacketSize
			, (int)handle->EpNumber
	);
}
void USBH_PipeDebug(USBH_PipeHandleTypeDef* handle){

}
void USBH_LL_NotifyURBChange_Callback(USBH_HandleTypeDef*phost , uint8_t pipe ,  USBH_URBStateTypeDef state){
	USBH_PipeHandleTypeDef* ptr = &phost->Pipes[pipe];
	if(ptr->state == PIPE_WORKING){
		BSP_LED_Off(LED_ORANGE);
		ptr->urb_state = state;
		ptr->state = PIPE_IDLE;
		//uart_print("Pipe(%i,%s)\r\n",pipe, URBStateToString(state));
		//if(ptr->Callback) ptr->Callback(phost,pipe);
	} else { // the fuck?
		uart_print("Pipe not working?(%i,%s)\r\n",pipe, URBStateToString(state));

	}
}
static void SubmitPipe(USBH_HandleTypeDef *phost,USBH_PipeHandleTypeDef* handle) {
	handle->state = PIPE_WORKING;
	BSP_LED_On(LED_ORANGE);
	handle->urb_state = USBH_URB_IDLE;
	USBH_DbgLog("SubmitPipe(%i)", handle->Pipe);
	USBH_PipeInitDebug(&handle->Init);
	switch(handle->Init.Type){
	case PIPE_SETUP:
		USBH_CtlSendSetup(phost,handle->Data,handle->Pipe);
		break;
	case PIPE_CONTROL_IN:
		//USBH_CtlSendData(phost,handle->Data,handle->Size,handle->Pipe,1);
		USBH_CtlReceiveData(phost,handle->Data,handle->Size,handle->Pipe);
		break;
	case PIPE_CONTROL_OUT:
		USBH_CtlSendData(phost,handle->Data,handle->Size,handle->Pipe,0);
		break;
	case PIPE_ISO_IN:
		USBH_IsocReceiveData(phost,handle->Data,handle->Size,handle->Pipe);
			break;
	case PIPE_ISO_OUT:
		USBH_IsocSendData(phost,handle->Data,handle->Size,handle->Pipe);
			break;
	case PIPE_BULK_IN:
		USBH_BulkReceiveData(phost,handle->Data,handle->Size,handle->Pipe);
			break;
	case PIPE_BULK_OUT:
		USBH_BulkSendData(phost,handle->Data,handle->Size,handle->Pipe,0);
			break;
	case PIPE_INTERRUPT_IN:
		USBH_InterruptReceiveData(phost,handle->Data,handle->Size,handle->Pipe);
			break;
	case PIPE_INTERRUPT_OUT:
		USBH_InterruptSendData(phost,handle->Data,handle->Size,handle->Pipe);
			break;
	}
#if 0
	uint8_t do_ping = (handle->Init.Speed == USBH_SPEED_HIGH && handle->Init.Direction == PIPE_DIR_OUT)  ? 1 : 0;
	USBH_LL_SubmitURB (phost,
		handle->Pipe,             // Pipe index
		handle->Init.Direction,     // Direction : OUT
		handle->Init.EpType,		// EP type
		handle->Init.DataType,
		handle->Data,
		handle->Size,
		do_ping); /* do ping (HS Only)*/
#endif
}


USBH_StatusTypeDef USBH_PipeStartup(USBH_HandleTypeDef *phost,USBH_PipeHandleTypeDef* handle){
	assert(handle->state != PIPE_NOTALLOCATED);
	assert(handle->state == PIPE_IDLE);
	SubmitPipe(phost,handle);
}

USBH_StatusTypeDef USBH_PipeStatus(USBH_HandleTypeDef *phost,USBH_PipeHandleTypeDef* handle){
	assert(handle->state != PIPE_NOTALLOCATED);
	switch(handle->state){
	case PIPE_IDLE:
		return handle->urb_state == USBH_URB_DONE ? USBH_OK : USBH_FAIL;
	case PIPE_WORKING: return USBH_BUSY; break;
	default:
		return USBH_FAIL;
	}
}
void BlockingChecker(USBH_HandleTypeDef *phost,USBH_PipeHandleTypeDef* handle) {
	USBH_URBStateTypeDef start = USBH_LL_GetURBState(phost, handle->Pipe);
	USBH_DbgLog("BlockingChecker Start(%i): %s",phost->Timer , URBStateToString(start));
	while(1){
		USBH_URBStateTypeDef next = USBH_LL_GetURBState(phost, handle->Pipe);
		if(next != start) {
			USBH_DbgLog("BlockingChecker(%i) %s -> %s",phost->Timer , URBStateToString(start),URBStateToString(next));
			start = next;
		}
		assert(next != USBH_URB_ERROR);
		if(next == USBH_URB_DONE) return;
	}
}


USBH_StatusTypeDef USBH_PipeWait(USBH_HandleTypeDef *phost,USBH_PipeHandleTypeDef* handle) {
	assert(handle->state != PIPE_NOTALLOCATED);
	assert(handle->state == PIPE_IDLE);
	SubmitPipe(phost,handle);
	//HAL_Delay(1); // we wait a millsecond
	BlockingChecker(phost,handle);
	//while(handle->state != PIPE_IDLE);
	if(handle->urb_state == USBH_URB_DONE) return USBH_OK;
	USBH_ErrLog("USBH_PipeWait Error: %s", URBStateToString(handle->urb_state));
	assert(handle->urb_state == USBH_URB_DONE);
	return USBH_FAIL;
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


