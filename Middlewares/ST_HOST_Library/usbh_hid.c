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
#include "usbh_fifo.h"

static USBH_StatusTypeDef USBH_HID_InterfaceInit  (USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_HID_InterfaceDeInit  (USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_HID_ClassRequest(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_HID_Process(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_HID_SOFProcess(USBH_HandleTypeDef *phost);
static void USBH_HID_ParseHIDDesc (HID_DescTypeDef * desc, uint8_t *buf);

// Reports should be big enough to hold them all, no error checking on its size

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
static HID_CallbackDriver HID_Drivers[HID_MIX_DRIVERS];
static uint32_t driver_count = 0;

USBH_StatusTypeDef USBH_HID_InstallDriver(HID_CallbackDriver init) {
	if(init == NULL || driver_count == HID_MIX_DRIVERS) return USBH_FAIL;
	HID_Drivers[driver_count++] = init;
	return USBH_OK;
}



const char* pDescriptorType(uint8_t x)
{
	static const char* tbl[]= {
		"Undefined",
		"Device",
		"Configuration",
		"String",
		"Interface",
		"Endpoint",
		"Device Qualifier",
		"Other Speed",
		"Interface Power",
		"OTG",
		"Unknown","Unknown","Unknown","Unknown","Unknown","Unknown",
		"Unknown","Unknown","Unknown","Unknown","Unknown","Unknown","Unknown","Unknown","Unknown","Unknown","Unknown","Unknown","Unknown","Unknown","Unknown","Unknown",
		"Unknown",
		"HID", // 0x21
		"HID Report",
		"Unknown",
		"Dependant on Type",
		"Dependant on Type",
		"Unknown","Unknown","Unknown",
		"Hub", // 0x29
	};
	if (x >= (sizeof(tbl) / sizeof(tbl[0]))) {
		return "Unknown";;
	}
	else {
		//if (tbl[x] == "Unknown" || tbl[x] == "Undefined") {
		//	possible_errors++;
		//}
		return tbl[x];;
	}
}
static void USBH_Print_HID_USBH_USBH_DescHeader_t(int ident, USBH_DescHeaderTypeDef* head){
	IdentLine(ident,"bDescriptorType(%s)=%i bLength=%i",pDescriptorType(head->bDescriptorType),(int)head->bDescriptorType,(int)head->bLength);
}
static void USBH_Print_HID_DevDescTypeDef(int ident, USBH_DevDescTypeDef* dev){
	USBH_Print_HID_USBH_USBH_DescHeader_t(ident++,(USBH_DescHeaderTypeDef*)dev);
	IdentLine(ident,"bcdUSB=%4.4X",dev->bcdUSB );
	IdentLine(ident,"bDeviceClass=%2.2X",(int)dev->bDeviceClass);
	IdentLine(ident,"bDeviceSubClass=%2.2X",(int)dev->bDeviceSubClass);
	IdentLine(ident,"bDeviceProtocol=%i",(int)dev->bDeviceProtocol);
	IdentLine(ident,"bMaxPacketSize=%i",(int)dev->bMaxPacketSize);
	IdentLine(ident,"idVendor=%4.4X",(unsigned)dev->idVendor);
	IdentLine(ident,"idProduct=%4.4X",(unsigned)dev->idProduct);
	IdentLine(ident,"bcdDevice=%4.4X",dev->bcdDevice );
	IdentLine(ident,"iManufacturer=%i",(int)dev->iManufacturer);
	IdentLine(ident,"iProduct=%i",(int)dev->iProduct);
	IdentLine(ident,"iSerialNumber=%i",(int)dev->iSerialNumber);
	IdentLine(ident,"bNumConfigurations=%i",(int)dev->bNumConfigurations);
}


static void USBH_Print_HID_DescTypeDef(int ident, HID_DescTypeDef* def){
	USBH_Print_HID_USBH_USBH_DescHeader_t(ident++,(USBH_DescHeaderTypeDef*)def);
	IdentLine(ident,"bcdHID=%X.%X",def->bcdHID >> 4,def->bcdHID &0xF );
	IdentLine(ident,"bCountryCode=%i",(int)def->bCountryCode);
	IdentLine(ident,"bNumDescriptors=%i",(int)def->bNumDescriptors);
	IdentLine(ident,"bReportDescriptorType=%i",(int)def->bReportDescriptorType);
	IdentLine(ident,"wItemLength=%i",(int)def->wItemLength);
}



const char* pDeviceClass(uint16_t c)
{
	static const char* tbl[]= {
		"Use class information in the Interface Descriptors",
		"Audio",
		"Communications and CDC Control",
		"HID",
		"",
		"Physical",
		"Image",
		"Printer",
		"Mass Storage",
		"Hub",
		"CDC-Data",
		"Smart Card",
		"Content Security",
		"Video",
		"Personal Healthcare",
		"Audio/Video Devices", // 0x10
		"","","","","","","","","","","","","","","", // 0x1F
		"","","","","","","","","","","","","","","","", // 0x2F
		"","","","","","","","","","","","","","","","", // 0x3F
		"","","","","","","","","","","","","","","","", // 0x4F
		"","","","","","","","","","","","","","","","", // 0x5F
		"","","","","","","","","","","","","","","","", // 0x6F
		"","","","","","","","","","","","","","","","", // 0x7F
		"","","","","","","","","","","","","","","","", // 0x8F
		"","","","","","","","","","","","","","","","", // 0x9F
		"","","","","","","","","","","","","","","","", // 0xAF
		"","","","","","","","","","","","","","","","", // 0xBF
		"","","","","","","","","","","","","","","","", // 0xCF
		"","","","","","","","","","","","", // 0xDB
		"Diagnostic Device", // 0xDC
		"","","", // 0xDF
		"Wireless Controller", // 0xE0
		"","","","","","","","","","","","","","", // 0xEE
		"Miscellaneous", // 0xEF
		"","","","","","","","","","","","","","", // 0xFD
		"Application Specific",
		"Vendor Specific",
	};
	if(c >= (sizeof(tbl) / sizeof(tbl[0])) || tbl[c]=="") return NULL;
	else return tbl[c];
}
const char*  pDeviceSubClass(uint16_t c,uint16_t sc)
{
	if (c == 0x10) // AV device
	{
		if (sc == 0x01) {
			return "AV Control Interface";
		}
		if (sc == 0x02) {
			return "AV Data Video";
		}
		if (sc == 0x03) {
			return "AV Data Audio";
		}
	}
	if (c == 0xFE)
	{
		if (sc == 0x01) {
			return "Device Firmware Upgrade";
		}
		if (sc == 0x02) {
			return "IRDA Bridge Device";
		}
		if (sc == 0x03) {
			return "USB Test and Measurement Device";
		}
	}
	return NULL;
}



static const char* EndpointAttributeToString(uint8_t bmAttributes) {
	switch((bmAttributes & 0x03)){
	case 0x00:return "Control";
	case 0x01:
	{
		static char endpoint[128];
		endpoint[0]=0;
		strcat(endpoint,"Isochronous,");
		switch((bmAttributes >> 2) & 0x03){
			case 0x00:strcat(endpoint,"No Sync,"); break;
			case 0x01:strcat(endpoint,"Async,"); break;
			case 0x02:strcat(endpoint,"Adaptive,"); break;
			case 0x03:strcat(endpoint,"Sync,"); break;
		}
		switch((bmAttributes >> 4) & 0x03){
			case 0x00:strcat(endpoint,"Data EP"); break;
			case 0x01:strcat(endpoint,"Feedback EP"); break;
			case 0x02:strcat(endpoint,"Implicit Feedback EP"); break;
			case 0x03:strcat(endpoint,"Reserved"); break;
		}
		return endpoint;
	}
	case 0x02:return  "Bulk";
	case 0x03:return  "Interrupt";
	}
	return NULL; // can't get here
}
static void USBH_Print_HID_EpDescTypeDef(int ident, USBH_EpDescTypeDef* def){
	USBH_Print_HID_USBH_USBH_DescHeader_t(ident++,(USBH_DescHeaderTypeDef*)def);
	const char* str;
	if (def->bEndpointAddress != 0){
		if((def->bEndpointAddress & 0x80) != 0) str = "IN/D2H"; else str = "OUT/H2D";
	} else str = "Control";
	IdentLine(ident,"bEndpointAddress(%s)=%i",str, (int)def->bEndpointAddress);
	IdentLine(ident,"bmAttributes(%s)=%i",EndpointAttributeToString(def->bmAttributes),(int)def->bmAttributes);
	IdentLine(ident,"wMaxPacketSize=%i",(int)def->wMaxPacketSize);
	IdentLine(ident,"bInterval=%i",(int)def->bInterval);
}


static void USBH_Print_HID_InterfaceDescTypeDef(int ident, USBH_InterfaceDescTypeDef* def){
	const char* str;
	USBH_Print_HID_USBH_USBH_DescHeader_t(ident++,(USBH_DescHeaderTypeDef*)def);
	IdentLine(ident,"bInterfaceNumber=%i",(int)def->bInterfaceNumber);
	IdentLine(ident,"bNumEndpoints=%i",(int)def->bNumEndpoints);
	str = pDeviceClass(def->bInterfaceClass);
	if(str != NULL)
		IdentLine(ident,"bInterfaceClass(%s)=%i",str,(int)def->bInterfaceClass);
	else
		IdentLine(ident,"bInterfaceClass=%i",(int)def->bInterfaceClass);
	str = pDeviceSubClass(def->bInterfaceClass,def->bInterfaceSubClass);
		if(str != NULL)
			IdentLine(ident,"bInterfaceSubClass(%s)=%i",str,(int)def->bInterfaceSubClass);
		else
			IdentLine(ident,"bInterfaceSubClass=%i",(int)def->bInterfaceSubClass);
	IdentLine(ident,"bInterfaceProtocol=%i",(int)def->bInterfaceProtocol);
	IdentLine(ident,"iInterface=%i",(int)def->iInterface);
	for(uint8_t i=0;i < def->bNumEndpoints;i++)
		USBH_Print_HID_EpDescTypeDef(ident,&def->Ep_Desc[i]);
}
static void USBH_Print_HID_CfgDescTypeDef(int ident, USBH_CfgDescTypeDef* def){
	USBH_Print_HID_USBH_USBH_DescHeader_t(ident++,(USBH_DescHeaderTypeDef*)def);
	IdentLine(ident,"wTotalLength=%i",(int)def->wTotalLength);
	IdentLine(ident,"bNumInterfaces=%i",(int)def->bNumInterfaces);
	IdentLine(ident,"bConfigurationValue=%i",(int)def->bConfigurationValue);
	IdentLine(ident,"iConfiguration=%i",(int)def->iConfiguration);
	IdentLine(ident,"bmAttributes=%i",(int)def->bmAttributes);
	IdentLine(ident,"bMaxPower=%i",(int)def->bMaxPower);
	for(uint8_t i=0;i < def->bNumInterfaces;i++)
		USBH_Print_HID_InterfaceDescTypeDef(ident,&def->Itf_Desc[i]);
}
static void USBH_Print_HID_USBH_USBH_DeviceTypeDef(int ident, USBH_DeviceTypeDef* dev){
	IdentLine(ident++,"USBH_DeviceTypeDef");
	IdentLine(ident,"address=%i",(int)dev->address);
	IdentLine(ident,"speed=%i",(int)dev->speed);
	IdentLine(ident,"is_connected=%i",(int)dev->is_connected);
	IdentLine(ident,"current_interface=%i",(int)dev->current_interface);
	USBH_Print_HID_DevDescTypeDef(ident, dev->DevDesc);
	USBH_Print_HID_CfgDescTypeDef(ident, dev->CfgDesc);
}

HID_HandleTypeDef* USBH_HID_CurrentHandle(USBH_HandleTypeDef *phost){
	HID_HandleTypeDef *HID_Handles=(HID_HandleTypeDef *)phost->pActiveClass->pData;
	if(HID_Handles==NULL) return NULL;
	return &HID_Handles[phost->device.current_interface];

}

static USBH_StatusTypeDef USBH_HID_DebugPrint(USBH_HandleTypeDef *phost){
	HID_HandleTypeDef* HID_Handles = (HID_HandleTypeDef*)phost->pActiveClass->pData ;
	USBH_Print_HID_USBH_USBH_DeviceTypeDef(0,&phost->device);
	if(HID_Handles) USBH_Print_HID_DescTypeDef(0,&HID_Handles[0].HID_Desc);
	///if(HID_Handles[0].HIDReport) // print?
	// HID_Handle->HIDReport = USBH_HID_AllocHIDReport(phost->device.Data, HID_Handle->HID_Desc.wItemLength);



}
static USBH_StatusTypeDef USBH_HID_InterfaceInit(USBH_HandleTypeDef *phost)
{	
	USBH_StatusTypeDef status = USBH_FAIL ;
	HID_HandleTypeDef* HID_Handles = NULL;
	uint8_t max_interfaces = phost->device.CfgDesc->bNumInterfaces;
	uint32_t total_handle_size = max_interfaces * sizeof(USBH_HandleTypeDef);
	USBH_UsrLog("This device has %i Interfaces" ,max_interfaces);
	for(uint32_t interface = 0; interface < max_interfaces;interface++){
		uint8_t max_ep = phost->device.CfgDesc->Itf_Desc[interface].bNumEndpoints;
		uint32_t max_packet_size=0;
		for(uint32_t i=0;i < max_ep;i++){
			uint32_t l = phost->device.CfgDesc->Itf_Desc[interface].Ep_Desc[0].wMaxPacketSize;
			if(l > max_packet_size) max_packet_size=l;
		}
		total_handle_size+=max_packet_size; // for the sending buffer
		total_handle_size+=(max_packet_size*HID_QUEUE_SIZE); // for the fifo queue
	}
	uint8_t* buffer = USBH_malloc(total_handle_size);
	memset(buffer,0,total_handle_size);

  	HID_Handles = (HID_HandleTypeDef*)buffer;
  	buffer+=(max_interfaces * sizeof(USBH_HandleTypeDef));
  	// cycle though again to init evetying
  	for(uint32_t interface = 0; interface < max_interfaces;interface++){
  		HID_HandleTypeDef* HID_Handle = &HID_Handles[interface];
  		HID_Handle->phost = phost;
  		HID_Handle->Interface = interface;
  		HID_Handle->state     = HID_INIT;
  		HID_Handle->ctl_state = HID_REQ_INIT;
  		HID_Handle->ep_addr   = phost->device.CfgDesc->Itf_Desc[interface].Ep_Desc[0].bEndpointAddress;
  		HID_Handle->length    = phost->device.CfgDesc->Itf_Desc[interface].Ep_Desc[0].wMaxPacketSize;
  		HID_Handle->poll      = phost->device.CfgDesc->Itf_Desc[interface].Ep_Desc[0].bInterval ;
  		HID_Handle->pData	  = buffer; // this is the current packet data, copied to the fifo
  		buffer+= (HID_Handle->length); // move ot the next interface buffer
  		// init the fifo here since all the driveers use it
  		fifo_init(&HID_Handle->fifo, buffer, HID_Handle->length*HID_QUEUE_SIZE);
  		buffer+= (HID_Handle->length*HID_QUEUE_SIZE); // move passed the interface buffer
  		/* Check fo available number of endpoints */
  		/* Find the number of EPs in the Interface Descriptor */
  		/* Choose the lower number in order not to overrun the buffer allocated */
  		uint8_t max_ep = ( (phost->device.CfgDesc->Itf_Desc[phost->device.current_interface].bNumEndpoints <= USBH_MAX_NUM_ENDPOINTS) ?
  				  phost->device.CfgDesc->Itf_Desc[phost->device.current_interface].bNumEndpoints :
  					  USBH_MAX_NUM_ENDPOINTS);


  		/* Decode endpoint IN and OUT address from interface descriptor */
  		for (uint32_t num=0 ;num < max_ep; num++)
  		{
  		if(phost->device.CfgDesc->Itf_Desc[phost->device.current_interface].Ep_Desc[num].bEndpointAddress & 0x80)
  		{
  			HID_Handle->InEp = (phost->device.CfgDesc->Itf_Desc[phost->device.current_interface].Ep_Desc[num].bEndpointAddress);
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
  			HID_Handle->OutEp = (phost->device.CfgDesc->Itf_Desc[phost->device.current_interface].Ep_Desc[num].bEndpointAddress);
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
  		printf("Pipe I=%i IN=%i OUT=%i\r\n",HID_Handle->Interface, HID_Handle->InPipe, HID_Handle->OutPipe);
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
	for(uint32_t interface=0; interface < phost->device.CfgDesc->bNumInterfaces;interface++) {
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
	  	  if(HID_Handle->HIDReport) {
			USBH_free(HID_Handle->HIDReport);
	  		HID_Handle->HIDReport = NULL;
	  	  }

	}
	USBH_free (phost->pActiveClass->pData);
	phost->pActiveClass->pData=NULL;
  return USBH_OK;
}

USBH_StatusTypeDef USBH_HID_ClassInterfaceRequest(USBH_HandleTypeDef *phost, uint16_t interface){
	USBH_StatusTypeDef status         = USBH_BUSY;
	USBH_StatusTypeDef classReqStatus = USBH_BUSY;
	HID_HandleTypeDef *HID_Handles =  (HID_HandleTypeDef *)phost->pActiveClass->pData;
	HID_HandleTypeDef *HID_Handle =  &HID_Handles[phost->device.current_interface];

	  //USBH_HID_CurrentHandle(phost);

	  /* Switch HID state machine */
	  switch (HID_Handle->ctl_state)
	  {
	  case HID_REQ_INIT:
	  case HID_REQ_GET_HID_DESC:
	    /* Get HID Desc */
	    if (USBH_HID_GetHIDDescriptor (phost, phost->device.current_interface,USBH_MAX_DATA_BUFFER)== USBH_OK)
	    {
	    	 USBH_HID_ParseHIDDesc(&HID_Handle->HID_Desc, phost->device.Data);
	    	 USBH_Print_HID_DescTypeDef(0,&HID_Handles[0].HID_Desc);
	    	 HID_Handle->ctl_state = HID_REQ_GET_REPORT_DESC;
	    }
	    break;
	  case HID_REQ_GET_REPORT_DESC:
		  if(USBH_HID_GetHIDReportDescriptor(phost, phost->device.current_interface, HID_Handle->HID_Desc.wItemLength) == USBH_OK)
		  {
			  HID_Handle->HIDReport = USBH_HID_AllocHIDReport(phost->device.Data, HID_Handle->HID_Desc.wItemLength);

			  // We don't know what this thing is till we get to this point
			// should be the first fifo write
			for(uint32_t i=0; i < driver_count;i++){
				if(HID_Drivers[i](HID_Handle, phost->device.Data) == USBH_OK) break;
			}
			if(HID_Handle->InterruptReceiveCallback){
				USBH_UsrLog("HID Driver installed");
				HID_Handle->ctl_state = HID_REQ_SET_PROTOCOL;
			} else {
				USBH_UsrLog(" No HID Driver found");
				HID_Handle->ctl_state = HID_REQ_IDLE;
				status = USBH_FAIL;
			}

		  }
	    break;
	  case HID_REQ_SET_PROTOCOL:
	  	    /* set protocol */
	  		if (USBH_HID_SetProtocol (phost, 1) == USBH_OK)
	  	  //  if (USBH_HID_SetProtocol (phost, 0) == USBH_OK) // not boot protocal anymore
	  	    {
	  	      HID_Handle->ctl_state = HID_REQ_SET_IDLE;

	  	      /* all requests performed*/
	  	      phost->pUser(phost, HOST_USER_CLASS_ACTIVE);
	  	      USBH_UsrLog("USBH_HID_SetProtocol success!");

	  	    }
	  	    break;
	  case HID_REQ_SET_IDLE:

			  classReqStatus = USBH_HID_SetIdle (phost, 0, 0);
		  if (classReqStatus == USBH_OK)
		  {
			  USBH_UsrLog("USBH_HID_SetIdle success!");
			  HID_Handle->ctl_state = HID_REQ_IDLE;
			  status = USBH_OK;
		  }
		  else if(classReqStatus == USBH_NOT_SUPPORTED)
		  {
			 USBH_UsrLog("USBH_HID_SetIdle fail!");
		     HID_Handle->ctl_state = HID_REQ_IDLE;
		     status = USBH_OK;
		  }

	    break;
	  case HID_REQ_IDLE:
	  default:
	    break;
	  }

	  return status;
}
bool debug_print = false;
int interface_state = 0;
USBH_StatusTypeDef USBH_HID_ClassRequest(USBH_HandleTypeDef *phost)
{   
  USBH_StatusTypeDef status         = USBH_BUSY;

  if(!debug_print){
	  USBH_Print_HID_USBH_USBH_DeviceTypeDef(0,&phost->device);
	  debug_print = true;
	  interface_state = 0;
	  USBH_SelectInterface(phost,0);
  }
  status = USBH_HID_ClassInterfaceRequest(phost,phost->device.current_interface);
  if(status != USBH_BUSY){
	  if(phost->device.current_interface < phost->device.CfgDesc->bNumInterfaces){
		  USBH_SelectInterface(phost,phost->device.current_interface+1);
		  status = USBH_BUSY;
	  }else {
		  USBH_SelectInterface(phost,0); // switch back
	  }
  }


  return status; 
}

static USBH_StatusTypeDef USBH_HID_ProcessPerInterface(USBH_HandleTypeDef *phost, uint16_t interface){
	HID_HandleTypeDef *HID_Handles =  (HID_HandleTypeDef *)phost->pActiveClass->pData;
	HID_HandleTypeDef *HID_Handle =  &HID_Handles[interface];

		USBH_StatusTypeDef status = USBH_OK;
		switch (HID_Handle->state)
		{
		  case HID_INIT:
			if(HID_Handle->Init) HID_Handle->Init(HID_Handle);
			HID_Handle->state = HID_SYNC;
			break;
		  case HID_SYNC:

			/* Sync with start of Even Frame */
			if(phost->Timer & 1)
			{
			  HID_Handle->state = HID_GET_DATA;
			}
		#if (USBH_USE_OS == 1)
			osMessagePut ( phost->os_event, USBH_URB_EVENT, 0);
		#endif
			break;

		  case HID_GET_DATA:

			USBH_InterruptReceiveData(phost,
									  HID_Handle->pData,
									  HID_Handle->length,
									  HID_Handle->InPipe);

			HID_Handle->state = HID_POLL;
			HID_Handle->timer = phost->Timer;
			HID_Handle->DataReady = 0;
			break;

		  case HID_POLL:

			if(USBH_LL_GetURBState(phost , HID_Handle->InPipe) == USBH_URB_DONE)
			{
			  if(HID_Handle->DataReady == 0)
			  {
				fifo_write(&HID_Handle->fifo, HID_Handle->pData, HID_Handle->length);
				HID_Handle->DataReady = 1;
				if(HID_Handle->InterruptReceiveCallback) HID_Handle->InterruptReceiveCallback(HID_Handle);
				USBH_HID_EventCallback(phost,HID_Handle);
		#if (USBH_USE_OS == 1)
			osMessagePut ( phost->os_event, USBH_URB_EVENT, 0);
		#endif
			  }
			}
			else if(USBH_LL_GetURBState(phost , HID_Handle->InPipe) == USBH_URB_STALL) /* IN Endpoint Stalled */
			{

			  /* Issue Clear Feature on interrupt IN endpoint */
			  if(USBH_ClrFeature(phost,
								 HID_Handle->ep_addr) == USBH_OK)
			  {
				/* Change state to issue next IN token */
				HID_Handle->state = HID_GET_DATA;
			  }
			}


			break;

		  default:
			break;
		  }
	  return status;
}
static USBH_StatusTypeDef USBH_HID_Process(USBH_HandleTypeDef *phost)
{

  USBH_StatusTypeDef status = USBH_OK;
  HID_HandleTypeDef *HID_Handles =  (HID_HandleTypeDef *) phost->pActiveClass->pData;
  for(uint8_t interface=0; interface < phost->device.CfgDesc->bNumInterfaces;interface++) {
	  HID_HandleTypeDef *HID_Handle =  &HID_Handles[interface];
	 // if(HID_Handle->Process)
		//  status = HID_Handle->Process(HID_Handle);
	 // else
		  status =USBH_HID_ProcessPerInterface(phost,interface);
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
	for(uint8_t interface=0; interface < phost->device.CfgDesc->bNumInterfaces;interface++) {
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
													uint16_t interface,    uint16_t length)
{
	/* HID report descriptor is available in phost->device.Data.
	In case of USB Boot Mode devices for In report handling ,
	HID report descriptor parsing is not required.
	In case, for supporting Non-Boot Protocol devices and output reports,
	user may parse the report descriptor*/
  USBH_StatusTypeDef status;
	  phost->Control.setup.b.bmRequestType = USB_D2H | USB_REQ_RECIPIENT_INTERFACE | USB_REQ_TYPE_STANDARD;
	  phost->Control.setup.b.bRequest = USB_REQ_GET_DESCRIPTOR;
	  phost->Control.setup.b.wValue.w = USB_DESC_HID_REPORT;
	  phost->Control.setup.b.wIndex.w = interface;
	  phost->Control.setup.b.wLength.w = length;
  return USBH_CtlReq(phost, phost->device.Data , length );
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
                                            uint16_t interface, uint16_t length)
{
	phost->Control.setup.b.bmRequestType = USB_D2H | USB_REQ_RECIPIENT_INTERFACE | USB_REQ_TYPE_STANDARD;
	phost->Control.setup.b.bRequest = USB_REQ_GET_DESCRIPTOR;
	phost->Control.setup.b.wValue.w = USB_DESC_HID;
	phost->Control.setup.b.wIndex.w = phost->device.current_interface;
	phost->Control.setup.b.wLength.w = length;
	return USBH_CtlReq(phost, phost->device.Data , length );
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
  
  phost->Control.setup.b.wIndex.w = phost->device.current_interface;
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
  
  phost->Control.setup.b.wIndex.w = phost->device.current_interface;
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
  
  phost->Control.setup.b.wIndex.w = phost->device.current_interface; // interface meh
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
  phost->Control.setup.b.wIndex.w = phost->device.current_interface;
  phost->Control.setup.b.wLength.w = 0;
  
  return USBH_CtlReq(phost, 0 , 0 );
  
}

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
