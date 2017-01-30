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


#define ENUM_TO_STRING_DEFINE(TYPE) const char* TYPE##_ToString(TYPE t)
#define ENUM_TO_STRING(TYPE,VALUE) TYPE##_ToString(VALUE)

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

typedef enum {
	HOST_PORT_DISCONNECTED=0,
	HOST_PORT_CONNECTED,
	HOST_PORT_WAIT_FOR_ATTACHMENT,
	HOST_PORT_IDLE,		// no data going in or out
	HOST_PORT_ACTIVE,	// waiting for packet or finish trasmit
} PORT_StateTypeDef;
ENUM_TO_STRING_DEFINE(PORT_StateTypeDef);

typedef enum
{
	HOST_IDLE=0,
	HOST_FINISHED, // returned on a finished process
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
ENUM_TO_STRING_DEFINE(HOST_StateTypeDef);
const char* HOST_StateTypeToString(HOST_StateTypeDef t);

typedef enum
{
  CTRL_SETUP=0,
  CTRL_DATA_IN,
  CTRL_DATA_OUT,
  CTRL_STATUS_IN,
  CTRL_STATUS_OUT,
  CTRL_ERROR,
  CTRL_STALLED,
  CTRL_COMPLETE,
  CTRL_IDLE,
  CTRL_MAX_STATES
}CTRL_StateTypeDef;
ENUM_TO_STRING_DEFINE(CTRL_StateTypeDef);
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
	USBH_URB_NON_ALLOCATED,
	USBH_STATE_MAX
}USBH_URBStateTypeDef;
ENUM_TO_STRING_DEFINE(USBH_URBStateTypeDef);
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
ENUM_TO_STRING_DEFINE(USBH_SpeedTypeDef);

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

typedef void (*USBH_URBChangeCallback)(struct _USBH_HandleTypeDef *phost, uint8_t chnum, USBH_URBStateTypeDef urb_state);



typedef struct _DescStringCache{
	struct _DescStringCache* Next;
	uint8_t Index;
	uint8_t Length;
	char Data[1];
} USBH_DescStringCacheTypeDef;

struct __DeviceType;
struct _PipeHandle;
struct __DeviceType;

typedef struct _StateInfo {
	__IO  HOST_StateTypeDef 	PrevState;
	__IO  HOST_StateTypeDef 	State;
	int Value;
	void* Data;
	struct __DeviceType* Device;
	struct _StateInfo* Parent;
	struct _StateInfo* Children;
	struct _StateInfo* Next;
	// humm, we need to keep a state of whats going on and what packet we are on
} USBH_StateInfoTypeDef;
struct _ThreadInfo ;
typedef char (*PTThread)(struct _ThreadInfo* pt,struct _USBH_HandleTypeDef* phost);

// we use the gcc system for labels as values, not very compatable
// but I want this all in cpp anyway so fuck it
//#define USE_GCC_LABLELS

#define LC_CONCAT2(s1, s2) s1##s2
#define LC_CONCAT(s1, s2) LC_CONCAT2(s1, s2)
#ifndef USE_GCC_LABELS
typedef unsigned short lc_t;
#define LC_INIT(s) s = 0;
#define LC_RESUME(s) switch(s) { case 0:
#define LC_SET(s) s = __LINE__; case __LINE__:
#define LC_SET2MORE (__LINE__+1000)
#define LC_SET2(s) s = LC_SET2MORE; case LC_SET2MORE:
#define LC_END(s) }
#else
typedef void * lc_t;
#define LC_INIT(s) s = NULL;
#define LC_RESUME(s) do {	if(s != NULL) goto *s;	  } while(0);

#define LC_END(s)
#define LC_SET(s)				\
  do {						\
    LC_CONCAT(LC_LABEL, __LINE__):   	        \
    (s) = &&LC_CONCAT(LC_LABEL, __LINE__);	\
  } while(0)
#define LC_SET2(s)				\
  do {						\
    LC_CONCAT(LC_2LABEL, __LINE__):   	        \
    (s) = &&LC_CONCAT(LC_LABEL, __LINE__);	\
  } while(0)
#endif

typedef struct _ThreadInfo {
	lc_t lc;
	USBH_StatusTypeDef status;
	struct __DeviceType* Device;
	int Value;
	void* Data;
	PTThread func;
} USBH_ThreadTypeDef;

#define USB_WAITING 0
#define USB_YIELDED 1
#define USB_EXITED  2
#define USB_ENDED   3


#define USBH_SCHEDULE(f) ((f) < USB_EXITED)

#define USBH_THREAD_NAME(NAME) NAME##_USBThread
#define USBH_THREAD_BEGIN(NAME) \
	char USBH_THREAD_NAME(NAME)(USBH_HandleTypeDef*phost, struct _ThreadInfo* pt) { \
		const char* THREAD_NAME = #NAME; (void)THREAD_NAME;\
		struct __DeviceType* dev = pt->Device; (void)dev;\
		LC_RESUME(pt->lc);

#define USBH_USING_VALUE(NAME) int NAME=pt->Value;
#define USBH_USING_DATA(NAME, TYPE)  TYPE NAME = (TYPE)pt->Data;


#define USBH_RUN_PHOST(PHOST,START_THREAD) do { USBH_THREAD_NAME(START_THREAD)((PHOST), &(PHOST)->PTThreads[0]); } while(0);

#define USBH_THREAD_INIT(STATE) do {\
		LC_INIT((STATE)->lc);\
		(STATE)->status=USBH_OK; \
	} while(0);

#define USBH_THREAD_END() \
		LC_END((pt)->lc); \
		USBH_THREAD_INIT(pt); \
		return USB_ENDED; \
		CTR_ERROR:; \
		USBH_DbgLog("CTR_ERROR error in '%s' status=%i",THREAD_NAME,phost->Control.state); \
		while(1); \
		return USB_ENDED; \
  }
#define USBH_THREAD_STATUS (pt->status)

#define USBH_EXIT() do{ USBH_THREAD_INIT(pt); return USB_EXITED;  } while(0)
#define USBH_WAIT_UNTILL(condition) \
	 do {						\
		 LC_SET(pt->lc); \
		if(!(condition))return USB_WAITING;\
	  } while(0);
#define USBH_WAIT_WHILE(cond)  USBH_WAIT_UNTILL(!(cond))
#define USBH_WAIT_CTRL_REQUEST() \
		pt->status=USBH_CtlReq(phost, phost->Control.data,phost->Control.length);\
		USBH_WAIT_WHILE(pt->status == USBH_BUSY); \
		if(pt->status != USBH_OK) goto CTR_ERROR;



#define USBH_SPAWN(thread, DATA, VALUE)		\
  do {						\
	  USBH_DbgLog("SPAWNING THREAD: %s", #thread); \
	  phost->PTThreadPos++; \
	  USBH_THREAD_INIT(&phost->PTThreads[phost->PTThreadPos]); \
	  phost->PTThreads[phost->PTThreadPos].Data = DATA;       \
	  phost->PTThreads[phost->PTThreadPos].Value = VALUE;     \
	  LC_SET(pt->lc); \
	  if(USBH_THREAD_NAME(thread)(phost,&phost->PTThreads[phost->PTThreadPos])< USB_EXITED) \
	  	  return USB_WAITING; \
	  LC_SET2(pt->lc); \
  } while(0)



typedef enum {
	PIPE_SETUP=0,
	PIPE_CONTROL_IN, // 0x???1 is in
	PIPE_CONTROL_OUT,
	PIPE_ISO_IN,
	PIPE_ISO_OUT,
	PIPE_BULK_IN,
	PIPE_BULK_OUT,
	PIPE_INTERRUPT_IN,
	PIPE_INTERRUPT_OUT
} USBH_PipeDataTypeDef;
ENUM_TO_STRING_DEFINE(USBH_PipeDataTypeDef);
#define USBH_SETUP_PKT_SIZE                       8


typedef struct _PipeInit {
	USBH_SpeedTypeDef Speed;
	USBH_PipeDataTypeDef Type;
	uint8_t Address;
	uint8_t PacketSize;
	uint8_t EpNumber;
} USBH_PipeInitTypeDef;


typedef enum {
	PIPE_NOTALLOCATED=0,
	PIPE_IDLE,
	PIPE_WORKING,
} USBH_PipeStatusTypDef;

typedef struct _PipeHandle {
	USBH_PipeInitTypeDef Init;
	__IO USBH_PipeStatusTypDef state;
	__IO USBH_URBStateTypeDef urb_state;
	void (*Callback)(struct _USBH_HandleTypeDef *phost,struct _PipeHandle* pipe);
	uint8_t Pipe; 	// pipe index
	uint8_t* Data;  // data from or to pipe. must be big enough for the packet size
	uint16_t Size;	// data size to be trasfered
	void** owner; // device using pipe, null if not in use
} USBH_PipeHandleTypeDef;



/* Control request structure */
typedef struct
{
	USBH_RequestInitTypeDef 	Init;
	USBH_PipeHandleTypeDef* 	pipe_in;
	USBH_PipeHandleTypeDef* 	pipe_out;
	USB_Setup_TypeDef     		setup;
	uint8_t*					data;
	uint16_t					length;
	uint16_t              		timer;
	__IO CTRL_StateTypeDef     	state;
	CTRL_StateTypeDef     		prev_state;
	uint8_t               		errorcount;
	lc_t lc;
} USBH_CtrlTypeDef;


typedef struct _USBH_Class {
	const char* Name;
	uint8_t ClassCode;
	USBH_StatusTypeDef  (*Init)        (struct _USBH_HandleTypeDef *phost,struct __DeviceType* dev);
	USBH_StatusTypeDef  (*DeInit)      (struct _USBH_HandleTypeDef *phost,struct __DeviceType* dev);
	USBH_StatusTypeDef  (*Requests)    (struct _USBH_HandleTypeDef *phost,struct __DeviceType* dev);
	USBH_StatusTypeDef  (*BgndProcess) (struct _USBH_HandleTypeDef *phost,struct __DeviceType* dev);
	USBH_StatusTypeDef  (*SOFProcess)  (struct _USBH_HandleTypeDef *phost,struct __DeviceType* dev);
	void *pData;
	//USBH_DriverKindTypeDef Type;
	struct _USBH_Class* Next; // used in a chain list of drivers
} USBH_ClassTypeDef;

/* Attached device structure */
typedef struct __DeviceType
{
	uint8_t                    	  	  DescData[USBH_MAX_DATA_BUFFER]; // block of space used for device data
	StaticMemTypeDef				  Memory;
	uint16_t					      DescDataUsed;
	// the two below are stored in CfgDesc_Raw lineraly.
	USBH_DevDescTypeDef*              DevDesc;
	USBH_CfgDescTypeDef*              CfgDesc;
	USBH_DescStringCacheTypeDef*	  StringDesc;// cache of strings, linked list
	uint8_t							  configuration;
	uint8_t                           address;
	uint8_t                           speed;
	uint8_t                           current_interface;
	uint8_t							  interface_count;
	USBH_ClassTypeDef*				  ActiveClass[1]; // array of interfaces
	int16_t 						  StringLangSupport[3];
	void* pData;					  // user data
	struct __DeviceType* Next;
}USBH_DeviceTypeDef;





typedef enum {
	USBH_DRIVER_HID = 1,
	USBH_DRIVER_MATCH_VID= 2,
	USBH_DRIVER_MATCH_PID= 4,
	USBH_DRIVER_MATCH_CLASS = 8,
} USBH_DriverKindTypeDef;

struct _USBH_Driver;





/* USB Host Class structure */





// Memmory is becomming an issue but since we use alot of static memory
// mabye we can get away with a static memory allocator?

/* USB Host handle structure */
typedef struct _USBH_HandleTypeDef
{
	// This is the buffer used for all packet data if a buffer
	// is not supplyed, it can be used for anything
  uint8_t                   PacketData[USBH_MAX_DATA_BUFFER];
  USBH_ThreadTypeDef		PTThreads[MAX_STATE_STACK];
  uint16_t					PTThreadPos;
  USBH_StateInfoTypeDef		StateStack[MAX_STATE_STACK];
  __IO PORT_StateTypeDef    port_state; // raw port state, turn on the power etc
  USBH_SpeedTypeDef			port_speed;	// current port speed
  uint8_t					StackPos;
  USBH_HostStatusTypeDef	HostStatus;


  USBH_CtrlTypeDef      	Control;
  uint8_t					DeviceCount;
  USBH_DeviceTypeDef    	Devices[127];
  USBH_ClassTypeDef*    	pClass[USBH_MAX_NUM_SUPPORTED_CLASS];

  USBH_ClassTypeDef*    	pActiveClass;
  uint32_t              	ClassNumber;
  USBH_PipeHandleTypeDef    Pipes[15];
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

