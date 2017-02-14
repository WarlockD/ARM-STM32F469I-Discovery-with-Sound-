/**
  ******************************************************************************
  * @file    usbh_ctlreq.c 
  * @author  MCD Application Team
  * @version V3.2.2
  * @date    07-July-2015
  * @brief   This file implements the control requests for device enumeration
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
#include "usbh_desc.h"
#include <assert.h>

#if 0
#define  USB_LEN_DESC_HDR                               0x02
#define  USB_LEN_DEV_DESC                               0x12
#define  USB_LEN_CFG_DESC                               0x09
#define  USB_LEN_IF_DESC                                0x09
#define  USB_LEN_EP_DESC                                0x07
#define  USB_LEN_OTG_DESC                               0x03
#define  USB_LEN_SETUP_PKT                              0x08

/* bmRequestType :D7 Data Phase Transfer Direction  */
#define  USB_REQ_DIR_MASK                               0x80
#define  USB_H2D                                        0x00
#define  USB_D2H                                        0x80

/* bmRequestType D6..5 Type */
#define  USB_REQ_TYPE_STANDARD                          0x00
#define  USB_REQ_TYPE_CLASS                             0x20
#define  USB_REQ_TYPE_VENDOR                            0x40
#define  USB_REQ_TYPE_RESERVED                          0x60

/* bmRequestType D4..0 Recipient */
#define  USB_REQ_RECIPIENT_DEVICE                       0x00
#define  USB_REQ_RECIPIENT_INTERFACE                    0x01
#define  USB_REQ_RECIPIENT_ENDPOINT                     0x02
#define  USB_REQ_RECIPIENT_OTHER                        0x03

/* Table 9-4. Standard Request Codes  */
/* bRequest , Value */
#define  USB_REQ_GET_STATUS                             0x00
#define  USB_REQ_CLEAR_FEATURE                          0x01
#define  USB_REQ_SET_FEATURE                            0x03
#define  USB_REQ_SET_ADDRESS                            0x05
#define  USB_REQ_GET_DESCRIPTOR                         0x06
#define  USB_REQ_SET_DESCRIPTOR                         0x07
#define  USB_REQ_GET_CONFIGURATION                      0x08
#define  USB_REQ_SET_CONFIGURATION                      0x09
#define  USB_REQ_GET_INTERFACE                          0x0A
#define  USB_REQ_SET_INTERFACE                          0x0B
#define  USB_REQ_SYNCH_FRAME                            0x0C

/* Table 9-5. Descriptor Types of USB Specifications */
#define  USB_DESC_TYPE_DEVICE                              1
#define  USB_DESC_TYPE_CONFIGURATION                       2
#define  USB_DESC_TYPE_STRING                              3
#define  USB_DESC_TYPE_INTERFACE                           4
#define  USB_DESC_TYPE_ENDPOINT                            5
#define  USB_DESC_TYPE_DEVICE_QUALIFIER                    6
#define  USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION           7
#define  USB_DESC_TYPE_INTERFACE_POWER                     8
#define  USB_DESC_TYPE_HID                                 0x21
#define  USB_DESC_TYPE_HID_REPORT                          0x22


#define USB_DEVICE_DESC_SIZE                               18
#define USB_CONFIGURATION_DESC_SIZE                        9
#define USB_HID_DESC_SIZE                                  9
#define USB_INTERFACE_DESC_SIZE                            9
#define USB_ENDPOINT_DESC_SIZE                             7

/* Descriptor Type and Descriptor Index  */
/* Use the following values when calling the function USBH_GetDescriptor  */
#define  USB_DESC_DEVICE                    ((USB_DESC_TYPE_DEVICE << 8) & 0xFF00)
#define  USB_DESC_CONFIGURATION             ((USB_DESC_TYPE_CONFIGURATION << 8) & 0xFF00)
#define  USB_DESC_STRING                    ((USB_DESC_TYPE_STRING << 8) & 0xFF00)
#define  USB_DESC_INTERFACE                 ((USB_DESC_TYPE_INTERFACE << 8) & 0xFF00)
#define  USB_DESC_ENDPOINT                  ((USB_DESC_TYPE_INTERFACE << 8) & 0xFF00)
#define  USB_DESC_DEVICE_QUALIFIER          ((USB_DESC_TYPE_DEVICE_QUALIFIER << 8) & 0xFF00)
#define  USB_DESC_OTHER_SPEED_CONFIGURATION ((USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION << 8) & 0xFF00)
#define  USB_DESC_INTERFACE_POWER           ((USB_DESC_TYPE_INTERFACE_POWER << 8) & 0xFF00)
#define  USB_DESC_HID_REPORT                ((USB_DESC_TYPE_HID_REPORT << 8) & 0xFF00)
#define  USB_DESC_HID                       ((USB_DESC_TYPE_HID << 8) & 0xFF00)


#define  USB_EP_TYPE_CTRL                               0x00
#define  USB_EP_TYPE_ISOC                               0x01
#define  USB_EP_TYPE_BULK                               0x02
#define  USB_EP_TYPE_INTR                               0x03

#define  USB_EP_DIR_OUT                                 0x00
#define  USB_EP_DIR_IN                                  0x80
#define  USB_EP_DIR_MSK                                 0x80

#ifndef USBH_MAX_PIPES_NBR
 #define USBH_MAX_PIPES_NBR                             15
#endif /* USBH_MAX_PIPES_NBR */

#define USBH_DEVICE_ADDRESS_DEFAULT                     0
#define USBH_MAX_ERROR_COUNT                            2
#define USBH_DEVICE_ADDRESS                             1

#define FEATURE_SELECTOR_ENDPOINT         0X00
#define FEATURE_SELECTOR_DEVICE           0X01


#define INTERFACE_DESC_TYPE               0x04
#define ENDPOINT_DESC_TYPE                0x05
#define INTERFACE_DESC_SIZE               0x09

typedef enum
{
  CTRL_IDLE =0,
  CTRL_SETUP,
  CTRL_SETUP_WAIT,
  CTRL_DATA_IN,
  CTRL_DATA_IN_WAIT,
  CTRL_DATA_OUT,
  CTRL_DATA_OUT_WAIT,
  CTRL_STATUS_IN,
  CTRL_STATUS_IN_WAIT,
  CTRL_STATUS_OUT,
  CTRL_STATUS_OUT_WAIT,
  CTRL_ERROR,
  CTRL_STALLED,
  CTRL_COMPLETE
}CTRL_StateTypeDef;

/* Following states are used for RequestState */
typedef enum
{
  CMD_IDLE =0,
  CMD_SEND,
  CMD_WAIT
} CMD_StateTypeDef;

static USBH_CtrlTypeDef Control ;
void InitControl() {
	Control.req_state = CMD_SEND;
	Control.state =CTRL_IDLE;
}
USBH_StatusTypeDef USBH_GetDescriptor(
                               uint8_t  req_type,
                               uint16_t value_idx,
							   USB_DESCRIPTOR* buff,
                               uint16_t length );

USBH_StatusTypeDef USBH_CtlReq(
                             uint8_t             *buff,
                             uint16_t            length);
/**
* @}
*/ 


/** @defgroup USBH_CTLREQ_Private_Functions
* @{
*/ 


/**
  * @brief  USBH_Get_DevDesc
  *         Issue Get Device Descriptor command to the device. Once the response 
  *         received, it parses the device descriptor and updates the status.
  * @param  phost: Host Handle
  * @param  length: Length of the descriptor
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_Get_DevDesc(USB_DEVICE_DESCRIPTOR* desc, uint8_t length)
{
  return USBH_GetDescriptor(USB_REQ_RECIPIENT_DEVICE | USB_REQ_TYPE_STANDARD,
                                  USB_DESC_DEVICE, 
                                  (USB_DESCRIPTOR*)desc,
                                  length);
}

/**
  * @brief  USBH_Get_CfgDesc
  *         Issues Configuration Descriptor to the device. Once the response 
  *         received, it parses the configuration descriptor and updates the 
  *         status.
  * @param  phost: Host Handle
  * @param  length: Length of the descriptor
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_Get_CfgDesc(USB_CONFIGURATION_DESCRIPTOR *desc,
                             uint16_t length)

{
  return USBH_GetDescriptor(USB_REQ_RECIPIENT_DEVICE | USB_REQ_TYPE_STANDARD,
                                  USB_DESC_CONFIGURATION, 
								  (USB_DESCRIPTOR*)desc,
                                  length);
}


/**
  * @brief  USBH_Get_StringDesc
  *         Issues string Descriptor command to the device. Once the response 
  *         received, it parses the string descriptor and updates the status.
  * @param  phost: Host Handle
  * @param  string_index: String index for the descriptor
  * @param  buff: Buffer address for the descriptor
  * @param  length: Length of the descriptor
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_Get_StringDesc(USB_STRING_DESCRIPTOR *desc,uint16_t length,
                                uint8_t string_index)
{
  return  USBH_GetDescriptor(USB_REQ_RECIPIENT_DEVICE | USB_REQ_TYPE_STANDARD,
                                  USB_DESC_STRING | string_index, 
								  (USB_DESCRIPTOR*)desc,
                                  length);
}

/**
  * @brief  USBH_GetDescriptor
  *         Issues Descriptor command to the device. Once the response received,
  *         it parses the descriptor and updates the status.
  * @param  phost: Host Handle
  * @param  req_type: Descriptor type
  * @param  value_idx: Value for the GetDescriptr request
  * @param  buff: Buffer to store the descriptor
  * @param  length: Length of the descriptor
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_GetDescriptor(
                               uint8_t  req_type,
                               uint16_t value_idx, 
							   USB_DESCRIPTOR* buff,
                               uint16_t length )
{ 
  if(Control.req_state == CMD_SEND)
  {
    Control.setup.b.bmRequestType = USB_D2H | req_type;
    Control.setup.b.bRequest = USB_REQ_GET_DESCRIPTOR;
    Control.setup.b.wValue.w = value_idx;
    
    if ((value_idx & 0xff00) == USB_DESC_STRING)
    {
      Control.setup.b.wIndex.w = 0x0409;
    }
    else
    {
      Control.setup.b.wIndex.w = 0;
    }
    Control.setup.b.wLength.w = length;
  }
  return USBH_CtlReq((uint8_t*)buff , length);
}

/**
  * @brief  USBH_SetAddress
  *         This command sets the address to the connected device
  * @param  phost: Host Handle
  * @param  DeviceAddress: Device address to assign
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_SetAddress(uint8_t DeviceAddress)
{
  if(Control.req_state == CMD_SEND)
  {
    Control.setup.b.bmRequestType = USB_H2D | USB_REQ_RECIPIENT_DEVICE | \
      USB_REQ_TYPE_STANDARD;
    
    Control.setup.b.bRequest = USB_REQ_SET_ADDRESS;
    
    Control.setup.b.wValue.w = (uint16_t)DeviceAddress;
    Control.setup.b.wIndex.w = 0;
    Control.setup.b.wLength.w = 0;
  }
  return USBH_CtlReq( 0 , 0 );
}

/**
  * @brief  USBH_SetCfg
  *         The command sets the configuration value to the connected device
  * @param  phost: Host Handle
  * @param  cfg_idx: Configuration value
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_SetCfg(uint16_t cfg_idx)
{
  if(Control.req_state == CMD_SEND)
  {
    Control.setup.b.bmRequestType = USB_H2D | USB_REQ_RECIPIENT_DEVICE |\
      USB_REQ_TYPE_STANDARD;
    Control.setup.b.bRequest = USB_REQ_SET_CONFIGURATION;
    Control.setup.b.wValue.w = cfg_idx;
    Control.setup.b.wIndex.w = 0;
    Control.setup.b.wLength.w = 0;
  }
  
  return USBH_CtlReq( 0 , 0 );
}

/**
  * @brief  USBH_SetInterface
  *         The command sets the Interface value to the connected device
  * @param  phost: Host Handle
  * @param  altSetting: Interface value
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_SetInterface(
                        uint8_t ep_num, uint8_t altSetting)
{
  
  if(Control.req_state == CMD_SEND)
  {
   Control.setup.b.bmRequestType = USB_H2D | USB_REQ_RECIPIENT_INTERFACE | \
      USB_REQ_TYPE_STANDARD;
    
   Control.setup.b.bRequest = USB_REQ_SET_INTERFACE;
    Control.setup.b.wValue.w = altSetting;
   Control.setup.b.wIndex.w = ep_num;
    Control.setup.b.wLength.w = 0;
  }
  return USBH_CtlReq( 0 , 0 );
}

/**
  * @brief  USBH_ClrFeature
  *         This request is used to clear or disable a specific feature.
  * @param  phost: Host Handle
  * @param  ep_num: endpoint number 
  * @param  hc_num: Host channel number 
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_ClrFeature(
                                   uint8_t ep_num) 
{
  if(Control.req_state == CMD_SEND)
  {
    Control.setup.b.bmRequestType = USB_H2D |
      USB_REQ_RECIPIENT_ENDPOINT |
        USB_REQ_TYPE_STANDARD;
    
    Control.setup.b.bRequest = USB_REQ_CLEAR_FEATURE;
    Control.setup.b.wValue.w = FEATURE_SELECTOR_ENDPOINT;
    Control.setup.b.wIndex.w = ep_num;
    Control.setup.b.wLength.w = 0;
  }
  return USBH_CtlReq( 0 , 0 );
}


/**
  * @brief  USBH_GetNextDesc 
  *         This function return the next descriptor header
  * @param  buf: Buffer where the cfg descriptor is available
  * @param  ptr: data pointer inside the cfg descriptor
  * @retval next header
  */
USB_DESCRIPTOR  *USBH_GetNextDesc (USB_DESCRIPTOR  *desc, uint16_t  *ptr)
{
  if(ptr) *ptr = *ptr+ desc->header.bLength;
  return (USB_DESCRIPTOR*)(((uint8_t*)desc)+desc->header.bLength);
}
typedef struct {
	USBH_CtrlType2TypeDef ctrl;
	CTRL_StateTypeDef state;
	void (*Callback)(const USBH_CtrlType2TypeDef* ctrl, void*);
	void* pData;
	uint8_t* data;
} t_internal_control;

static t_internal_control ctl_one = { {0 ,0, NULL,NULL } };
static void USBH_SubmitControlCallback( const USBH_EndpointTypeDef* ep, void* udata);

const USBH_CtrlType2TypeDef* USBH_OpenControl(uint8_t DevAddr, uint8_t MaxPacketSize){
	t_internal_control* ictrl = &ctl_one;
	USBH_CtrlType2TypeDef* ctl = &ictrl->ctrl;
	if(ctl->MaxPacketSize!=0) return NULL; // in use;
	ctl->status = USBH_PIPE_IDLE;
	ctl->urb_state = USBH_URB_IDLE;
	USBH_ModifyControl(ctl, DevAddr,MaxPacketSize);
	//USBH_ModifyEndpointCallback(ctl->ep_in, USBH_SubmitControlCallback,ictrl);
//	USBH_ModifyEndpointCallback(ctl->ep_out, USBH_SubmitControlCallback,ictrl);
	return ctl;
}
void USBH_ModifyControl(const USBH_CtrlType2TypeDef* cctl, uint8_t DevAddr, uint8_t MaxPacketSize){
	USBH_CtrlType2TypeDef* ctl = (USBH_CtrlType2TypeDef*)cctl;
	USBH_EndpointInitTypeDef Init;
	Init.Speed = USBH_SPEED_DEFAULT_PORT;
	Init.DevAddr =DevAddr;
	Init.Number = 0;
	Init.Type = USBH_EP_CONTROL_IN;
	Init.MaxPacketSize = MaxPacketSize;
	if(ctl->ep_in) USBH_ModifyEndpoint(ctl->ep_in, &Init);
	else ctl->ep_in = USBH_OpenEndpoint(&Init);
	Init.Type = USBH_EP_CONTROL_OUT;
	if(ctl->ep_out) USBH_ModifyEndpoint(ctl->ep_out, &Init);
	else ctl->ep_out = USBH_OpenEndpoint(&Init);
	ctl->DevAddr = DevAddr;
	ctl->MaxPacketSize = MaxPacketSize;
}

void USBH_CloseControl(const USBH_CtrlType2TypeDef* cctl){
	USBH_CtrlType2TypeDef* ctl = (USBH_CtrlType2TypeDef*)cctl;
	ctl->MaxPacketSize=0; // HACKY
}

static void USBH_SubmitControlCallback( const USBH_EndpointTypeDef* ep, void* udata){
	t_internal_control* ictrl = (t_internal_control*)udata;
	USBH_CtrlType2TypeDef* ctrl = (USBH_CtrlType2TypeDef*)&ictrl->ctrl;
	switch(ictrl->state) {
	case CTRL_SETUP:
		USBH_UsrLog("CTRL_SETUP");
		if (ctrl->Setup.b.wLength.w != 0 )
			ictrl->state = ((ctrl->Setup.b.bmRequestType & USB_REQ_DIR_MASK) == USB_D2H) ? CTRL_DATA_IN: CTRL_DATA_OUT;
		else
			ictrl->state = ((ctrl->Setup.b.bmRequestType & USB_REQ_DIR_MASK) == USB_D2H) ? CTRL_STATUS_OUT: CTRL_STATUS_IN;
		USBH_SubmitSetup(ctrl->ep_out,&ctrl->Setup);
		break;
	case CTRL_DATA_IN:
		USBH_UsrLog("CTRL_DATA_IN");
		ictrl->state = CTRL_STATUS_OUT;
		USBH_SubmitRequest(ctrl->ep_in,(uint8_t*)ictrl->data, ctrl->Setup.b.wLength.w);
		break;
	case CTRL_DATA_OUT: USBH_UsrLog("CTRL_DATA_OUT");
		ictrl->state = CTRL_STATUS_IN;
		USBH_SubmitRequest(ctrl->ep_out,(uint8_t*)ictrl->data, ctrl->Setup.b.wLength.w);
		break;
	case CTRL_STATUS_IN: USBH_UsrLog("CTRL_STATUS_IN");
		ictrl->state = CTRL_COMPLETE;
		USBH_SubmitRequest(ctrl->ep_in,NULL,0);
		break;
	case CTRL_STATUS_OUT: USBH_UsrLog("CTRL_STATUS_OUT");
		ictrl->state = CTRL_COMPLETE;
		USBH_SubmitRequest(ctrl->ep_out,NULL,0);
		break;
	case CTRL_COMPLETE:
		ctrl->status = USBH_PIPE_IDLE;
		ctrl->urb_state= USBH_URB_DONE;
		if(ictrl->Callback) ictrl->Callback(ctrl,ictrl->pData);
		break;
	default:
		break;
	}

}
void USBH_SubmitControl(USBH_CtrlType2TypeDef* ctrl, void* dev, uint8_t length){
	//USBH_CtlSendSetup((uint8_t *)ctrl->Setup.d8 ,ctrl->ep_out->Channel);
	USBH_SubmitSetup(ctrl->ep_out,&ctrl->Setup);
	while(ctrl->ep_out->status == USBH_PIPE_WORKING);
	assert(ctrl->ep_out->urb_state == USBH_URB_DONE);
	 uint8_t direction = (ctrl->Setup.b.bmRequestType & USB_REQ_DIR_MASK);
	if (ctrl->Setup.b.wLength.w != 0 )
	{
		USBH_UsrLog("Data?");
		if (direction == USB_D2H)
		{
			USBH_UsrLog("USBH_SubmitRequest(ctrl->ep_in,(uint8_t*)dev, length)");
			USBH_SubmitRequest(ctrl->ep_in,(uint8_t*)dev, length);
			// USBH_CtlReceiveData(,ctrl->ep_in->Channel);
			 while(ctrl->ep_in->status == USBH_PIPE_WORKING);
			 assert(ctrl->ep_in->urb_state == USBH_URB_DONE);
		}
		else
		{
			USBH_UsrLog("USBH_SubmitRequest(ctrl->ep_out,(uint8_t*)dev, length)");
			USBH_SubmitRequest(ctrl->ep_out,(uint8_t*)dev, length);
			//USBH_CtlSendData((uint8_t*)dev, length,ctrl->ep_out->Channel,1);
			while(ctrl->ep_out->status == USBH_PIPE_WORKING);
			assert(ctrl->ep_out->urb_state == USBH_URB_DONE);
		}
	}
	USBH_UsrLog("status!");
	/* No DATA stage */
	/* If there is No Data Transfer Stage */
	if (direction == USB_D2H)
	{
		USBH_SubmitRequest(ctrl->ep_out,NULL,0);
		//USBH_CtlSendData(NULL,0,ctrl->ep_out->Channel,1);
		while(ctrl->ep_out->status == USBH_PIPE_WORKING);
		assert(ctrl->ep_out->urb_state == USBH_URB_DONE);
	}
	else
	{
		USBH_SubmitRequest(ctrl->ep_in,NULL,0);
		//USBH_CtlReceiveData(NULL,0,ctrl->ep_in->Channel);
		 while(ctrl->ep_in->status == USBH_PIPE_WORKING);
		 assert(ctrl->ep_in->urb_state == USBH_URB_DONE);
	}
}
void  TestDev(const USBH_CtrlType2TypeDef* cctrl, USB_DEVICE_DESCRIPTOR* dev, uint8_t length) {
	USBH_CtrlType2TypeDef* ctrl = (USBH_CtrlType2TypeDef* )cctrl;
	ctrl->Setup.b.bmRequestType = USB_D2H | USB_REQ_RECIPIENT_DEVICE | USB_REQ_TYPE_STANDARD;
	ctrl->Setup.b.bRequest = USB_REQ_GET_DESCRIPTOR;
	ctrl->Setup.b.wValue.w = USB_DESC_DEVICE;
	ctrl->Setup.b.wIndex.w = 0;
	ctrl->Setup.b.wLength.w = length;
	ctrl->status = USBH_PIPE_WORKING;
	t_internal_control* ictrl = (t_internal_control*)ctrl;
	ictrl->state= CTRL_SETUP;
	USBH_SubmitControlCallback(NULL, ictrl);
	while(ctrl->status == USBH_PIPE_WORKING);
}

static USBH_StatusTypeDef USBH_HandleControl ();
/**
  * @brief  USBH_CtlReq
  *         USBH_CtlReq sends a control request and provide the status after 
  *            completion of the request
  * @param  phost: Host Handle
  * @param  req: Setup Request Structure
  * @param  buff: data buffer address to store the response
  * @param  length: length of the response
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_CtlReq(
                             uint8_t             *buff,
                             uint16_t            length)
{
  USBH_StatusTypeDef status;
  status = USBH_BUSY;
  
  switch (Control.req_state)
  {
  case CMD_SEND:
    /* Start a SETUP transfer */
    Control.buff = buff;
    Control.length = length;
    Control.state = CTRL_SETUP;
    Control.req_state = CMD_WAIT;
    status = USBH_BUSY;
    break;
    
  case CMD_WAIT:
    status = USBH_HandleControl();
     if (status == USBH_OK) 
    {
      /* Commands successfully sent and Response Received  */       
    	 Control.req_state = CMD_SEND;
    	 Control.state =CTRL_IDLE;
      status = USBH_OK;      
    }
    else if  (status == USBH_FAIL)
    {
      /* Failure Mode */
    	Control.req_state = CMD_SEND;
      status = USBH_FAIL;
    }   
    break;
    
  default:
    break; 
  }
  return status;
}

/**
  * @brief  USBH_HandleControl
  *         Handles the USB control transfer state machine
  * @param  phost: Host Handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_HandleControl ()
{
  uint8_t direction;  
  USBH_StatusTypeDef status = USBH_BUSY;
  USBH_URBStateTypeDef URB_Status = USBH_URB_IDLE;
  
  switch (Control.state)
  {
  case CTRL_SETUP:
	  USBH_UsrLog("CTRL_SETUP");
    /* send a SETUP packet */
    USBH_CtlSendSetup     (
	                   (uint8_t *)Control.setup.d8 ,
	                   Control.pipe_out);
    
    Control.state = CTRL_SETUP_WAIT;
    break; 
    
  case CTRL_SETUP_WAIT:
    URB_Status = USBH_PipeStatus(Control.pipe_out);
    /* case SETUP packet sent successfully */
    if(URB_Status == USBH_URB_DONE)
    { 
      direction = (Control.setup.b.bmRequestType & USB_REQ_DIR_MASK);
      
      /* check if there is a data stage */
      if (Control.setup.b.wLength.w != 0 )
      {        
        if (direction == USB_D2H)
        {
          /* Data Direction is IN */
          Control.state = CTRL_DATA_IN;
        }
        else
        {
          /* Data Direction is OUT */
          Control.state = CTRL_DATA_OUT;
        } 
      }
      /* No DATA stage */
      else
      {
        /* If there is No Data Transfer Stage */
        if (direction == USB_D2H)
        {
          /* Data Direction is IN */
          Control.state = CTRL_STATUS_OUT;
        }
        else
        {
          /* Data Direction is OUT */
          Control.state = CTRL_STATUS_IN;
        } 
      }          

    }
    else if(URB_Status == USBH_URB_ERROR)
    {
      Control.state = CTRL_ERROR;

    }    
    break;
    
  case CTRL_DATA_IN:  
    /* Issue an IN token */ 
    USBH_CtlReceiveData(
                        Control.buff,
                        Control.length,
                        Control.pipe_in);
 
    Control.state = CTRL_DATA_IN_WAIT;
    break;    
    
  case CTRL_DATA_IN_WAIT:
    
    URB_Status = USBH_PipeStatus( Control.pipe_in);
    
    /* check is DATA packet transferred successfully */
    if  (URB_Status == USBH_URB_DONE)
    { 
      Control.state = CTRL_STATUS_OUT;
    }
   
    /* manage error cases*/
    if  (URB_Status == USBH_URB_STALL) 
    { 
      /* In stall case, return to previous machine state*/
      status = USBH_NOT_SUPPORTED;
    }   
    else if (URB_Status == USBH_URB_ERROR)
    {
      /* Device error */
      Control.state = CTRL_ERROR;
    }
    break;
    
  case CTRL_DATA_OUT:
	  USBH_UsrLog("CTRL_DATA_OUT");
    USBH_CtlSendData (
                      Control.buff,
                      Control.length ,
                      Control.pipe_out,
                      1);
    Control.state = CTRL_DATA_OUT_WAIT;
    break;
    
  case CTRL_DATA_OUT_WAIT:
    
    URB_Status = USBH_PipeStatus( Control.pipe_out);
    
    if  (URB_Status == USBH_URB_DONE)
    { /* If the Setup Pkt is sent successful, then change the state */
      Control.state = CTRL_STATUS_IN;
    }
    
    /* handle error cases */
    else if  (URB_Status == USBH_URB_STALL) 
    { 
      /* In stall case, return to previous machine state*/
      Control.state = CTRL_STALLED;
      status = USBH_NOT_SUPPORTED;
    } 
    else if  (URB_Status == USBH_URB_NOTREADY)
    { 
      /* Nack received from device */
      Control.state = CTRL_DATA_OUT;

    }    
    else if (URB_Status == USBH_URB_ERROR)
    {
      /* device error */
      Control.state = CTRL_ERROR;
      status = USBH_FAIL;
    } 
    break;
    
    
  case CTRL_STATUS_IN:
    /* Send 0 bytes out packet */
    USBH_CtlReceiveData (
                         0,
                         0,
                         Control.pipe_in);
    Control.state = CTRL_STATUS_IN_WAIT;
    
    break;
    
  case CTRL_STATUS_IN_WAIT:
    
    URB_Status = USBH_PipeStatus( Control.pipe_in);
    
    if  ( URB_Status == USBH_URB_DONE)
    { /* Control transfers completed, Exit the State Machine */
      Control.state = CTRL_COMPLETE;
      status = USBH_OK;
    }
    
    else if (URB_Status == USBH_URB_ERROR)
    {
      Control.state = CTRL_ERROR;
    }
     else if(URB_Status == USBH_URB_STALL)
    {
      /* Control transfers completed, Exit the State Machine */
      status = USBH_NOT_SUPPORTED;

    }
    break;
    
  case CTRL_STATUS_OUT:
    USBH_CtlSendData (
                      0,
                      0,
                      Control.pipe_out,
                      1);
    Control.state = CTRL_STATUS_OUT_WAIT;
    break;
    
  case CTRL_STATUS_OUT_WAIT: 
    
    URB_Status = USBH_PipeStatus( Control.pipe_out);
    if  (URB_Status == USBH_URB_DONE)
    { 
      status = USBH_OK;      
      Control.state = CTRL_COMPLETE;

    }
    else if  (URB_Status == USBH_URB_NOTREADY)
    { 
      Control.state = CTRL_STATUS_OUT;

    }      
    else if (URB_Status == USBH_URB_ERROR)
    {
      Control.state = CTRL_ERROR;

    }
    break;
    
  case CTRL_ERROR:
    /* 
    After a halt condition is encountered or an error is detected by the 
    host, a control endpoint is allowed to recover by accepting the next Setup 
    PID; i.e., recovery actions via some other pipe are not required for control
    endpoints. For the Default Control Pipe, a device reset will ultimately be 
    required to clear the halt or error condition if the next Setup PID is not 
    accepted.
    */
    if (++ Control.errorcount <= USBH_MAX_ERROR_COUNT)
    {
      /* try to recover control */
      USBH_Stop();
         
      /* Do the transmission again, starting from SETUP Packet */
      Control.state = CTRL_SETUP;
      Control.req_state = CMD_SEND;
    }
    else
    {
      Control.errorcount = 0;
      USBH_ErrLog("Control error");
      status = USBH_FAIL;
    }
    break;
    
  default:
    break;
  }
  return status;
}
#endif

