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
#include <assert.h>
/** @addtogroup USBH_LIB
* @{
*/

/** @addtogroup USBH_LIB_CORE
* @{
*/

/** @defgroup USBH_CTLREQ 
* @brief This file implements the standard requests for device enumeration
* @{
*/


/** @defgroup USBH_CTLREQ_Private_Defines
* @{
*/ 
/**
* @}
*/ 


/** @defgroup USBH_CTLREQ_Private_TypesDefinitions
* @{
*/ 
/**
* @}
*/ 



/** @defgroup USBH_CTLREQ_Private_Macros
* @{
*/ 
/**
* @}
*/ 


/** @defgroup USBH_CTLREQ_Private_Variables
* @{
*/
/**
* @}
*/ 

/** @defgroup USBH_CTLREQ_Private_FunctionPrototypes
* @{
*/
static USBH_StatusTypeDef USBH_HandleControl (USBH_HandleTypeDef *phost);

static void USBH_ParseDevDesc (USBH_DevDescTypeDef* , uint8_t *buf, uint16_t length);

static void USBH_ParseCfgDesc (USBH_CfgDescTypeDef* cfg_desc,
                               uint8_t *buf, 
                               uint16_t length);


static void USBH_ParseEPDesc (USBH_EpDescTypeDef  *ep_descriptor, uint8_t *buf);
static void USBH_ParseStringDesc (uint8_t* psrc, uint8_t* pdest, uint16_t length);
static void USBH_ParseInterfaceDesc (USBH_InterfaceDescTypeDef  *if_descriptor, uint8_t *buf);


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
USBH_StatusTypeDef USBH_Get_DevDesc(USBH_HandleTypeDef *phost, uint8_t length)
{
  USBH_StatusTypeDef status;
  uint8_t *pData = phost->device.DescData;
  if((status = USBH_GetDescriptor(phost,
                                  USB_REQ_RECIPIENT_DEVICE | USB_REQ_TYPE_STANDARD,                          
                                  USB_DESC_DEVICE, 
								  pData,
                                  length)) == USBH_OK)
  {

    /* Commands successfully sent and Response Received */
	  printf("USBH_Get_DevDesc success!\r\n");
	  phost->device.DescDataUsed = phost->device.DevDesc->Header.bLength;
  }
  return status;      
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
USBH_StatusTypeDef USBH_Get_CfgDesc(USBH_HandleTypeDef *phost,                      
                             uint16_t length)

{
  USBH_StatusTypeDef status;
  uint8_t *pData = phost->device.DescData + phost->device.DevDesc->Header.bLength;
  if((status = USBH_GetDescriptor(phost,
                                  USB_REQ_RECIPIENT_DEVICE | USB_REQ_TYPE_STANDARD,                          
                                  USB_DESC_CONFIGURATION, 
                                  pData,
                                  length)) == USBH_OK)
  {
	  if(length > USB_CONFIGURATION_DESC_SIZE){
		  // this is the final one
		  phost->device.DescDataUsed += phost->device.CfgDesc->wTotalLength;
		 	  // we set up the string table now
		 phost->device.StringDesc = (USBH_DescStringCacheTypeDef*)(phost->device.DescData + phost->device.DescDataUsed);
		 phost->device.StringDesc->bLength = 0; // make sure its an empty entry
		 	  /* Commands successfully sent and Response Received  */
		     //USBH_ParseCfgDesc (&phost->device.CfgDesc, pData, length);
	  }

  }
  return status;
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
USBH_StatusTypeDef USBH_Get_StringDesc(USBH_HandleTypeDef *phost,
                                uint8_t string_index, USBH_DescStringCacheTypeDef** str)
{
	// first search though the cache, shame we have to do this each time we are checking the status
	// but whatever
	USBH_DescStringCacheTypeDef* ptr = phost->device.StringDesc;
	assert(ptr); // ptr should be valid about now
	while(ptr && ptr->bLength != 0) {
		if(ptr->bIndex == string_index) {
			*str = ptr;
			printf("Found String Index:%i = '%s'\r\n",ptr->bIndex, ptr->strData);
			return USBH_OK; // found it
		}
		printf("Incorrect Index:%i = '%s'\r\n",ptr->bIndex, ptr->strData);
		ptr= (USBH_DescStringCacheTypeDef*)(((uint8_t*)ptr) + ptr->bLength);
	}
  USBH_StatusTypeDef status;
  if((status = USBH_GetDescriptor(phost,
                                  USB_REQ_RECIPIENT_DEVICE | USB_REQ_TYPE_STANDARD,                                    
                                  USB_DESC_STRING | string_index, 
                                  phost->device.Data,
								  0xff)) == USBH_OK)
  {
    /* Commands successfully sent and Response Received  */
	  assert(phost->device.Data[1] == USB_DESC_TYPE_STRING);
	  ptr->bIndex = string_index;
	  ptr->bLength = 3; // size of structure plus 0
	  uint8_t ulen = phost->device.Data[0]-2;
	  char* c_str = ptr->strData;
	  for (uint8_t idx = 2; idx < phost->device.Data[0]; idx+=2 )
	  {/* Copy Only the string and ignore the UNICODE ID, hence add the src */
	 	  *c_str++ =  phost->device.Data[idx];
	 	 ptr->bLength++;
	 }

	  *c_str++ = 0;
	  *c_str = 0; // 0 length of the next entry
	  printf("String Index:%i = '%s'\r\n",ptr->bIndex, ptr->strData);
	  phost->device.DescDataUsed += ptr->bLength;
	  *str = ptr;
  }

  return status;
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
                               uint16_t value_idx, 
                               uint8_t* buff, 
                               uint16_t length )
{ 

	USBH_CtrlTypeDef* ctl = (USBH_CtrlTypeDef*)&phost->Control;
	ctl->setup.b.bmRequestType = USB_D2H | req_type;
	ctl->setup.b.bRequest = USB_REQ_GET_DESCRIPTOR;
	ctl->setup.b.wValue.w = value_idx;
	if ((value_idx & 0xff00) == USB_DESC_STRING)
		ctl->setup.b.wIndex.w = 0x0409;
	 else
		ctl->setup.b.wIndex.w = 0;
	ctl->setup.b.wLength.w = length;
	ctl->buff = buff;
	ctl->length = length;
	USBH_PushState(phost, CTRL_SETUP,ctl,0);
	return USBH_OK;
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
	USBH_CtrlTypeDef* ctl = (USBH_CtrlTypeDef*)&phost->Control;
	ctl->setup.b.bmRequestType  = USB_H2D | USB_REQ_RECIPIENT_DEVICE | USB_REQ_TYPE_STANDARD;
	ctl->setup.b.bRequest = USB_REQ_SET_ADDRESS;
	ctl->setup.b.wValue.w = (uint16_t)DeviceAddress;
	ctl->setup.b.wIndex.w = 0;
	ctl->setup.b.wLength.w = 0;
	ctl->buff = 0;
	ctl->length = 0;
	USBH_PushState(phost, CTRL_SETUP,ctl,0);
	return USBH_OK;
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
	USBH_CtrlTypeDef* ctl = (USBH_CtrlTypeDef*)&phost->Control;
	ctl->setup.b.bmRequestType  = USB_H2D | USB_REQ_RECIPIENT_DEVICE |USB_REQ_TYPE_STANDARD;
	ctl->setup.b.bRequest = USB_REQ_SET_CONFIGURATION;
	ctl->setup.b.wValue.w = cfg_idx;
	ctl->setup.b.wIndex.w = 0;
	ctl->setup.b.wLength.w = 0;
	ctl->buff = 0;
	ctl->length = 0;
	USBH_PushState(phost, CTRL_SETUP,ctl,0);
	return USBH_OK;
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
	USBH_CtrlTypeDef* ctl = (USBH_CtrlTypeDef*)&phost->Control;
	ctl->setup.b.bmRequestType  = USB_H2D | USB_REQ_RECIPIENT_INTERFACE |USB_REQ_TYPE_STANDARD;
	ctl->setup.b.bRequest = USB_REQ_SET_INTERFACE;
	ctl->setup.b.wValue.w = altSetting;
	ctl->setup.b.wIndex.w = 0;
	ctl->setup.b.wLength.w = 0;
	ctl->buff = 0;
	ctl->length = 0;
	USBH_PushState(phost, CTRL_SETUP,ctl,0);
	return USBH_OK;
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
	USBH_CtrlTypeDef* ctl = (USBH_CtrlTypeDef*)&phost->Control;
	ctl->setup.b.bmRequestType  = USB_H2D | USB_REQ_RECIPIENT_ENDPOINT |USB_REQ_TYPE_STANDARD;
	ctl->setup.b.bRequest = USB_REQ_CLEAR_FEATURE;
	ctl->setup.b.wValue.w = FEATURE_SELECTOR_ENDPOINT;
	ctl->setup.b.wIndex.w = ep_num;
	ctl->setup.b.wLength.w = 0;
	ctl->buff = 0;
	ctl->length = 0;
	USBH_PushState(phost, CTRL_SETUP,ctl,0);
	return USBH_OK;
}




/**
  * @brief  USBH_GetNextDesc 
  *         This function return the next descriptor header
  * @param  buf: Buffer where the cfg descriptor is available
  * @param  ptr: data pointer inside the cfg descriptor
  * @retval next header
  */
USBH_DescHeaderTypeDef  *USBH_GetNextDesc (uint8_t   *pbuf, uint16_t  *ptr)
{
	USBH_DescHeaderTypeDef  *pnext;
 
  *ptr += ((USBH_DescHeaderTypeDef *)pbuf)->bLength;
  pnext = (USBH_DescHeaderTypeDef *)((uint8_t *)pbuf + \
         ((USBH_DescHeaderTypeDef *)pbuf)->bLength);
 
  return(pnext);
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/




