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

#include "usbh_ctlreq.h"
#include "usbh_pipes.h"

#include <assert.h>
#include "usart.h"




#define PRINT_BYTE_VALUE(DEV,NAME) printf("%*s%-18s = %i\r\n",ident,"", #NAME, (int)DEV->NAME);
#define PRINT_BCD_VALUE(DEV,NAME) printf("%*s%-18s = %2.2X%2.2X\r\n",ident,"", #NAME, DEV->NAME<<8, DEV->NAME & 0xFF);
#define PRINT_HEX_VALUE(DEV,NAME) printf("%*s%-18s = 0x%4.4X\r\n",ident,"", #NAME, (int)DEV->NAME);
#define PRINT_STRING_VALUE(DEV,NAME) printf("%*s%-18s = %s\r\n",ident,"", #NAME, ((const char*)DEV->NAME) ? ((const char*)DEV->NAME) : "N/A");
#define PRINT_STRING_START(DEV,NAME) {  printf("\%*s%-18s = 0x%4.4X {",ident,"", #NAME, (int)DEV->NAME);
#define PRINT_STRING(STRING) printf(" %s",(STRING));
#define PRINT_STRING_END printf(" }\r\n"); }
#define PRINT_STRING_HEADER(...) printf("%*s",ident,""); printf(__VA_ARGS__); printf("\r\n");

static const char* EndpointAttributeToString(uint8_t bmAttributes) {
	switch((bmAttributes & 0x03)){
	case 0x00:return "Control";
	case 0x01:
	{
		static char endpoint[128];
		endpoint[0]=0;
		strcat(endpoint,"Isochronous ");
		switch((bmAttributes >> 2) & 0x03){
			case 0x00:strcat(endpoint,"No Sync "); break;
			case 0x01:strcat(endpoint,"Async "); break;
			case 0x02:strcat(endpoint,"Adaptive "); break;
			case 0x03:strcat(endpoint,"Sync "); break;
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

USBH_StatusTypeDef USBH_Print_DevDesc(int ident, USBH_DevDescTypeDef *dev){
	assert(dev);
	PRINT_BCD_VALUE(dev,USB);
	PRINT_BYTE_VALUE(dev,DeviceClass);
	PRINT_BYTE_VALUE(dev,DeviceSubClass);
	PRINT_BYTE_VALUE(dev,DeviceProtocol);
	PRINT_BYTE_VALUE(dev,MaxPacketSize);
	PRINT_HEX_VALUE(dev,VendorID);
	PRINT_HEX_VALUE(dev,Product);
	PRINT_BCD_VALUE(dev,Device);
	PRINT_STRING_VALUE(dev,Manufacturer);
	PRINT_STRING_VALUE(dev,Product);
	PRINT_STRING_VALUE(dev,SerialNumber);
	PRINT_BYTE_VALUE(dev,NumConfigurations);
	return USBH_OK;
}
USBH_StatusTypeDef USBH_Print_EndpointDesc(int ident, USBH_EpDescTypeDef *epp){
	if((epp->Attributes & 0x03)!=0) {
		PRINT_STRING_START(epp,EndpointAddress);
			PRINT_STRING((epp->EndpointAddress & 0x80) ? "IN": "OUT");
		PRINT_STRING_END;
	} else {
		PRINT_BYTE_VALUE(epp,EndpointAddress); // control end point
	}
	PRINT_STRING_START(epp,Attributes)
		PRINT_STRING(EndpointAttributeToString(epp->Attributes));
	PRINT_STRING_END;

	PRINT_BYTE_VALUE(epp,MaxPacketSize);
	PRINT_BYTE_VALUE(epp,Interval);
	return USBH_OK;
}

USBH_StatusTypeDef USBH_Print_InterfaceDesc(int ident, USBH_InterfaceDescTypeDef *iface){
	PRINT_BYTE_VALUE(iface,InterfaceNumber);
	PRINT_BYTE_VALUE(iface,AlternateSetting);
	PRINT_BYTE_VALUE(iface,NumEndpoints);
	PRINT_BYTE_VALUE(iface,InterfaceClass);
	PRINT_BYTE_VALUE(iface,InterfaceSubClass);
	PRINT_BYTE_VALUE(iface,InterfaceProtocol);
	PRINT_STRING_VALUE(iface,Interface);
	return USBH_OK;
}

USBH_StatusTypeDef USBH_Print_CfgDesc(int ident, USBH_CfgDescTypeDef *dev){
	assert(dev);
	PRINT_BYTE_VALUE(dev,NumInterfaces);
	PRINT_BYTE_VALUE(dev,ConfigurationValue);
	PRINT_STRING_VALUE(dev,Configuration);
	PRINT_STRING_START(dev,Attributes)
		if(dev->Attributes & 0x80) PRINT_STRING("BUS_POWERED");
		if(dev->Attributes & 0x40) PRINT_STRING("SELF_POWERED");
		if(dev->Attributes & 0x20) PRINT_STRING("REMOTE_WAKEUP");
	PRINT_STRING_END;
	PRINT_BYTE_VALUE(dev,MaxPower);
	return USBH_OK;
}

USBH_StatusTypeDef USBH_Print_FullCfgDesc(int ident, USBH_CfgDescTypeDef *dev){
	USBH_Print_CfgDesc(ident, dev);
	ident+=3;
	assert(dev->NumInterfaces < 4);
	for(int interface=0; interface < dev->NumInterfaces;interface++) {
		PRINT_STRING_HEADER("Interface Descriptor(%i)", interface);
		ident +=3;
		USBH_Print_InterfaceDesc(ident, &dev->Interfaces[interface]);
		assert(dev->Interfaces[interface].NumEndpoints < 4);
		for(int ep=0; ep <dev->Interfaces[interface].NumEndpoints;ep++) {
			PRINT_STRING_HEADER("Endpoint Descriptor(%i)", ep);
			ident+=3;
			USBH_Print_EndpointDesc(ident,&dev->Interfaces[interface].Endpoints[ep]);
			ident-=3;
		}
		ident -=3;
	}
	return USBH_OK;
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
USBH_StatusTypeDef USBH_GetDescriptor(USBH_HandleTypeDef *phost,                          
		 	 	 	 	 	   uint8_t  req_type,
		                       uint16_t value,
							   uint16_t index,
		                       uint8_t* buff,
                               uint16_t length )
{ 

	if(phost->Control.state == CTRL_SETUP){
		phost->Control.Init.RequestType = USB_D2H | req_type;
		phost->Control.Init.Request  	= USB_REQ_GET_DESCRIPTOR;
		phost->Control.Init.Value  		= value;
		phost->Control.Init.Index  		= index;
		phost->Control.Init.Length  	= length;
		phost->Control.Init.Data  		= buff;
	}
	return USBH_CtlReq(phost);
}

/**
  * @brief  USBH_SetAddress
  *         This command sets the address to the connected device
  * @param  phost: Host Handle
  * @param  DeviceAddress: Device address to assign
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_SetAddress(USBH_HandleTypeDef *phost, 
                                   uint8_t DeviceAddress)
{
	if(phost->Control.state == CTRL_SETUP){
		phost->Control.Init.RequestType = USB_H2D | USB_REQ_RECIPIENT_DEVICE |USB_REQ_TYPE_STANDARD;
		phost->Control.Init.Request  	= USB_REQ_SET_ADDRESS;
		phost->Control.Init.Value  		= (uint16_t)DeviceAddress;
		phost->Control.Init.Index  		= 0;
		phost->Control.Init.Length  	= 0;
		phost->Control.Init.Data  		= NULL;
	}
	return USBH_CtlReq(phost);
}


/**
  * @brief  USBH_SetCfg
  *         The command sets the configuration value to the connected device
  * @param  phost: Host Handle
  * @param  cfg_idx: Configuration value
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_SetCfg(USBH_HandleTypeDef *phost, 
                               uint16_t cfg_idx)
{
	if(phost->Control.state == CTRL_SETUP){
		phost->Control.Init.RequestType = USB_H2D | USB_REQ_RECIPIENT_DEVICE |USB_REQ_TYPE_STANDARD;
		phost->Control.Init.Request  	= USB_REQ_SET_CONFIGURATION;
		phost->Control.Init.Value  		= cfg_idx;
		phost->Control.Init.Index  		= 0;
		phost->Control.Init.Length  	= 0;
		phost->Control.Init.Data  		= NULL;
	}
	return USBH_CtlReq(phost);
}

/**
  * @brief  USBH_SetInterface
  *         The command sets the Interface value to the connected device
  * @param  phost: Host Handle
  * @param  altSetting: Interface value
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_SetInterface(USBH_HandleTypeDef *phost, 
                        uint8_t ep_num, uint8_t altSetting)
{
	if(phost->Control.state == CTRL_SETUP){
		phost->Control.Init.RequestType = USB_H2D | USB_REQ_RECIPIENT_INTERFACE |USB_REQ_TYPE_STANDARD;
		phost->Control.Init.Request  	= USB_REQ_SET_INTERFACE;
		phost->Control.Init.Value  		= altSetting;
		phost->Control.Init.Index  		= 0;
		phost->Control.Init.Length  	= 0;
		phost->Control.Init.Data  		= NULL;
	}
	return USBH_CtlReq(phost);
}

/**
  * @brief  USBH_ClrFeature
  *         This request is used to clear or disable a specific feature.
  * @param  phost: Host Handle
  * @param  ep_num: endpoint number 
  * @param  hc_num: Host channel number 
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_ClrFeature(USBH_HandleTypeDef *phost, uint8_t ep_num)
{
	if(phost->Control.state == CTRL_SETUP){
		phost->Control.Init.RequestType = USB_H2D | USB_REQ_RECIPIENT_ENDPOINT |USB_REQ_TYPE_STANDARD;
		phost->Control.Init.Request  	= USB_REQ_CLEAR_FEATURE;
		phost->Control.Init.Value  		= FEATURE_SELECTOR_ENDPOINT;
		phost->Control.Init.Index  		= ep_num;
		phost->Control.Init.Length  	= 0;
		phost->Control.Init.Data  		= NULL;
	}
	return USBH_CtlReq(phost);
}

const char* URBStateToString(USBH_URBStateTypeDef t){
#define TO_STRING(ENUM) #ENUM
	static const char* tbl[USBH_STATE_MAX] = {
		TO_STRING(USBH_URB_IDLE),
		TO_STRING(USBH_URB_DONE),
		TO_STRING(USBH_URB_NOTREADY),
		TO_STRING(USBH_URB_NYET),
		TO_STRING(USBH_URB_ERROR),
		TO_STRING(USBH_URB_STALL),
	};
#undef TO_STRING
	return tbl[t];
}

const char* CTRLStateToString(CTRL_StateTypeDef t){
#define TO_STRING(ENUM) #ENUM
	static const char* tbl[CTRL_MAX_STATES] = {
		TO_STRING(CTRL_SETUP),
		TO_STRING(CTRL_DATA_IN),
		TO_STRING(CTRL_DATA_OUT),
		TO_STRING(CTRL_STATUS_IN),
		TO_STRING(CTRL_STATUS_OUT),
		TO_STRING(CTRL_ERROR),
		TO_STRING(CTRL_STALLED),
		TO_STRING(CTRL_COMPLETE)
	};
#undef TO_STRING
	return tbl[t];
}
static void USBStateCallback(USBH_HandleTypeDef *phost,USBH_PipeHandleTypeDef* pipe){

	switch(pipe->urb_state){
	case USBH_URB_IDLE:
		break;
	case USBH_URB_DONE:
		//phost->Control.state = phost->Control.next_state;
		break;
	case USBH_URB_STALL:

		USBH_ErrLog("USBH_URB_STALL(%i)  Last State=%s Current_State=%s",(int)pipe->Pipe,
				CTRLStateToString(phost->Control.prev_state),CTRLStateToString(phost->Control.state));
		phost->Control.state = CTRL_STALLED;
		break;
	case USBH_URB_NOTREADY: // NAK
		USBH_ErrLog("USBH_URB_NOTREADY(%i)  Last State=%s Current_State=%s",(int)pipe->Pipe,
					CTRLStateToString(phost->Control.prev_state),CTRLStateToString(phost->Control.state));
		phost->Control.state = phost->Control.prev_state;
		break;
	case USBH_URB_ERROR: // NAK
		USBH_ErrLog("USBH_URB_ERROR(%i)  Last State=%s Current_State=%s",(int)pipe->Pipe,
						CTRLStateToString(phost->Control.prev_state),CTRLStateToString(phost->Control.state));
		phost->Control.state = CTRL_ERROR;
		break;
	case USBH_URB_NYET: // NYET?
		USBH_ErrLog("USBH_URB_NYET(%i)  Last State=%s Current_State=%s",(int)pipe->Pipe,
							CTRLStateToString(phost->Control.prev_state),CTRLStateToString(phost->Control.state));
		phost->Control.state = CTRL_ERROR;
				break;
	default:
		break;
	}
}

#define CONTROL_LC phost->Control.lc
#define PIPE_THREAD_NAME(NAME) NAME##_PIPEThread
#define PIPE_THREAD_BEGIN(NAME) char PIPE_THREAD_NAME(NAME)(USBH_HandleTypeDef*phost) { LC_RESUME(CONTROL_LC);
#define PIPE_THREAD_END() LC_END(CONTROL_LC); LC_INIT(CONTROL_LC); return USB_ENDED; }
#define PIPE_EXIT() do{ LC_INIT(CONTROL_LC); return USB_EXITED;  } while(0)
#define PIPE_WAIT_UNTILL(condition) do { LC_SET(CONTROL_LC); if(!(condition))return USB_WAITING; } while(0)
#define PIPE_WAIT_WHILE(cond)  PIPE_WAIT_UNTILL(!(cond))
#define PIPE_WAIT_UTILL_IDLE(PIPE) PIPE_WAIT_UNTILL((PIPE)->state == PIPE_IDLE); if((PIPE)->urb_state != USBH_URB_DONE) return USB_EXITED;
#define PIPE_SCHEDULE(f) ((f) < USB_EXITED)

void CtlSetup(USBH_HandleTypeDef *phost){
	phost->Control.timer = phost->Timer;
	phost->Control.pipe_in->Callback = USBStateCallback;
	phost->Control.pipe_out->Callback = USBStateCallback; // set the callbacks
	phost->Control.pipe_in->Init.EpType  = PIPE_EP_CONTROL;
	phost->Control.pipe_out->Init.EpType  = PIPE_EP_CONTROL; // and the types


	phost->Control.setup.b.bmRequestType = phost->Control.Init.RequestType;
	phost->Control.setup.b.bRequest = phost->Control.Init.Request;
	phost->Control.setup.b.wValue.w = phost->Control.Init.Value;
	phost->Control.setup.b.wIndex.w = phost->Control.Init.Index;
	phost->Control.setup.b.wLength.w = phost->Control.Init.Length;
	// set up the packet
	phost->Control.pipe_out->Init.DataType = PIPE_PID_SETUP;
	phost->Control.pipe_out->Size = USBH_SETUP_PKT_SIZE;
	phost->Control.pipe_out->Data = (uint8_t *)phost->Control.setup.d8;

	uint8_t direction = (phost->Control.setup.b.bmRequestType & USB_REQ_DIR_MASK);
	/* check if there is a data stage */
	if (phost->Control.setup.b.wLength.w != 0 )
		phost->Control.state = direction == USB_D2H ? CTRL_DATA_IN : CTRL_DATA_OUT;
	else
		phost->Control.state = direction == USB_D2H ? CTRL_STATUS_OUT : CTRL_STATUS_IN;
}
PIPE_THREAD_BEGIN(Control_Setup_Blocking)
phost->Control.timer = phost->Timer;
	phost->Control.state = CTRL_SETUP;
	phost->Control.pipe_in->Init.EpType  = PIPE_EP_CONTROL;
	phost->Control.pipe_out->Init.EpType  = PIPE_EP_CONTROL; // and the types
	// set up the request
	phost->Control.setup.b.bmRequestType = phost->Control.Init.RequestType;
	phost->Control.setup.b.bRequest = phost->Control.Init.Request;
	phost->Control.setup.b.wValue.w = phost->Control.Init.Value;
	phost->Control.setup.b.wIndex.w = phost->Control.Init.Index;
	phost->Control.setup.b.wLength.w = phost->Control.Init.Length;
	// set up the packet
	phost->Control.pipe_out->Init.DataType = PIPE_PID_SETUP;
	phost->Control.pipe_out->Size = USBH_SETUP_PKT_SIZE;
	phost->Control.pipe_out->Data = (uint8_t *)phost->Control.setup.d8;
	// submit it
	assert(USBH_PipeWait(phost,phost->Control.pipe_out)==USBH_OK);

	if (phost->Control.setup.b.wLength.w != 0 ){ // status
		if((phost->Control.setup.b.bmRequestType & USB_REQ_DIR_MASK)== USB_D2H) { // CTRL_DATA_IN
			phost->Control.state = CTRL_DATA_IN;
			phost->Control.pipe_in->Size = phost->Control.length;
			phost->Control.pipe_in->Data = phost->Control.data;
			phost->Control.pipe_in->Init.DataType = PIPE_PID_DATA;
			assert(USBH_PipeWait(phost,phost->Control.pipe_in)==USBH_OK);
		}else { // CTRL_DATA_OUT
			phost->Control.state = CTRL_DATA_OUT;
			phost->Control.pipe_out->Size = phost->Control.length;
			phost->Control.pipe_out->Data = phost->Control.data;
			phost->Control.pipe_out->Init.DataType = PIPE_PID_DATA;
			assert(USBH_PipeWait(phost,phost->Control.pipe_out)==USBH_OK);
		}
	}
	if((phost->Control.setup.b.bmRequestType & USB_REQ_DIR_MASK)== USB_D2H) { // CTRL_STATUS_OUT
		phost->Control.state = CTRL_STATUS_OUT;
		phost->Control.pipe_out->Size = 0;
		phost->Control.pipe_out->Data = 0;
		phost->Control.pipe_out->Init.DataType = PIPE_PID_DATA;
		assert(USBH_PipeWait(phost,phost->Control.pipe_out)==USBH_OK);
	}else { // CTRL_STATUS_IN
		phost->Control.state = CTRL_STATUS_IN;
		phost->Control.pipe_in->Size = 0;
		phost->Control.pipe_in->Data = 0;
		phost->Control.pipe_in->Init.DataType = PIPE_PID_DATA;
		assert(USBH_PipeWait(phost,phost->Control.pipe_in)==USBH_OK);
	}
	phost->Control.state = CTRL_COMPLETE;


PIPE_THREAD_END()

PIPE_THREAD_BEGIN(Control_Setup)
	phost->Control.timer = phost->Timer;
	phost->Control.state = CTRL_SETUP;
	phost->Control.pipe_in->Init.EpType  = PIPE_EP_CONTROL;
	phost->Control.pipe_out->Init.EpType  = PIPE_EP_CONTROL; // and the types
	// set up the request
	phost->Control.setup.b.bmRequestType = phost->Control.Init.RequestType;
	phost->Control.setup.b.bRequest = phost->Control.Init.Request;
	phost->Control.setup.b.wValue.w = phost->Control.Init.Value;
	phost->Control.setup.b.wIndex.w = phost->Control.Init.Index;
	phost->Control.setup.b.wLength.w = phost->Control.Init.Length;
	// set up the packet
	phost->Control.pipe_out->Init.DataType = PIPE_PID_SETUP;
	phost->Control.pipe_out->Size = USBH_SETUP_PKT_SIZE;
	phost->Control.pipe_out->Data = (uint8_t *)phost->Control.setup.d8;
	// submit it
	USBH_PipeStartup(phost,phost->Control.pipe_out);
	PIPE_WAIT_UTILL_IDLE(phost->Control.pipe_out);

	if (phost->Control.setup.b.wLength.w != 0 ){ // status
		if((phost->Control.setup.b.bmRequestType & USB_REQ_DIR_MASK)== USB_D2H) { // CTRL_DATA_IN
			phost->Control.state = CTRL_DATA_IN;
			phost->Control.pipe_in->Size = phost->Control.length;
			phost->Control.pipe_in->Data = phost->Control.data;
			phost->Control.pipe_in->Init.DataType = PIPE_PID_DATA;
			PIPE_WAIT_UTILL_IDLE(phost->Control.pipe_in);
		}else { // CTRL_DATA_OUT
			phost->Control.state = CTRL_DATA_OUT;
			phost->Control.pipe_out->Size = phost->Control.length;
			phost->Control.pipe_out->Data = phost->Control.data;
			phost->Control.pipe_out->Init.DataType = PIPE_PID_DATA;
			PIPE_WAIT_UTILL_IDLE(phost->Control.pipe_out);
		}
	}
	if((phost->Control.setup.b.bmRequestType & USB_REQ_DIR_MASK)== USB_D2H) { // CTRL_STATUS_OUT
		phost->Control.state = CTRL_STATUS_OUT;
		phost->Control.pipe_out->Size = 0;
		phost->Control.pipe_out->Data = 0;
		phost->Control.pipe_out->Init.DataType = PIPE_PID_DATA;
		PIPE_WAIT_UTILL_IDLE(phost->Control.pipe_out);
	}else { // CTRL_STATUS_IN
		phost->Control.state = CTRL_STATUS_IN;
		phost->Control.pipe_in->Size = 0;
		phost->Control.pipe_in->Data = 0;
		phost->Control.pipe_in->Init.DataType = PIPE_PID_DATA;
		PIPE_WAIT_UTILL_IDLE(phost->Control.pipe_in);
	}
	phost->Control.state = CTRL_COMPLETE;
PIPE_THREAD_END()

USBH_StatusTypeDef USBH_CtlReq(USBH_HandleTypeDef *phost)
{
	if(phost->Control.state == CTRL_IDLE) LC_INIT(CONTROL_LC);
	if(PIPE_SCHEDULE(PIPE_THREAD_NAME(Control_Setup_Blocking)(phost))) return USBH_BUSY;
	return phost->Control.state == CTRL_COMPLETE ?  USBH_OK : USBH_FAIL;
}

USBH_StatusTypeDef USBH_CtlReq2(USBH_HandleTypeDef *phost)
{
	if(phost->Control.state == CTRL_IDLE) LC_INIT(CONTROL_LC);
	if(PIPE_SCHEDULE(PIPE_THREAD_NAME(Control_Setup)(phost))) return USBH_BUSY;
	return phost->Control.state == CTRL_COMPLETE ?  USBH_OK : USBH_FAIL;
}
void Ctl_DataIn(USBH_HandleTypeDef *phost){
	phost->Control.timer = phost->Timer;
	phost->Control.pipe_in->Size = phost->Control.length;
	phost->Control.pipe_in->Data = phost->Control.data;
	phost->Control.pipe_in->Init.DataType = PIPE_PID_DATA;
	phost->Control.state= CTRL_STATUS_OUT;
}
void Ctl_DataOut(USBH_HandleTypeDef *phost){
	phost->Control.timer = phost->Timer;
	phost->Control.pipe_out->Size = phost->Control.length;
	phost->Control.pipe_out->Data = phost->Control.data;
	phost->Control.pipe_out->Init.DataType = PIPE_PID_DATA;
	phost->Control.state= CTRL_STATUS_IN;
}
void Ctl_StatusIn(USBH_HandleTypeDef *phost){
	phost->Control.timer = phost->Timer;
	phost->Control.pipe_in->Size = 0;
	phost->Control.pipe_in->Data = 0;
	phost->Control.pipe_in->Init.DataType = PIPE_PID_DATA;
	phost->Control.state=  CTRL_COMPLETE;
}
void Ctl_StatusOut(USBH_HandleTypeDef *phost){
	phost->Control.timer = phost->Timer;
	phost->Control.pipe_out->Size = 0;
	phost->Control.pipe_out->Data = 0;
	phost->Control.pipe_out->Init.DataType = PIPE_PID_DATA;
	phost->Control.state=  CTRL_COMPLETE;
}
USBH_StatusTypeDef USBH_CtlReq_Blocking(USBH_HandleTypeDef *phost){
	return USBH_BUSY;
}
USBH_StatusTypeDef USBH_CtlReq_old(USBH_HandleTypeDef *phost)
{
	if(phost->port_state != HOST_PORT_IDLE) return USBH_BUSY;
	if(USBH_PipeStartup(phost,phost->Control.pipe_out) == USBH_BUSY) return USBH_BUSY;
	if(USBH_PipeStartup(phost,phost->Control.pipe_in) == USBH_BUSY) return USBH_BUSY;
	assert(USBH_PipeStartup(phost,phost->Control.pipe_in) == USBH_FAIL);
	assert(USBH_PipeStartup(phost,phost->Control.pipe_out) == USBH_FAIL);
	USBH_StatusTypeDef status=USBH_BUSY;
	switch(phost->Control.state) {
	case CTRL_SETUP:
		CtlSetup(phost);
		USBH_PipeStartup(phost,phost->Control.pipe_out);
		break;
	case CTRL_DATA_IN:
		Ctl_DataIn(phost);
		USBH_PipeStartup(phost,phost->Control.pipe_in);
		break;
	case CTRL_DATA_OUT:
		Ctl_DataOut(phost);
		USBH_PipeStartup(phost,phost->Control.pipe_out);
		break;
	case CTRL_STATUS_IN:
		Ctl_StatusIn(phost);
		USBH_PipeStartup(phost,phost->Control.pipe_in);
		break;
	case CTRL_STATUS_OUT:
		Ctl_StatusOut(phost);
		USBH_PipeStartup(phost,phost->Control.pipe_out);
		break;
	case CTRL_COMPLETE:
		phost->Control.state = CTRL_SETUP;
		status= USBH_OK;
		break;
	case CTRL_STALLED:
	// a stall on a previous out means the request we sent returned bad so
		// lets return that
		//USBH_UsrLog("USBH_CtlReq %s", CTRLStateToString(phost->Control.prev_state));
		//CTRL_DATA_IN
		if(phost->Control.prev_state == CTRL_DATA_IN) { // stall at data in so its bad request we sent
			status =  USBH_FAIL;
			phost->Control.state = CTRL_SETUP; // restart the mess?
		}
		if (++ phost->Control.errorcount > USBH_MAX_ERROR_COUNT){
			USBH_ErrLog("Hard Stall");
			while(1);
		} else {
			print_buffer(phost->Control.data,phost->Control.length > 10 ? 10 : phost->Control.length);
		}

		break;
	default:
	case CTRL_ERROR:
		 /*
		After a halt condition is encountered or an error is detected by the
		host, a control endpoint is allowed to recover by accepting the next Setup
		PID; i.e., recovery actions via some other pipe are not required for control
		endpoints. For the Default Control Pipe, a device reset will ultimately be
		required to clear the halt or error condition if the next Setup PID is not
		accepted.
		*/
		if (++ phost->Control.errorcount <= USBH_MAX_ERROR_COUNT)
		{
		  /* try to recover control */
			USBH_LL_Stop(phost);
			phost->Control.state = CTRL_SETUP; // restart the mess?
		  /* Do the transmission again, starting from SETUP Packet */
		}
		else
		{
			if(phost->pUser) phost->pUser(phost, HOST_USER_UNRECOVERED_ERROR);
			phost->Control.errorcount = 0;
			USBH_ErrLog("Control error");
			status= USBH_FAIL;
		}

		break;
	}
	return status;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/




