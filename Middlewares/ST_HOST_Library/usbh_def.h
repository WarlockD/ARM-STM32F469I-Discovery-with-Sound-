/**
  ******************************************************************************
  * @file    usbh_def.h
  * @author  MCD Application Team
  * @version V3.2.2
  * @date    07-July-2015
  * @brief   Definitions used in the USB host library
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
#ifndef  USBH_DEF_H
#define  USBH_DEF_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbh_conf.h"
#include "usbh_fifo.h"
/** @addtogroup USBH_LIB
  * @{
  */

/** @addtogroup USBH_LIB_CORE
* @{
*/
  
/** @defgroup USBH_DEF
  * @brief This file is includes USB descriptors
  * @{
  */ 

#ifndef NULL
#define NULL  0
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE  1
#endif


#define ValBit(VAR,POS)                               (VAR & (1 << POS))
#define SetBit(VAR,POS)                               (VAR |= (1 << POS))
#define ClrBit(VAR,POS)                               (VAR &= ((1 << POS)^255))

#define  LE16(addr)             (((uint16_t)(*((uint8_t *)(addr))))\
                                + (((uint16_t)(*(((uint8_t *)(addr)) + 1))) << 8))

#define  LE16S(addr)              (uint16_t)(LE16((addr)))

#define  LE32(addr)              ((((uint32_t)(*(((uint8_t *)(addr)) + 0))) + \
                                              (((uint32_t)(*(((uint8_t *)(addr)) + 1))) << 8) + \
                                              (((uint32_t)(*(((uint8_t *)(addr)) + 2))) << 16) + \
                                              (((uint32_t)(*(((uint8_t *)(addr)) + 3))) << 24)))

#define  LE64(addr)              ((((uint64_t)(*(((uint8_t *)(addr)) + 0))) + \
                                              (((uint64_t)(*(((uint8_t *)(addr)) + 1))) <<  8) +\
                                              (((uint64_t)(*(((uint8_t *)(addr)) + 2))) << 16) +\
                                              (((uint64_t)(*(((uint8_t *)(addr)) + 3))) << 24) +\
                                              (((uint64_t)(*(((uint8_t *)(addr)) + 4))) << 32) +\
                                              (((uint64_t)(*(((uint8_t *)(addr)) + 5))) << 40) +\
                                              (((uint64_t)(*(((uint8_t *)(addr)) + 6))) << 48) +\
                                              (((uint64_t)(*(((uint8_t *)(addr)) + 7))) << 56)))


#define  LE24(addr)              ((((uint32_t)(*(((uint8_t *)(addr)) + 0))) + \
                                              (((uint32_t)(*(((uint8_t *)(addr)) + 1))) << 8) + \
                                              (((uint32_t)(*(((uint8_t *)(addr)) + 2))) << 16)))


#define  LE32S(addr)              (int32_t)(LE32((addr)))



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
#define MAX_STATE_STACK 								10

/**
  * @}
  */ 


#define USBH_CONFIGURATION_DESCRIPTOR_SIZE (USB_CONFIGURATION_DESC_SIZE \
                                           + USB_INTERFACE_DESC_SIZE\
                                           + (USBH_MAX_NUM_ENDPOINTS * USB_ENDPOINT_DESC_SIZE))


#define CONFIG_DESC_wTOTAL_LENGTH (ConfigurationDescriptorData.ConfigDescfield.\
                                          ConfigurationDescriptor.wTotalLength)


typedef union
{
  uint16_t w;
  struct BW
  {
    uint8_t msb;
    uint8_t lsb;
  }
  bw;
}
uint16_t_uint8_t;


typedef union _USB_Setup
{
  uint32_t d8[2];
  
  struct _SetupPkt_Struc
  {
    uint8_t           bmRequestType;
    uint8_t           bRequest;
    uint16_t_uint8_t  wValue;
    uint16_t_uint8_t  wIndex;
    uint16_t_uint8_t  wLength;
  } b;
} 
USB_Setup_TypeDef;  

typedef struct _USB_RequestInit {
	uint8_t  RequestType;
	uint8_t  Request;
	uint16_t Value;
	uint16_t Index;
	uint16_t Length;
	uint8_t* Data;
}USBH_RequestInitTypeDef;

typedef struct _EndpointDescriptor
{
  uint8_t   EndpointAddress;   /* indicates what endpoint this descriptor is describing */
  uint8_t   Attributes;       /* specifies the transfer type. */
  uint16_t  MaxPacketSize;    /* Maximum Packet Size this endpoint is capable of sending or receiving */
  uint8_t   Interval;          /* is used to specify the polling interval of certain transfers. */
} USBH_EpDescTypeDef;

typedef struct _InterfaceDescriptor
{
  uint8_t 		InterfaceNumber;
  uint8_t 		AlternateSetting;    /* Value used to select alternative setting */
  uint8_t 		NumEndpoints;        /* Number of Endpoints used for this interface */
  uint8_t 		InterfaceClass;      /* Class Code (Assigned by USB Org) */
  uint8_t 		InterfaceSubClass;   /* Subclass Code (Assigned by USB Org) */
  uint8_t 		InterfaceProtocol;   /* Protocol Code */
  const char* 	Interface;           /* Index of String Descriptor Describing this interface */
  USBH_EpDescTypeDef* Endpoints;
} USBH_InterfaceDescTypeDef;


typedef struct _ConfigurationDescriptor
{
  uint8_t   	NumInterfaces;       /* Number of Interfaces */
  uint8_t   	ConfigurationValue;  /* Value to use as an argument to select this configuration*/
  const char*   Configuration;       /*Index of String Descriptor Describing this configuration */
  uint8_t   	Attributes;         /* D7 Bus Powered , D6 Self Powered, D5 Remote Wakeup , D4..0 Reserved (0)*/
  uint8_t   	MaxPower;            /*Maximum Power Consumption */
  USBH_InterfaceDescTypeDef* Interfaces;
} USBH_CfgDescTypeDef;

typedef struct _DeviceDescriptor
{
  uint16_t  	USB;        /* USB Specification Number which device complies too, bcd */
  uint8_t   	DeviceClass;
  uint8_t   	DeviceSubClass;
  uint8_t   	DeviceProtocol;
  /* If equal to Zero, each interface specifies its own class
  code if equal to 0xFF, the class code is vendor specified.
  Otherwise field is valid Class Code.*/
  uint8_t   	MaxPacketSize;
  uint16_t  	VendorID;      /* Vendor ID (Assigned by USB Org) */
  uint16_t  	ProductID;     /* Product ID (Assigned by Manufacturer) */
  uint16_t  	Device;     /* Device Release Number */
  const char*   Manufacturer;  /* Index of Manufacturer String Descriptor */
  const char*   Product;       /* Index of Product String Descriptor */
  const char*   SerialNumber;  /* Index of Serial Number String Descriptor */
  uint8_t   	NumConfigurations; /* Number of Possible Configurations */
  USBH_CfgDescTypeDef*	Configurations;
}  USBH_DevDescTypeDef;

/* Following states are used for gState */
typedef enum
{
	HOST_IDLE=0,
	HOST_FINISHED, // returned on a finished process
	HOST_CONNECTED,
	HOST_DEV_WAIT_FOR_ATTACHMENT,
	HOST_DEV_ATTACHED,
	HOST_DEV_DISCONNECTED,
	HOST_DETECT_DEVICE_SPEED,
	HOST_ENUMERATION, // --> goto ENUM_SET_ADDR
		ENUM_GET_DEV_DESC, // we HAVE to send a packet first
		ENUM_SET_ADDR,	  // change the addres here then
		ENUM_GET_STRING_LANGS,
		ENUM_GET_FULL_DEV_DESC,
	HOST_ENUMERATION_FINISHED,
	HOST_CLASS_REQUEST,
	HOST_INPUT,
	HOST_SET_CONFIGURATION,
	HOST_CHECK_CLASS,
	HOST_CLASS,
	HOST_SUSPENDED,
	HOST_ABORT_STATE,
	HOST_STACK_GET_STRING,
	HOST_STACK_GET_STRING_DESC,
	HOST_STACK_GET_CFG_DESC,
	HOST_STACK_GET_FULL_CFG_DESC,
	USBH_MAX_STATES
}HOST_StateTypeDef;
const char* HOST_StateTypeToString(HOST_StateTypeDef t);

typedef enum
{
  CMD_IDLE =0,
  CMD_SEND,
  CMD_WAIT
} CMD_StateTypeDef;

typedef enum
{
  CTRL_IDLE =0,
  CTRL_SETUP,
  CTRL_DATA_IN,
  CTRL_DATA_OUT,
  CTRL_STATUS_IN,
  CTRL_STATUS_OUT,
  CTRL_ERROR,
  CTRL_STALLED,
  CTRL_COMPLETE,
  CTRL_MAX_STATES
}CTRL_StateTypeDef;
const char* CTRLStateToString(CTRL_StateTypeDef t);


typedef enum
{
	USBH_HOST_Disconnected = 0,
	USBH_HOST_Connected,
	USBH_HOST_ConnectedMissingDriver,
} USBH_HostStatusTypeDef;


typedef enum {
	USBH_URB_IDLE=0,
	USBH_URB_DONE,
	USBH_URB_NOTREADY,
	USBH_URB_NYET,
	USBH_URB_ERROR,
	USBH_URB_STALL,
	USBH_STATE_MAX
}USBH_URBStateTypeDef;

const char* URBStateToString(USBH_URBStateTypeDef t);

struct _USBH_HandleTypeDef;
typedef int USBH_MStateTypeDef;


/* Following USB Host status */
typedef enum 
{
  USBH_OK   = 0,
  USBH_BUSY,
  USBH_FAIL,
  USBH_NOT_SUPPORTED,
  USBH_UNRECOVERED_ERROR,
  USBH_ERROR_SPEED_UNKNOWN,
}USBH_StatusTypeDef;


/** @defgroup USBH_CORE_Exported_Types
  * @{
  */

typedef enum 
{
  USBH_SPEED_HIGH  = 0,
  USBH_SPEED_FULL  = 1,
  USBH_SPEED_LOW   = 2,  
    
}USBH_SpeedTypeDef;



typedef enum
{
  USBH_PORT_EVENT = 1,  
  USBH_URB_EVENT,
  USBH_CONTROL_EVENT,    
  USBH_CLASS_EVENT,     
  USBH_STATE_CHANGED_EVENT,   
}
USBH_OSEventTypeDef;

struct _USBH_HandleTypeDef;
typedef	USBH_StatusTypeDef (* USBH_CallbackTypeDef )(struct _USBH_HandleTypeDef *pHandle);

typedef void (*USBH_URBChangeCallback)(struct _USBH_HandleTypeDef *pHandle, uint8_t chnum, USBH_URBStateTypeDef urb_state);

typedef struct {
	uint8_t address; // usb device address
	uint8_t pipe;
} USBH_EndPointTypeDef;
/* Control request structure */
typedef struct 
{
	USBH_RequestInitTypeDef Init;
  uint8_t               pipe_in; 
  uint8_t               pipe_out; 
  uint8_t               pipe_size;  
  uint8_t               *buff;
  uint16_t              length;
  uint16_t              timer;  
  USB_Setup_TypeDef     setup;
  __IO CTRL_StateTypeDef     state;

  CTRL_StateTypeDef     prev_state;
  uint8_t               errorcount;  
} USBH_CtrlTypeDef;

typedef struct _DescStringCache{
	struct _DescStringCache* Next;
	uint8_t Index;
	uint8_t Length;
	char Data[1];
} USBH_DescStringCacheTypeDef;


/* Attached device structure */
typedef struct
{
	uint8_t                           Data[USBH_MAX_DATA_BUFFER];
	uint8_t                           DescData[USBH_MAX_DATA_BUFFER];
	uint16_t					      DescDataUsed;
	// the two below are stored in CfgDesc_Raw lineraly.
	USBH_DevDescTypeDef*              DevDesc;
	USBH_CfgDescTypeDef*              CfgDesc;
	USBH_DescStringCacheTypeDef*	  StringDesc;// cache of strings, linked list
	uint8_t							  configuration;
	uint8_t                           address;
	uint8_t                           speed;
	__IO uint8_t                      is_connected;
	uint8_t                           current_interface;
}USBH_DeviceTypeDef;



typedef enum {
	USBH_DRIVER_HID = 1,
	USBH_DRIVER_MATCH_VID= 2,
	USBH_DRIVER_MATCH_PID= 4,
	USBH_DRIVER_MATCH_CLASS = 8,
} USBH_DriverKindTypeDef;

struct _USBH_Driver;

typedef struct _USBH_DriverHandle
{
	struct _USBH_HandleTypeDef* phost;
  uint8_t Interface;	// Current Interface being used
  uint8_t Address; 		// USB address for USB device
  void* pData;			// User Data
  // endpoint information? humm
} USBH_DriverHandleTypeDef;

typedef struct _USBH_Class {
	const char* Name;
	uint8_t ClassCode;
	USBH_StatusTypeDef  (*Init)        (struct _USBH_HandleTypeDef *phost);
	USBH_StatusTypeDef  (*DeInit)      (struct _USBH_HandleTypeDef *phost);
	USBH_StatusTypeDef  (*Requests)    (struct _USBH_HandleTypeDef *phost);
	USBH_StatusTypeDef  (*BgndProcess) (struct _USBH_HandleTypeDef *phost);
	USBH_StatusTypeDef  (*SOFProcess)  (struct _USBH_HandleTypeDef *phost);
	void *pData;
	USBH_DriverKindTypeDef Type;
	struct _USBH_Class* Next; // used in a chain list of drivers
} USBH_ClassTypeDef;

/* USB Host Class structure */



typedef struct {
	__IO  HOST_StateTypeDef 	PrevState;
	__IO  HOST_StateTypeDef 	State;
	int Value;
	void* Data;
} USBH_StateInfoTypeDef;

// Memmory is becomming an issue but since we use alot of static memory
// mabye we can get away with a static memory allocator?

/* USB Host handle structure */
typedef struct _USBH_HandleTypeDef
{
  HOST_StateTypeDef			StateStackgSave;
  USBH_StateInfoTypeDef		StateStack[MAX_STATE_STACK];
  uint8_t					StackPos;
  USBH_HostStatusTypeDef	HostStatus;
  StaticMemTypeDef			Memory;

  int16_t 					StringLangSupport[3];

  USBH_URBChangeCallback	URBChangeCallback;
  __IO CMD_StateTypeDef     RequestState;
  USBH_CallbackTypeDef		RequestCompletedCallback;
  USBH_CallbackTypeDef		RequestErrorCallback;
  USBH_CtrlTypeDef      	Control;
  USBH_DeviceTypeDef    	device;
  USBH_ClassTypeDef*    	pClass[USBH_MAX_NUM_SUPPORTED_CLASS];

  USBH_ClassTypeDef*    	pActiveClass;
  uint32_t              	ClassNumber;
  uint32_t              	Pipes[15];
  __IO uint32_t         	Timer;
  uint8_t               	id;
  void*                 	pData;
  void                		(* pUser )(struct _USBH_HandleTypeDef *pHandle, uint8_t id);
#if (USBH_USE_OS == 1)
  osMessageQId          os_event;   
  osThreadId            thread; 
#endif  
  
} USBH_HandleTypeDef;


#if  defined ( __GNUC__ )
  #ifndef __weak
    #define __weak   __attribute__((weak))
  #endif /* __weak */
  #ifndef __packed
    #define __packed __attribute__((__packed__))
  #endif /* __packed */
#endif /* __GNUC__ */

#ifdef __cplusplus
}
#endif

#endif /* USBH_DEF_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

