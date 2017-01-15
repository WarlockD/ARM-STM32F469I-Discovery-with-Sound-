/**
  ******************************************************************************
  * @file    usbh_hid.c
  * @author  MCD Application Team
  * @version V3.2.2
  * @date    07-July-2015
  * @brief   This file is the HID Layer Handlers for USB Host HID class.
  *
  * @verbatim
  *      
  *          ===================================================================      
  *                                HID Class  Description
  *          =================================================================== 
  *           This module manages the HID class V1.11 following the "Device Class Definition
  *           for Human Interface Devices (HID) Version 1.11 Jun 27, 2001".
  *           This driver implements the following aspects of the specification:
  *             - The Boot Interface Subclass
  *             - The Mouse and Keyboard protocols
  *      
  *  @endverbatim
  *
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
#include "usbh_hid.h"
#include "usbh_hid_parser.h"
#include "HIDParser.h"


static USBH_StatusTypeDef USBH_HID_InterfaceInit  (USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_HID_InterfaceDeInit  (USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_HID_ClassRequest(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_HID_Process(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_HID_SOFProcess(USBH_HandleTypeDef *phost);
static void  USBH_HID_ParseHIDDesc (HID_DescTypeDef *desc, uint8_t *buf);

//extern USBH_StatusTypeDef USBH_HID_MouseInit(USBH_HandleTypeDef *phost);
//extern USBH_StatusTypeDef USBH_HID_KeybdInit(USBH_HandleTypeDef *phost);

USBH_ClassTypeDef  HID_Class = 
{
  "HID",
  USB_HID_CLASS,
  USBH_HID_InterfaceInit,
  USBH_HID_InterfaceDeInit,
  USBH_HID_ClassRequest,
  USBH_HID_Process,
  USBH_HID_SOFProcess,
  NULL,
};
/**
* @}
*/ 


/** @defgroup USBH_HID_CORE_Private_Functions
* @{
*/ 
#define HID_MIX_DRIVERS 10
static HID_Callback HID_Drivers[HID_MIX_DRIVERS];
static uint32_t driver_count = 0;

USBH_StatusTypeDef USBH_HID_InstallDriver(HID_Callback init) {
	if(init == NULL || driver_count == HID_MIX_DRIVERS) return USBH_FAIL;
	HID_Drivers[driver_count++] = init;
	return USBH_OK;
}


static void USBH_Print_HID_DescTypeDef(HID_DescTypeDef* def){
	USBH_UsrLog("HID_DescTypeDef");
	USBH_UsrLog("   bLength=%i",(int)def->bLength);
	USBH_UsrLog("   bDescriptorType=%i",(int)def->bDescriptorType);
	USBH_UsrLog("   bcdHID=%i",(int)def->bcdHID);
	USBH_UsrLog("   bCountryCode=%i",(int)def->bCountryCode);
	USBH_UsrLog("   bNumDescriptors=%i",(int)def->bNumDescriptors);
	USBH_UsrLog("   bReportDescriptorType=%i",(int)def->bReportDescriptorType);
	USBH_UsrLog("   wItemLength=%i",(int)def->wItemLength);
}
static void USBH_Print_HID_USBH_InterfaceDescTypeDef(USBH_InterfaceDescTypeDef* def){
	USBH_UsrLog("USBH_InterfaceDescTypeDef");
	USBH_UsrLog("   bLength=%i",(int)def->bLength);
	USBH_UsrLog("   bDescriptorType=%i",(int)def->bDescriptorType);
	USBH_UsrLog("   bAlternateSetting=%i",(int)def->bAlternateSetting);
	USBH_UsrLog("   bNumEndpoints=%i",(int)def->bNumEndpoints);
	USBH_UsrLog("   bInterfaceClass=%i",(int)def->bInterfaceClass);
	USBH_UsrLog("   bInterfaceSubClass=%i",(int)def->bInterfaceSubClass);
	USBH_UsrLog("   bInterfaceProtocol=%i",(int)def->bInterfaceProtocol);
	 // uint8_t iInterface;           /* Index of String Descriptor Describing this interface */
	//  USBH_EpDescTypeDef               Ep_Desc[USBH_MAX_NUM_ENDPOINTS];
}


HID_HandleTypeDef* USBH_HID_CurrentHandle(USBH_HandleTypeDef *phost){
	HID_HandleTypeDef *HID_Handles=(HID_HandleTypeDef *)phost->pActiveClass->pData;
	if(HID_Handles==NULL) return NULL;
	return &HID_Handles[phost->device.current_interface];
}


static USBH_StatusTypeDef USBH_HID_InterfaceInit(USBH_HandleTypeDef *phost)
{	
	USBH_StatusTypeDef status = USBH_FAIL ;
	HID_HandleTypeDef* HID_Handles = NULL;
	uint8_t max_interfaces = phost->device.CfgDesc.bNumInterfaces;
	uint32_t total_handle_size = max_interfaces * sizeof(USBH_HandleTypeDef);
	for(uint32_t interface = 0; interface < max_interfaces;interface++){
		uint8_t max_ep = phost->device.CfgDesc.Itf_Desc[interface].bNumEndpoints;
		uint32_t max_packet_size=0;
		for(uint32_t i=0;i < max_ep;i++){
			uint32_t l = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[0].wMaxPacketSize;
			if(l > max_packet_size) max_packet_size=l;
		}
		total_handle_size+=(max_packet_size*HID_QUEUE_SIZE);
	}
  	HID_Handles = USBH_malloc(total_handle_size);
  	if(HID_Handles == NULL) return status;
  	USBH_UsrLog("HID_Handles total=%lu",total_handle_size);
  	memset(HID_Handles,0,total_handle_size);
  	uint8_t* buffer = ((uint8_t*)HID_Handles) + (max_interfaces * sizeof(USBH_HandleTypeDef));
  	// cycle though again to init evetying
  	for(uint32_t interface = 0; interface < max_interfaces;interface++){
  		HID_HandleTypeDef* HID_Handle = &HID_Handles[interface];
  		HID_Handle->phost = phost;
  		HID_Handle->Interface = interface;
  		HID_Handle->state     = HID_INIT;
  		HID_Handle->ctl_state = HID_REQ_INIT;
  		HID_Handle->ep_addr   = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[0].bEndpointAddress;
  		HID_Handle->length    = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[0].wMaxPacketSize;
  		HID_Handle->poll      = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[0].bInterval ;
  		HID_Handle->pData	  = buffer;
  		buffer+= (HID_Handle->length*HID_QUEUE_SIZE); // move ot the next interface buffer
  		// init the fifo here since all the driveers use it
  		fifo_init(&HID_Handle->fifo, HID_Handle->pData, HID_Handle->length*HID_QUEUE_SIZE);
  		/* Check fo available number of endpoints */
  		/* Find the number of EPs in the Interface Descriptor */
  		/* Choose the lower number in order not to overrun the buffer allocated */
  		uint8_t max_ep = ( (phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].bNumEndpoints <= USBH_MAX_NUM_ENDPOINTS) ?
  				  phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].bNumEndpoints :
  					  USBH_MAX_NUM_ENDPOINTS);


  		/* Decode endpoint IN and OUT address from interface descriptor */
  		for (uint32_t num=0 ;num < max_ep; num++)
  		{
  		if(phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[num].bEndpointAddress & 0x80)
  		{
  			HID_Handle->InEp = (phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[num].bEndpointAddress);
  			HID_Handle->InPipe  = USBH_AllocPipe(phost, HID_Handle->InEp);

  			/* Open pipe for IN endpoint */
  			USBH_OpenPipe  (phost,
  							HID_Handle->InPipe,
  							HID_Handle->InEp,
  							phost->device.address,
  							phost->device.speed,
  							USB_EP_TYPE_INTR,
  							HID_Handle->length);

  			USBH_LL_SetToggle (phost, HID_Handle->InPipe, 0);
  			status= USBH_OK;
  		}
  		else
  		{
  			HID_Handle->OutEp = (phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[num].bEndpointAddress);
  			HID_Handle->OutPipe  =USBH_AllocPipe(phost, HID_Handle->OutEp);

  			/* Open pipe for OUT endpoint */
  			USBH_OpenPipe  (phost,
  							HID_Handle->OutPipe,
  							HID_Handle->OutEp,
  							phost->device.address,
  							phost->device.speed,
  							USB_EP_TYPE_INTR,
  							HID_Handle->length);
  			USBH_LL_SetToggle (phost, HID_Handle->OutPipe, 0);
  			status= USBH_OK;
  		}
  	}

}
	if(status == USBH_FAIL && HID_Handles) USBH_free(HID_Handles);
  	else phost->pActiveClass->pData = HID_Handles;
   return status;
}

/**
  * @brief  USBH_HID_InterfaceDeInit 
  *         The function DeInit the Pipes used for the HID class.
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_HID_InterfaceDeInit (USBH_HandleTypeDef *phost )
{	
	HID_HandleTypeDef *HID_Handles = (HID_HandleTypeDef *) phost->pActiveClass->pData;
	if(HID_Handles == NULL) return USBH_FAIL;
	for(uint32_t interface=0; interface < phost->device.CfgDesc.bNumInterfaces;interface++) {
	  // USBH_SelectInterface (phost, interface); NO REASON TO CALL THIS EXCEPT TO PRINT DEBUG INFO
	  HID_HandleTypeDef *HID_Handle =  &HID_Handles[interface];
	  if(HID_Handle->InPipe != 0x00)
	   {
	     USBH_ClosePipe  (phost, HID_Handle->InPipe);
	     USBH_FreePipe  (phost, HID_Handle->InPipe);
	     HID_Handle->InPipe = 0;     /* Reset the pipe as Free */
	   }
	  if(HID_Handle->OutPipe != 0x00)
	   {
	     USBH_ClosePipe(phost, HID_Handle->OutPipe);
	     USBH_FreePipe(phost, HID_Handle->OutPipe);
	     HID_Handle->OutPipe = 0;     /* Reset the pipe as Free */
	   }
	}
	USBH_free (phost->pActiveClass->pData);
	phost->pActiveClass->pData=NULL;
  return USBH_OK;
}


USBH_StatusTypeDef USBH_HID_ClassRequest(USBH_HandleTypeDef *phost)
{   
  
  USBH_StatusTypeDef status         = USBH_BUSY;
  USBH_StatusTypeDef classReqStatus = USBH_BUSY;
  HID_HandleTypeDef *HID_Handles =  (HID_HandleTypeDef *)phost->pActiveClass->pData;
  HID_HandleTypeDef *HID_Handle =  &HID_Handles[0];

  //USBH_HID_CurrentHandle(phost);

  /* Switch HID state machine */
  switch (HID_Handle->ctl_state)
  {
  case HID_REQ_INIT:  
  case HID_REQ_GET_HID_DESC:
    /* Get HID Desc */ 
    if (USBH_HID_GetHIDDescriptor (phost, USB_HID_DESC_SIZE)== USBH_OK)
    {
      USBH_HID_ParseHIDDesc(&HID_Handle->HID_Desc, phost->device.Data);
      // we know the basic data, so we can now generate handles
      
      HID_Handle->ctl_state = HID_REQ_GET_REPORT_DESC;
    }
    break;     
  case HID_REQ_GET_REPORT_DESC:
    /* Get Report Desc */ 
    if (USBH_HID_GetHIDReportDescriptor(phost, HID_Handle->HID_Desc.wItemLength) == USBH_OK)
    {
    	// We don't know what this thing is till we get to this point
    	// should be the first fifo write
    	fifo_write(&HID_Handle->fifo, phost->device.Data, HID_Handle->HID_Desc.wItemLength);
    	for(uint32_t i=0; i < driver_count;i++){
    		if(HID_Drivers[i](HID_Handle) == USBH_OK) break;
    	}
    	if(HID_Handle->Callback){
    		USBH_UsrLog("HID Driver installed");
    	} else {
    		USBH_UsrLog(" No HID Driver found");
    	}
    	//USBH_UsrLog("USBH_HID_GetHIDDescriptor length=%i", (int)HID_Handle->HID_Desc.wItemLength);
    	//print_buffer(phost->device.Data, HID_Handle->HID_Desc.wItemLength);

      HID_Handle->ctl_state = HID_REQ_SET_IDLE;
    }
    
    break;
    
  case HID_REQ_SET_IDLE:
    
    classReqStatus = USBH_HID_SetIdle (phost, 0, 0);
    
    /* set Idle */
    if (classReqStatus == USBH_OK)
    {
      HID_Handle->ctl_state = HID_REQ_SET_PROTOCOL;  
    }
    else if(classReqStatus == USBH_NOT_SUPPORTED) 
    {
      HID_Handle->ctl_state = HID_REQ_SET_PROTOCOL;        
    } 
    break; 
    
  case HID_REQ_SET_PROTOCOL:
    /* set protocol */
    if (USBH_HID_SetProtocol (phost, 1) == USBH_OK) // hard mode, no boot protocol
  //  if (USBH_HID_SetProtocol (phost, 0) == USBH_OK)
    {
      HID_Handle->ctl_state = HID_REQ_IDLE;
      
      /* all requests performed*/
      phost->pUser(phost, HOST_USER_CLASS_ACTIVE); 
      status = USBH_OK; 
    } 
    break;
    
  case HID_REQ_IDLE:
  default:
    break;
  }
  
  return status; 
}


static USBH_StatusTypeDef USBH_HID_Process(USBH_HandleTypeDef *phost)
{
  USBH_StatusTypeDef status = USBH_OK;
  HID_HandleTypeDef *HID_Handles =  (HID_HandleTypeDef *) phost->pActiveClass->pData;
  for(uint8_t interface=0; interface < phost->device.CfgDesc.bNumInterfaces;interface++) {
	  HID_HandleTypeDef *HID_Handle =  &HID_Handles[interface];
	  if(HID_Handle->Process) status = HID_Handle->Process(HID_Handle);
	  else status = USBH_FAIL;
	  if(status != USBH_OK) break;
  }
  return status;
}

/**
  * @brief  USBH_HID_SOFProcess 
  *         The function is for managing the SOF Process 
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_HID_SOFProcess(USBH_HandleTypeDef *phost)
{
	HID_HandleTypeDef *HID_Handles =  (HID_HandleTypeDef *)phost->pActiveClass->pData;
	for(uint8_t interface=0; interface < phost->device.CfgDesc.bNumInterfaces;interface++) {
	  HID_HandleTypeDef *HID_Handle =  &HID_Handles[interface];
	  if(HID_Handle->state == HID_POLL)
	  {
		if(( phost->Timer - HID_Handle->timer) >= HID_Handle->poll)
		{
		  HID_Handle->state = HID_GET_DATA;
	#if (USBH_USE_OS == 1)
		osMessagePut ( phost->os_event, USBH_URB_EVENT, 0);
	#endif
		}
	  }
	}
  return USBH_OK;
}

/**
* @brief  USBH_Get_HID_ReportDescriptor
  *         Issue report Descriptor command to the device. Once the response 
  *         received, parse the report descriptor and update the status.
  * @param  phost: Host handle
  * @param  Length : HID Report Descriptor Length
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_HID_GetHIDReportDescriptor (USBH_HandleTypeDef *phost,
                                                         uint16_t length)
{
  
  USBH_StatusTypeDef status;
  
  status = USBH_GetDescriptor(phost,
                              USB_REQ_RECIPIENT_INTERFACE | USB_REQ_TYPE_STANDARD,                                  
                              USB_DESC_HID_REPORT, 
                              phost->device.Data,
                              length);


  /* HID report descriptor is available in phost->device.Data.
  In case of USB Boot Mode devices for In report handling ,
  HID report descriptor parsing is not required.
  In case, for supporting Non-Boot Protocol devices and output reports,
  user may parse the report descriptor*/
  
  
  return status;
}

/**
  * @brief  USBH_Get_HID_Descriptor
  *         Issue HID Descriptor command to the device. Once the response 
  *         received, parse the report descriptor and update the status.
  * @param  phost: Host handle
  * @param  Length : HID Descriptor Length
  * @retval USBH Status
  */

USBH_StatusTypeDef USBH_HID_GetHIDDescriptor (USBH_HandleTypeDef *phost,
                                            uint16_t length)
{
  
  USBH_StatusTypeDef status;
  
  status = USBH_GetDescriptor( phost,
                              USB_REQ_RECIPIENT_INTERFACE | USB_REQ_TYPE_STANDARD,                                  
                              USB_DESC_HID,
                              phost->device.Data,
                              length);

  return status;
}

/**
  * @brief  USBH_Set_Idle
  *         Set Idle State. 
  * @param  phost: Host handle
  * @param  duration: Duration for HID Idle request
  * @param  reportId : Targeted report ID for Set Idle request
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_HID_SetIdle (USBH_HandleTypeDef *phost,
                                         uint8_t duration,
                                         uint8_t reportId)
{
  
  phost->Control.setup.b.bmRequestType = USB_H2D | USB_REQ_RECIPIENT_INTERFACE |\
    USB_REQ_TYPE_CLASS;
  
  
  phost->Control.setup.b.bRequest = USB_HID_SET_IDLE;
  phost->Control.setup.b.wValue.w = (duration << 8 ) | reportId;
  
  phost->Control.setup.b.wIndex.w = 0;
  phost->Control.setup.b.wLength.w = 0;
  
  return USBH_CtlReq(phost, 0 , 0 );
}


/**
  * @brief  USBH_HID_Set_Report
  *         Issues Set Report 
  * @param  phost: Host handle
  * @param  reportType  : Report type to be sent
  * @param  reportId    : Targeted report ID for Set Report request
  * @param  reportBuff  : Report Buffer
  * @param  reportLen   : Length of data report to be send
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_HID_SetReport (USBH_HandleTypeDef *phost,
                                    uint8_t reportType,
                                    uint8_t reportId,
                                    uint8_t* reportBuff,
                                    uint8_t reportLen)
{
  
  phost->Control.setup.b.bmRequestType = USB_H2D | USB_REQ_RECIPIENT_INTERFACE |\
    USB_REQ_TYPE_CLASS;
  
  
  phost->Control.setup.b.bRequest = USB_HID_SET_REPORT;
  phost->Control.setup.b.wValue.w = (reportType << 8 ) | reportId;
  
  phost->Control.setup.b.wIndex.w = 0;
  phost->Control.setup.b.wLength.w = reportLen;
  
  return USBH_CtlReq(phost, reportBuff , reportLen );
}


/**
  * @brief  USBH_HID_GetReport
  *         retreive Set Report 
  * @param  phost: Host handle
  * @param  reportType  : Report type to be sent
  * @param  reportId    : Targeted report ID for Set Report request
  * @param  reportBuff  : Report Buffer
  * @param  reportLen   : Length of data report to be send
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_HID_GetReport (USBH_HandleTypeDef *phost,
                                    uint8_t reportType,
                                    uint8_t reportId,
                                    uint8_t* reportBuff,
                                    uint8_t reportLen)
{
  
  phost->Control.setup.b.bmRequestType = USB_D2H | USB_REQ_RECIPIENT_INTERFACE |\
    USB_REQ_TYPE_CLASS;
  
  
  phost->Control.setup.b.bRequest = USB_HID_GET_REPORT;
  phost->Control.setup.b.wValue.w = (reportType << 8 ) | reportId;
  
  phost->Control.setup.b.wIndex.w = 0;
  phost->Control.setup.b.wLength.w = reportLen;
  
  return USBH_CtlReq(phost, reportBuff , reportLen );
}

/**
  * @brief  USBH_Set_Protocol
  *         Set protocol State.
  * @param  phost: Host handle
  * @param  protocol : Set Protocol for HID : boot/report protocol
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_HID_SetProtocol(USBH_HandleTypeDef *phost,
                                            uint8_t protocol)
{
  
  
  phost->Control.setup.b.bmRequestType = USB_H2D | USB_REQ_RECIPIENT_INTERFACE |\
    USB_REQ_TYPE_CLASS;
  
  
  phost->Control.setup.b.bRequest = USB_HID_SET_PROTOCOL;
  phost->Control.setup.b.wValue.w = protocol != 0 ? 0 : 1;
  phost->Control.setup.b.wIndex.w = 0;
  phost->Control.setup.b.wLength.w = 0;
  
  return USBH_CtlReq(phost, 0 , 0 );
  
}

/**
  * @brief  USBH_ParseHIDDesc 
  *         This function Parse the HID descriptor
  * @param  desc: HID Descriptor
  * @param  buf: Buffer where the source descriptor is available
  * @retval None
  */
static void  USBH_HID_ParseHIDDesc (HID_DescTypeDef *desc, uint8_t *buf)
{
  
  desc->bLength                  = *(uint8_t  *) (buf + 0);
  desc->bDescriptorType          = *(uint8_t  *) (buf + 1);
  desc->bcdHID                   =  LE16  (buf + 2);
  desc->bCountryCode             = *(uint8_t  *) (buf + 4);
  desc->bNumDescriptors          = *(uint8_t  *) (buf + 5);
  desc->bReportDescriptorType    = *(uint8_t  *) (buf + 6);
  desc->wItemLength              =  LE16  (buf + 7);
} 

/**
  * @brief  USBH_HID_GetDeviceType
  *         Return Device function.
  * @param  phost: Host handle
  * @retval HID function: HID_MOUSE / HID_KEYBOARD
  */
HID_TypeTypeDef USBH_HID_GetDeviceType(USBH_HandleTypeDef *phost)
{
  HID_TypeTypeDef   type = HID_UNKNOWN;
  
  if(phost->gState == HOST_CLASS)
  {
    
    if(phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].bInterfaceProtocol \
      == HID_KEYBRD_BOOT_CODE)
    {
      type = HID_KEYBOARD;  
    }
    else if(phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].bInterfaceProtocol \
      == HID_MOUSE_BOOT_CODE)		  
    {
      type=  HID_MOUSE;  
    }
  }
  return type;
}


/**
  * @brief  USBH_HID_GetPollInterval
  *         Return HID device poll time
  * @param  phost: Host handle
  * @retval poll time (ms)
  */
uint8_t USBH_HID_GetPollInterval(USBH_HandleTypeDef *phost)
{
  HID_HandleTypeDef *HID_Handle =  (HID_HandleTypeDef *) phost->pActiveClass->pData;
    
    if((phost->gState == HOST_CLASS_REQUEST) ||
       (phost->gState == HOST_INPUT) ||
         (phost->gState == HOST_SET_CONFIGURATION) ||
           (phost->gState == HOST_CHECK_CLASS) ||           
             ((phost->gState == HOST_CLASS)))
  {
    return (HID_Handle->poll);
  }
  else
  {
    return 0;
  }
}
/**
  * @brief  fifo_init
  *         Initialize FIFO.
  * @param  f: Fifo address
  * @param  buf: Fifo buffer
  * @param  size: Fifo Size
  * @retval none
  */
void fifo_init(FIFO_TypeDef * f, uint8_t * buf, uint16_t size)
{
     f->head = 0;
     f->tail = 0;
     f->lock = 0;
     f->size = size;
     f->buf = buf;
}

/**
  * @brief  fifo_read
  *         Read from FIFO.
  * @param  f: Fifo address
  * @param  buf: read buffer 
  * @param  nbytes: number of item to read
  * @retval number of read items
  */
uint16_t  fifo_read(FIFO_TypeDef * f, void * buf, uint16_t  nbytes)
{
  uint16_t  i;
  uint8_t * p;
  p = (uint8_t*) buf;
  
  if(f->lock == 0)
  {
    f->lock = 1;
    for(i=0; i < nbytes; i++)
    {
      if( f->tail != f->head )
      { 
        *p++ = f->buf[f->tail];  
        f->tail++;  
        if( f->tail == f->size )
        {  
          f->tail = 0;
        }
      } else 
      {
        f->lock = 0;
        return i; 
      }
    }
  }
  f->lock = 0;
  return nbytes;
}
 
/**
  * @brief  fifo_write
  *         Read from FIFO.
  * @param  f: Fifo address
  * @param  buf: read buffer 
  * @param  nbytes: number of item to write
  * @retval number of written items
  */
uint16_t  fifo_write(FIFO_TypeDef * f, const void * buf, uint16_t  nbytes)
{
  uint16_t  i;
  const uint8_t * p;
  p = (const uint8_t*) buf;
  if(f->lock == 0)
  {
    f->lock = 1;
    for(i=0; i < nbytes; i++)
    {
      if( (f->head + 1 == f->tail) ||
         ( (f->head + 1 == f->size) && (f->tail == 0)) )
      {
        f->lock = 0;
        return i;
      } 
      else 
      {
        f->buf[f->head] = *p++;
        f->head++;
        if( f->head == f->size )
        {
          f->head = 0;
        }
      }
    }
  }
  f->lock = 0;
  return nbytes;
}


/**
* @brief  The function is a callback about HID Data events
*  @param  phost: Selected device
* @retval None
*/
__weak void USBH_HID_EventCallback(USBH_HandleTypeDef *phost, HID_HandleTypeDef* handle)
{
  
}
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


/**
* @}
*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
