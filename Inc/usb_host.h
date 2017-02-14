/*
 * usb_host.h
 *
 *  Created on: Jan 31, 2017
 *      Author: Paul
 */

#ifndef USB_HOST_H_
#define USB_HOST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define ENUM_TO_STRING_DEFINE(TYPE) const char* TYPE##_ToString(TYPE t)
#define ENUM_TO_STRING(TYPE,VALUE) TYPE##_ToString(VALUE)
#ifndef PRINT_STRUCT_DEFINE
#define PRINT_STRUCT_DEFINE(TYPE) void TYPE##_Print(const TYPE* s)
#endif
#define PRINT_STRUCT(TYPE,PTR) TYPE##_Print(PTR)
// enums


ENUM_TO_STRING_DEFINE(HCD_URBStateTypeDef);
typedef enum {
	USBH_SPEED_LOW=HCD_SPEED_LOW,
	USBH_SPEED_FULL=HCD_SPEED_FULL,
	USBH_SPEED_HIGH=HCD_SPEED_HIGH,
	USBH_SPEED_DEFAULT_PORT=100, // used to configure using port speed
} USBH_PortSpeedTypeDef;
ENUM_TO_STRING_DEFINE(USBH_PortSpeedTypeDef);

typedef enum {
	USBH_EP_CONTROL_OUT=0,
	USBH_EP_CONTROL_IN,
	USBH_EP_ISO_OUT,
	USBH_EP_ISO_IN,
	USBH_EP_BULK_OUT,
	USBH_EP_BULK_IN,
	USBH_EP_INTERRUPT_OUT,
	USBH_EP_INTERRUPT_IN,
} USBH_EpRequestTypeDef;
ENUM_TO_STRING_DEFINE(USBH_EpRequestTypeDef);

typedef enum {
	USBH_PIPE_NOT_ALLOCATED=0,
	USBH_PIPE_IDLE,
	USBH_PIPE_WORKING,
	USBH_PIPE_WAITING_FOR_IDLE,
} USBH_EndpointStatusTypeDef;
ENUM_TO_STRING_DEFINE(USBH_EndpointStatusTypeDef);

ENUM_TO_STRING_DEFINE(USB_OTG_URBStateTypeDef);

typedef enum
{
  USBH_OK   = 0,
  USBH_BUSY,
  USBH_FAIL,
  USBH_NOT_SUPPORTED,
  USBH_UNRECOVERED_ERROR,
  USBH_ERROR_SPEED_UNKNOWN,
}USBH_StatusTypeDef;
ENUM_TO_STRING_DEFINE(USBH_StatusTypeDef);

typedef enum {
	USBH_URB_IDLE=URB_IDLE,
	USBH_URB_DONE=URB_DONE,
	USBH_URB_NOTREADY=URB_NOTREADY,
	USBH_URB_NYET=URB_NYET,
	USBH_URB_ERROR=URB_ERROR,
	USBH_URB_STALL=URB_STALL,
	USBH_URB_NON_ALLOCATED,
	USBH_URB_TIMEOUT
} USBH_URBStateTypeDef; // mirror of USB_OTG_URBStateTypeDef
ENUM_TO_STRING_DEFINE(USBH_URBStateTypeDef);
ENUM_TO_STRING_DEFINE(USB_OTG_HCStateTypeDef);

struct __DeviceType;
struct __PipeHandle;
struct __ControlRequest;
typedef void (*PipeCallback)(struct __DeviceType *, struct __PipeHandle*, void*);




typedef struct __PipeHandle {
	__IO USBH_EndpointStatusTypeDef status;
	__IO USBH_URBStateTypeDef urb_state;
	__IO USBH_URBStateTypeDef hc_state;
} USBH_PipeHandleTypeDef;

typedef struct __DeviceType {
	uint8_t Address;
	uint8_t MaxPacketSize;
	USBH_PortSpeedTypeDef Speed;
	void* pData;
} USBH_DeviceTypeDef;

typedef enum {
	USBH_SETUP_H2D=0x00, // host to device
	USBH_SETUP_D2H=0x80, // device to host
} USBH_SetupDirectionTypeDef;

typedef enum {
	USBH_SETUP_STANDARD=0x00, // host to device
	USBH_SETUP_CLASS=0x20, // device to host
	USBH_SETUP_VENDOR=0x40, // device to host
} USBH_SetupTypeTypeDef;

typedef enum {
	USBH_SETUP_DEVICE=0x00, // host to device
	USBH_SETUP_INTERFACE=0x01, // device to host
	USBH_SETUP_ENDPOINT=0x02, // device to host
	USBH_SETUP_OTHER=0x03, // device to host
} USBH_SetupRecipentTypeDef;







// utility functions
/* Memory management macros */

void* USBH_malloc(size_t size, void** owner);
void USBH_free(void * ptr);
void USBH_memset(void* ptr, uint8_t value, size_t size);
void USBH_memcpy(void* dst, const void* src, size_t size);

#define USBH_DEBUG_LEVEL 4
/* DEBUG macros */
#if (USBH_DEBUG_LEVEL > 0)
#define USBH_UsrLog(...)   PrintDebugMessage(LOG_USER, __VA_ARGS__);
#else
#define USBH_UsrLog(...)
#endif
#if (USBH_DEBUG_LEVEL > 1)

#define USBH_ErrLog(...)   PrintDebugMessage(LOG_ERROR, __VA_ARGS__);
#else
#define USBH_ErrLog(...)
#endif
#if (USBH_DEBUG_LEVEL > 2)

#define USBH_WarnLog(...)   PrintDebugMessage(LOG_WARN, __VA_ARGS__);
#else
#define USBH_WarnLog(...)
#endif
#if (USBH_DEBUG_LEVEL > 3)
#define USBH_DbgLog(...)    PrintDebugMessage(LOG_DEBUG, __VA_ARGS__);
#else
#define USBH_DbgLog(...)
#endif

#define USBH_EndpointStatus(EP) (ep->status)
// returns the channel being used or 0 on no free channel


USBH_StatusTypeDef 	 USBH_Init(void* user_data);
USBH_StatusTypeDef 	 USBH_Start();
USBH_StatusTypeDef 	 USBH_Stop();
USBH_URBStateTypeDef USBH_Poll(); // run this a
void				 USBH_SetUserData(void* userdata);
void 				 USBH_PortConnectCallback(void* userdata);
void 				 USBH_PortDisconnectCallback(void* userdata);
USBH_URBStateTypeDef USBH_PipeStatus(uint8_t pipe);
// More complcated functions, blocking for now
void USBH_Print_Buffer(uint8_t* data, size_t length);
// testing stuff


void USBH_IsocSendData(uint8_t *buff, uint32_t length, uint8_t pipe_num);
void USBH_IsocReceiveData(uint8_t *buff, uint32_t length, uint8_t pipe_num);
void USBH_InterruptSendData(uint8_t *buff,  uint8_t length, uint8_t pipe_num);
void USBH_InterruptReceiveData(uint8_t *buff,  uint8_t length, uint8_t pipe_num);
void USBH_BulkReceiveData(uint8_t *buff, uint16_t length, uint8_t pipe_num);
void USBH_BulkSendData (uint8_t *buff, uint16_t length, uint8_t pipe_num,  uint8_t do_ping);
void USBH_CtlReceiveData(uint8_t* buff,  uint16_t length, uint8_t pipe_num);
void USBH_CtlSendData (uint8_t *buff, uint16_t length, uint8_t pipe_num, uint8_t do_ping);
void USBH_CtlSendSetup (uint8_t *buff, uint8_t pipe_num);


#ifdef __cplusplus
}
#endif

#endif /* USB_HOST_H_ */
