/**
  ******************************************************************************
  * @file    usbh_hid.h
  * @author  MCD Application Team
  * @version V3.2.2
  * @date    07-July-2015
  * @brief   This file contains all the prototypes for the usbh_hid.c
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

/* Define to prevent recursive  ----------------------------------------------*/
#ifndef __USBH_HID_H
#define __USBH_HID_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbh_core.h"
#include "usbh_fifo.h"
#include "usbh_hid_parser.h"
/** @addtogroup USBH_CLASS
  * @{
  */

/** @addtogroup USBH_HID_CLASS
  * @{
  */
  
/** @defgroup USBH_HID_CORE
  * @brief This file is the Header file for usbh_hid.c
  * @{
  */ 


/** @defgroup USBH_HID_CORE_Exported_Types
  * @{
  */ 

#define HID_MIN_POLL                                10
#define HID_REPORT_SIZE                             16    
#define HID_MAX_USAGE                               10
#define HID_MAX_NBR_REPORT_FMT                      10 
#define HID_QUEUE_SIZE                              10    
    
#define  HID_ITEM_LONG                              0xFE
                                                                       
#define  HID_ITEM_TYPE_MAIN                         0x00
#define  HID_ITEM_TYPE_GLOBAL                       0x01
#define  HID_ITEM_TYPE_LOCAL                        0x02
#define  HID_ITEM_TYPE_RESERVED                     0x03

                                                                         
#define  HID_MAIN_ITEM_TAG_INPUT                    0x08
#define  HID_MAIN_ITEM_TAG_OUTPUT                   0x09
#define  HID_MAIN_ITEM_TAG_COLLECTION               0x0A
#define  HID_MAIN_ITEM_TAG_FEATURE                  0x0B
#define  HID_MAIN_ITEM_TAG_ENDCOLLECTION            0x0C

                                                                         
#define  HID_GLOBAL_ITEM_TAG_USAGE_PAGE             0x00
#define  HID_GLOBAL_ITEM_TAG_LOG_MIN                0x01
#define  HID_GLOBAL_ITEM_TAG_LOG_MAX                0x02
#define  HID_GLOBAL_ITEM_TAG_PHY_MIN                0x03
#define  HID_GLOBAL_ITEM_TAG_PHY_MAX                0x04
#define  HID_GLOBAL_ITEM_TAG_UNIT_EXPONENT          0x05
#define  HID_GLOBAL_ITEM_TAG_UNIT                   0x06
#define  HID_GLOBAL_ITEM_TAG_REPORT_SIZE            0x07
#define  HID_GLOBAL_ITEM_TAG_REPORT_ID              0x08
#define  HID_GLOBAL_ITEM_TAG_REPORT_COUNT           0x09
#define  HID_GLOBAL_ITEM_TAG_PUSH                   0x0A
#define  HID_GLOBAL_ITEM_TAG_POP                    0x0B

                                                                         
#define  HID_LOCAL_ITEM_TAG_USAGE                   0x00
#define  HID_LOCAL_ITEM_TAG_USAGE_MIN               0x01
#define  HID_LOCAL_ITEM_TAG_USAGE_MAX               0x02
#define  HID_LOCAL_ITEM_TAG_DESIGNATOR_INDEX        0x03
#define  HID_LOCAL_ITEM_TAG_DESIGNATOR_MIN          0x04
#define  HID_LOCAL_ITEM_TAG_DESIGNATOR_MAX          0x05
#define  HID_LOCAL_ITEM_TAG_STRING_INDEX            0x07
#define  HID_LOCAL_ITEM_TAG_STRING_MIN              0x08
#define  HID_LOCAL_ITEM_TAG_STRING_MAX              0x09
#define  HID_LOCAL_ITEM_TAG_DELIMITER               0x0A
    

/* States for HID State Machine */
typedef enum
{
  HID_INIT= 0,
  HID_IDLE,
  HID_SEND_DATA,
  HID_BUSY,
  HID_GET_DATA,
  HID_SYNC,
  HID_POLL,
  HID_ERROR,
}
HID_StateTypeDef;



typedef enum
{
  HID_REQ_INIT = 0,
  HID_REQ_IDLE,
  HID_REQ_GET_REPORT_DESC,
  HID_REQ_GET_HID_DESC,
  HID_REQ_SET_IDLE,
  HID_REQ_SET_PROTOCOL,
  HID_REQ_SET_REPORT,
} HID_CtlStateTypeDef;


typedef struct _HIDDescriptor
{
  uint8_t   bLength;
  uint8_t   bDescriptorType;
  uint16_t  bcdHID;               	/* indicates what endpoint this descriptor is describing */
  uint8_t   bCountryCode;        	/* specifies the transfer type. */
  uint8_t   bNumDescriptors;     	/* specifies the transfer type. */
  uint8_t   bReportDescriptorType;    /* Maximum Packet Size this endpoint is capable of sending or receiving */
  uint16_t  wItemLength;          	/* is used to specify the polling interval of certain transfers. */
}
HID_DescTypeDef;



struct _HID_Process;
typedef USBH_StatusTypeDef  ( * HID_Callback)(struct _HID_Process*);
typedef USBH_StatusTypeDef  ( * HID_CallbackDriver)(struct _HID_Process*,const uint8_t* hid_report);



typedef struct _HID_Process
{
  USBH_HandleTypeDef   	*phost;
  HID_ReportItemCollectionTypeDef* 	HIDReport;
  uint8_t			   Interface;
  uint8_t              OutPipe; 
  uint8_t              InPipe; 
  HID_StateTypeDef     state; 
  uint8_t              OutEp;
  uint8_t              InEp;
  HID_CtlStateTypeDef  ctl_state;
  uint32_t		       ctl_value; // used for state
  FIFO_TypeDef         fifo; 
  uint8_t              *pData;
  uint16_t             length;
  uint8_t              ep_addr;
  uint16_t             poll; 
  uint32_t             timer;
  uint8_t              DataReady;
  HID_DescTypeDef      HID_Desc;
  void*				   DriverUserData;
  //USBH_StatusTypeDef  ( * Init)(struct _HID_Process*);
  HID_Callback  Init;
  HID_Callback  DeInit;
  HID_Callback  InterruptReceiveCallback; // Must have this
  HID_Callback  IdleProcess; // Don't NEED this, be sure to do Set Idle

} HID_HandleTypeDef;
// install a driver
// once its installed it will be called once we get the
// report discriptor so you can parse and check that
// return OK if the driver is insalled or fail otherwise
USBH_StatusTypeDef USBH_HID_InstallDriver(HID_CallbackDriver init);
/**
  * @}
  */ 

/** @defgroup USBH_HID_CORE_Exported_Defines
  * @{
  */ 

#define USB_HID_GET_REPORT           0x01
#define USB_HID_GET_IDLE             0x02
#define USB_HID_GET_PROTOCOL         0x03
#define USB_HID_SET_REPORT           0x09
#define USB_HID_SET_IDLE             0x0A
#define USB_HID_SET_PROTOCOL         0x0B    




/* HID Class Codes */
#define USB_HID_CLASS                                   0x03

/* Interface Descriptor field values for HID Boot Protocol */
#define HID_BOOT_CODE                                  0x01    
#define HID_KEYBRD_BOOT_CODE                           0x01
#define HID_MOUSE_BOOT_CODE                            0x02


/**
  * @}
  */ 

/** @defgroup USBH_HID_CORE_Exported_Macros
  * @{
  */ 
/**
  * @}
  */ 

/** @defgroup USBH_HID_CORE_Exported_Variables
  * @{
  */ 
extern USBH_ClassTypeDef  HID_Class;
#define USBH_HID_CLASS    &HID_Class


// standard keyboard and mouse classes
typedef struct _HID_MOUSE_Info
{
  uint8_t              x;
  uint8_t              y;
  uint8_t              buttons[3];
} HID_MOUSE_Info_TypeDef;
USBH_StatusTypeDef USBH_HID_MouseInit(HID_HandleTypeDef *phost);
HID_MOUSE_Info_TypeDef *USBH_HID_GetMouseInfo(USBH_HandleTypeDef *phost);

typedef struct
{
  uint8_t state;
  uint8_t lctrl;
  uint8_t lshift;
  uint8_t lalt;
  uint8_t lgui;
  uint8_t rctrl;
  uint8_t rshift;
  uint8_t ralt;
  uint8_t rgui;
  uint8_t keys[6];
}
HID_KEYBD_Info_TypeDef;

USBH_StatusTypeDef USBH_HID_KeybdInit(HID_HandleTypeDef *phost);
HID_KEYBD_Info_TypeDef *USBH_HID_GetKeybdInfo(USBH_HandleTypeDef *phost);
uint8_t USBH_HID_GetASCIICode(HID_KEYBD_Info_TypeDef *info);




HID_HandleTypeDef* USBH_HID_CurrentHandle(USBH_HandleTypeDef *phost);
USBH_StatusTypeDef USBH_HID_SetReport (USBH_HandleTypeDef *phost,
                                  uint8_t reportType,
                                  uint8_t reportId,
                                  uint8_t* reportBuff,
                                  uint8_t reportLen);
  
USBH_StatusTypeDef USBH_HID_GetReport (USBH_HandleTypeDef *phost,
                                  uint8_t reportType,
                                  uint8_t reportId,
                                  uint8_t* reportBuff,
                                  uint8_t reportLen);  

USBH_StatusTypeDef USBH_HID_GetHIDReportDescriptor (USBH_HandleTypeDef *phost,  
		 uint16_t interface, uint16_t length);

USBH_StatusTypeDef USBH_HID_GetHIDDescriptor (USBH_HandleTypeDef *phost,  
                                            uint16_t interface, uint16_t length);

USBH_StatusTypeDef USBH_HID_SetIdle (USBH_HandleTypeDef *phost,
                                  uint8_t duration,
                                  uint8_t reportId);

USBH_StatusTypeDef USBH_HID_SetProtocol (USBH_HandleTypeDef *phost,
                                      uint8_t protocol);

void USBH_HID_EventCallback(USBH_HandleTypeDef *phost,HID_HandleTypeDef* handle);



uint8_t USBH_HID_GetPollInterval(USBH_HandleTypeDef *phost);


/**
  * @}
  */ 
#include "usbh_hid_mouse.h"
#include "usbh_hid_keybd.h"

// standard hid stuff combinded from mouse and keyboard
#ifdef __cplusplus
}
#endif

#endif /* __USBH_HID_H */

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

