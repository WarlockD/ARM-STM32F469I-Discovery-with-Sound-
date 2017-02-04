/** 
  ******************************************************************************
  * @file    usbh_ioreq.c 
  * @author  MCD Application Team
  * @version V3.2.2
  * @date    07-July-2015
  * @brief   This file handles the issuing of the USB transactions
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

#include "usb_host.h"
#include "stm32f4xx_hal.h"
extern HCD_HandleTypeDef hhcd;
/**
  * @brief  USBH_CtlSendSetup
  *         Sends the Setup Packet to the Device
  * @param  phost: Host Handle
  * @param  buff: Buffer pointer from which the Data will be send to Device
  * @param  pipe_num: Pipe Number
  * @retval USBH Status
  */

void USBH_CtlSendSetup (uint8_t *buff, uint8_t pipe_num)
{

	HAL_HCD_HC_SubmitRequest (&hhcd,                     /* Driver handle    */
                          pipe_num,             /* Pipe index       */
                          0,                    /* Direction : OUT  */
						  EP_TYPE_CTRL,      /* EP type          */
                          0,       /* Type setup       */
                          buff,                 /* data buffer      */
                          8,  /* data length      */
                          0);
}


/**
  * @brief  USBH_CtlSendData
  *         Sends a data Packet to the Device
  * @param  phost: Host Handle
  * @param  buff: Buffer pointer from which the Data will be sent to Device
  * @param  length: Length of the data to be sent
  * @param  pipe_num: Pipe Number
  * @retval USBH Status
  */
void USBH_CtlSendData (uint8_t *buff, uint16_t length, uint8_t pipe_num, uint8_t do_ping )
{
  if(hhcd.hc[pipe_num].speed != HCD_SPEED_HIGH)  do_ping = 0;
  HAL_HCD_HC_SubmitRequest (&hhcd,                     /* Driver handle    */
                          pipe_num,             /* Pipe index       */
                          0,                    /* Direction : OUT  */
						  EP_TYPE_CTRL,      /* EP type          */
                          1,        /* Type Data        */
                          buff,                 /* data buffer      */
                          length,               /* data length      */ 
                          do_ping);             /* do ping (HS Only)*/
  
}


/**
  * @brief  USBH_CtlReceiveData
  *         Receives the Device Response to the Setup Packet
  * @param  phost: Host Handle
  * @param  buff: Buffer pointer in which the response needs to be copied
  * @param  length: Length of the data to be received
  * @param  pipe_num: Pipe Number
  * @retval USBH Status. 
  */
void USBH_CtlReceiveData(uint8_t* buff,  uint16_t length, uint8_t pipe_num)
{
	HAL_HCD_HC_SubmitRequest (&hhcd,                     /* Driver handle    */
                          pipe_num,             /* Pipe index       */
                          1,                    /* Direction : IN   */
						  EP_TYPE_CTRL,      /* EP type          */
                          1,        /* Type Data        */
                          buff,                 /* data buffer      */
                          length,               /* data length      */ 
                          0);
}


/**
  * @brief  USBH_BulkSendData
  *         Sends the Bulk Packet to the device
  * @param  phost: Host Handle
  * @param  buff: Buffer pointer from which the Data will be sent to Device
  * @param  length: Length of the data to be sent
  * @param  pipe_num: Pipe Number
  * @retval USBH Status
  */

void USBH_BulkSendData (uint8_t *buff, uint16_t length, uint8_t pipe_num,uint8_t do_ping )
{ 
	if(hhcd.hc[pipe_num].speed != HCD_SPEED_HIGH)  do_ping = 0;
  
  HAL_HCD_HC_SubmitRequest (&hhcd,                     /* Driver handle    */
                          pipe_num,             /* Pipe index       */
                          0,                    /* Direction : IN   */
						  EP_TYPE_BULK,         /* EP type          */
                          1,        /* Type Data        */
                          buff,                 /* data buffer      */
                          length,               /* data length      */  
                          do_ping);             /* do ping (HS Only)*/
}


/**
  * @brief  USBH_BulkReceiveData
  *         Receives IN bulk packet from device
  * @param  phost: Host Handle
  * @param  buff: Buffer pointer in which the received data packet to be copied
  * @param  length: Length of the data to be received
  * @param  pipe_num: Pipe Number
  * @retval USBH Status. 
  */

void USBH_BulkReceiveData(uint8_t *buff, uint16_t length, uint8_t pipe_num)
{
	HAL_HCD_HC_SubmitRequest (&hhcd,                     /* Driver handle    */
                          pipe_num,             /* Pipe index       */
                          1,                    /* Direction : IN   */
						  EP_TYPE_BULK,         /* EP type          */
                          1,        /* Type Data        */
                          buff,                 /* data buffer      */
                          length,               /* data length      */  
                          0);
}


/**
  * @brief  USBH_InterruptReceiveData
  *         Receives the Device Response to the Interrupt IN token
  * @param  phost: Host Handle
  * @param  buff: Buffer pointer in which the response needs to be copied
  * @param  length: Length of the data to be received
  * @param  pipe_num: Pipe Number
  * @retval USBH Status. 
  */
void USBH_InterruptReceiveData(uint8_t *buff,  uint8_t length, uint8_t pipe_num)
{
	HAL_HCD_HC_SubmitRequest (&hhcd,                     /* Driver handle    */
                          pipe_num,             /* Pipe index       */
                          1,                    /* Direction : IN   */
						  EP_TYPE_INTR,    /* EP type          */
                          1,        /* Type Data        */
                          buff,                 /* data buffer      */
                          length,               /* data length      */  
                          0); 
}

/**
  * @brief  USBH_InterruptSendData
  *         Sends the data on Interrupt OUT Endpoint
  * @param  phost: Host Handle
  * @param  buff: Buffer pointer from where the data needs to be copied
  * @param  length: Length of the data to be sent
  * @param  pipe_num: Pipe Number
  * @retval USBH Status. 
  */

void USBH_InterruptSendData(uint8_t *buff,  uint8_t length, uint8_t pipe_num)
{
	HAL_HCD_HC_SubmitRequest (&hhcd,                     /* Driver handle    */
                          pipe_num,             /* Pipe index       */
                          0,                    /* Direction : OUT   */
						  EP_TYPE_INTR,    /* EP type          */
                          1,        /* Type Data        */
                          buff,                 /* data buffer      */
                          length,               /* data length      */  
                          0);  
}

/**
  * @brief  USBH_IsocReceiveData
  *         Receives the Device Response to the Isochronous IN token
  * @param  phost: Host Handle
  * @param  buff: Buffer pointer in which the response needs to be copied
  * @param  length: Length of the data to be received
  * @param  pipe_num: Pipe Number
  * @retval USBH Status. 
  */
void USBH_IsocReceiveData(uint8_t *buff, uint32_t length, uint8_t pipe_num)
{    
	HAL_HCD_HC_SubmitRequest (&hhcd,                     /* Driver handle    */
                          pipe_num,             /* Pipe index       */
                          1,                    /* Direction : IN   */
						  EP_TYPE_ISOC,          /* EP type          */
                          1,        /* Type Data        */
                          buff,                 /* data buffer      */
                          length,               /* data length      */
                          0);
}

/**
  * @brief  USBH_IsocSendData
  *         Sends the data on Isochronous OUT Endpoint
  * @param  phost: Host Handle
  * @param  buff: Buffer pointer from where the data needs to be copied
  * @param  length: Length of the data to be sent
  * @param  pipe_num: Pipe Number
  * @retval USBH Status. 
  */
void USBH_IsocSendData(uint8_t *buff, uint32_t length, uint8_t pipe_num)
{
	HAL_HCD_HC_SubmitRequest (&hhcd,                     /* Driver handle    */
                          pipe_num,             /* Pipe index       */
                          0,                    /* Direction : OUT   */
						  EP_TYPE_ISOC,          /* EP type          */
                          1,        /* Type Data        */
                          buff,                 /* data buffer      */
                          length,               /* data length      */ 
                          0);
}
