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
#include "stm32f4xx_ll_usb.h"
#undef USBH_HIGH_SPEED_ENABLED

#if 0
typedef struct {
	uint32_t* data;
	size_t size;
	bool done;
} t_data_trasfer;

typedef struct {
	USBH_EndpointTypeDef ep;
	HCD_HCTypeDef * hc;
	uint8_t chan_num;
	uint8_t toggle_out;
	uint8_t toggle_in;
	uint8_t data_pid;
	uint8_t do_ping;
	t_data_trasfer data;
	bool enable;
} t_internal_endpoint;

t_internal_endpoint endpoints[16];

extern HCD_HandleTypeDef hhcd;


static t_internal_endpoint* FindFreePipe() {
	for(int i=1; i < 12;i++) {
		if(endpoints[i].ep.Device == NULL) {
			endpoints[i].chan_num = i;
			endpoints[i].hc = &hhcd.hc[i];
			endpoints[i].toggle_out = 0;
			endpoints[i].toggle_in = 0;
			endpoints[i].data_pid = 0;
			endpoints[i].do_ping = 0;
			endpoints[i].enable = false;
			return &endpoints[i];
		}
	}
	return NULL;
}


void  USBH_SubmitRequest(const USBH_EndpointTypeDef* ep, uint8_t* data, uint16_t length) {
	//assert(ep);
	//assert(ep->status == USBH_PIPE_IDLE);
	//SetEndPointStatus(ep, USBH_PIPE_WORKING);
	//HAL_HCD_HC_SubmitRequest (&hhcd,  ep->Channel,  ep->Direction , ep->Type ,1, data,length,ep->DoPing);
}

void  USBH_SubmitSetup(const USBH_EndpointTypeDef* ep, const USB_Setup_TypeDef* setup) {
	//assert(ep);
	//assert(setup);
	//assert(ep->status == USBH_PIPE_IDLE);
//	SetEndPointStatus(ep, USBH_PIPE_WORKING);
	//if(ep->Type != EP_TYPE_CTRL) {
	//	USBH_ErrLog("Cannot submit setup without a Control Out endpoint!");
	//}
//	HAL_HCD_HC_SubmitRequest(&hhcd,  ep->Channel,  0, EP_TYPE_CTRL ,0, (uint8_t*)setup, 8,0);
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
extern HCD_HandleTypeDef hhcd;

#if 0

#define HCINTMSK_CFG_CONTROL_BULK (USB_OTG_HCINTMSK_XFRCM | USB_OTG_HCINTMSK_STALLM | USB_OTG_HCINTMSK_TXERRM | USB_OTG_HCINTMSK_DTERRM | USB_OTG_HCINTMSK_AHBERR | USB_OTG_HCINTMSK_NAKM)
#define HCINTMSK_CFG_INTERRUPT  (USB_OTG_HCINTMSK_XFRCM  |USB_OTG_HCINTMSK_STALLM |USB_OTG_HCINTMSK_TXERRM |USB_OTG_HCINTMSK_DTERRM |USB_OTG_HCINTMSK_NAKM   |USB_OTG_HCINTMSK_AHBERR |	USB_OTG_HCINTMSK_FRMORM)
#define HCINTMSK_CFG_ISO (USB_OTG_HCINTMSK_XFRCM | USB_OTG_HCINTMSK_ACKM | USB_OTG_HCINTMSK_AHBERR | USB_OTG_HCINTMSK_FRMORM)
static const uint32_t HCINTMSK_CFG[6] = {
	/*USBH_EP_CONTROL_OUT	*/	HCINTMSK_CFG_CONTROL_BULK,
	/*USBH_EP_CONTROL_IN 	*/	USB_OTG_HCINTMSK_BBERRM|HCINTMSK_CFG_CONTROL_BULK ,
	/*USBH_EP_ISO_OUT 		*/	HCINTMSK_CFG_ISO,
	/*USBH_EP_ISO_IN 		*/	USB_OTG_HCINTMSK_TXERRM|USB_OTG_HCINTMSK_BBERRM|HCINTMSK_CFG_ISO,
	/*USBH_EP_BULK_OUT 		*/	HCINTMSK_CFG_CONTROL_BULK,
	/*USBH_EP_BULK_IN 		*/	USB_OTG_HCINTMSK_BBERRM|HCINTMSK_CFG_CONTROL_BULK,
	/*USBH_EP_INTERRUPT_OUT */	HCINTMSK_CFG_INTERRUPT,
	/*USBH_EP_INTERRUPT_IN  */  USB_OTG_HCINTMSK_BBERRM|HCINTMSK_CFG_INTERRUPT,
};
inline void InitEndpointType(t_internal_endpoint* iep){
	iep->hc->HCINT = 0xFFFFFFFFU; // Clear old interrupt conditions for this host channel.
	iep->hc->HCINTMSK = HCINTMSK_CFG[iep->ep.Type];
	if (iep->ep.Type == EP_TYPE_INTR)  iep->hc->HCCHAR |= USB_OTG_HCCHAR_ODDFRM ;
	// direction here
	if(iep->ep.Type & 1) iep->hc->HCCHAR |=  USB_OTG_HCCHAR_EPDIR; else iep->hc->HCCHAR &=  ~USB_OTG_HCCHAR_EPDIR;
#ifdef USBH_HIGH_SPEED_ENABLED
	if(hhcd->Instance  != USB_OTG_FS && ep_type == USBH_EP_CONTROL_OUT)
		iep->hc->HCINTMSK |= (USB_OTG_HCINTMSK_NYET | USB_OTG_HCINTMSK_ACKM);
#endif
}
void USBH_SetEndpointType(const USBH_EndpointTypeDef* ep, USBH_EpRequestTypeDef ep_type){
	t_internal_endpoint* iep = (t_internal_endpoint*)ep;
	if(iep->ep.Type != ep_type) InitEndpointType(iep,ep_type);
}
void _DisableEndpoint(USB_OTG_HostChannelTypeDef * hc,bool wait){
	hc->HCCHAR |= USB_OTG_HCCHAR_CHDIS;
	if(wait){
		uint32_t count = 0;
		hc->HCCHAR &= ~USB_OTG_HCCHAR_CHENA;
	    hc->HCCHAR |= USB_OTG_HCCHAR_CHENA;
		while ((++count < 1000U) && (hc->HCCHAR & USB_OTG_HCCHAR_CHENA) == USB_OTG_HCCHAR_CHENA);
	} else  hc->HCCHAR |= USB_OTG_HCCHAR_CHENA;
}
void USBH_SetEndpointEnable(const USBH_EndpointTypeDef* ep, bool enable){
	t_internal_endpoint* iep = (t_internal_endpoint*)ep;
	if(iep->enable != enable){
		if(enable) {
			InitEndpointType(ep);
			USBx_HOST->HAINTMSK |= (1 << iep->chan_num);
			USBx->GINTMSK |= USB_OTG_GINTMSK_HCIM;
		} else { // disable
			 uint32_t count = 0U;
			 uint8_t hc_type = ((iep->hc->HCCHAR) & USB_OTG_HCCHAR_EPTYP) >> 18;
			 if(hc_type == HCCHAR_CTRL || hc_type == HCCHAR_BULK )
				 _DisableEndpoint(iep->hc, (USBx->HNPTXSTS & 0xFF0000U) == 0U);
			 else
				 _DisableEndpoint(iep->hc, (USBx_HOST->HPTXSTS & 0xFFFFU) == 0U);
		}
	}
}
void USBH_RefreshEndpoint(const USBH_EndpointTypeDef* ep){
	t_internal_endpoint* iep = (t_internal_endpoint*)ep;
	uint32_t hcchar = iep->hc->HCCHAR & USB_OTG_HCCHAR_EPNUM & USB_OTG_HCCHAR_EPDIR &USB_OTG_HCCHAR_EPTYP;
	hcchar |= (((ep->Device->Address << 22U) & USB_OTG_HCCHAR_DAD) |
			 (((ep->Device->Speed == USB_OTG_SPEED_LOW)<< 17U) & USB_OTG_HCCHAR_LSDEV) |
			 (ep->Device->MaxPacketSize & USB_OTG_HCCHAR_MPSIZ));
	iep->hc->HCCHAR = hcchar;
}



const USBH_EndpointTypeDef* USBH_AttachFreePipe(USBH_DeviceTypeDef* device){
	t_internal_endpoint* iep = FindFreePipe();
	assert(iep);
	iep->ep.Device = device;
	iep->ep.Next = device->Endpoints;
	device->Endpoints = iep->ep.Next;
	InitEndpointType(USBH_EP_CONTROL_OUT); // we default set it to a control out pipe
	USBH_RefreshEndpoint(&iep->ep);
	return &iep->ep;
}


#endif

inline static _USBH_StartTrasfer( HCD_HCTypeDef* hc, uint8_t* data, uint16_t length) {
	hc->xfer_buff = data;
	hc->xfer_len  = length;
	hc->urb_state  = URB_IDLE;
	hc->xfer_count  = 0;
	hc->state  = HC_IDLE;
	return USB_HC_StartXfer(hhcd.Instance, hc, hhcd.Init.dma_enable);
}


void USBH_CtlSendSetup (uint8_t *buff, uint8_t pipe_num)
{
	HCD_HCTypeDef* hc = &hhcd.hc[pipe_num];
	HAL_HCD_HC_Init(&hhcd, pipe_num, 0x00, hc->dev_addr,EP_TYPE_CTRL, hc->speed,hc->max_packet);
	hc->ch_num = pipe_num;
	hc->data_pid = HC_PID_SETUP;
	_USBH_StartTrasfer(hc,buff,8);
#if 0
	HAL_HCD_HC_SubmitRequest (&hhcd,                     /* Driver handle    */
                          pipe_num,             /* Pipe index       */
                          0,                    /* Direction : OUT  */
						  EP_TYPE_CTRL,      /* EP type          */
                          0,       /* Type setup       */
                          buff,                 /* data buffer      */
                          8,  /* data length      */
                          0);
#endif
}

void USBH_CtlSendData(uint8_t *buff, uint16_t length, uint8_t pipe_num, uint8_t do_ping){
	HCD_HCTypeDef* hc = &hhcd.hc[pipe_num];
	HAL_HCD_HC_Init(&hhcd,pipe_num, 0x00, hc->dev_addr,EP_TYPE_CTRL, hc->speed,hc->max_packet);
	hc->ch_num = pipe_num;
	if (length == 0U) hc->toggle_out = 1U;  // For Status OUT stage, Length==0, Status Out PID = 1
	hc->data_pid = hc->toggle_out == 0U ? HC_PID_DATA0: HC_PID_DATA1;
    if(hc->urb_state  != URB_NOTREADY) hc->do_ping = do_ping;
    _USBH_StartTrasfer(hc,buff,length);
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
void USBH_CtlReceiveData(uint8_t* buff,  uint16_t length, uint8_t pipe_num) {
	HCD_HCTypeDef* hc = &hhcd.hc[pipe_num];
    HAL_HCD_HC_Init(&hhcd, pipe_num, 0x80, hc->dev_addr, EP_TYPE_CTRL, hc->speed,hc->max_packet);
	hc->ch_num = pipe_num;
	hc->data_pid = HC_PID_DATA1;
    _USBH_StartTrasfer(hc,buff,length);
#if 0
	HAL_HCD_HC_SubmitRequest (&hhcd,                     /* Driver handle    */
                          pipe_num,             /* Pipe index       */
                          1,                    /* Direction : IN   */
						  EP_TYPE_CTRL,      /* EP type          */
                          1,        /* Type Data        */
                          buff,                 /* data buffer      */
                          length,               /* data length      */ 
                          0);
#endif
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
#endif
