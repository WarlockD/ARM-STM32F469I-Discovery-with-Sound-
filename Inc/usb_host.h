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

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include "stm32f4xx_hal.h"

#define ENUM_TO_STRING_DEFINE(TYPE) const char* TYPE##_ToString(TYPE t)
#define ENUM_TO_STRING(TYPE,VALUE) TYPE##_ToString(VALUE)
#ifndef PRINT_STRUCT_DEFINE
#define PRINT_STRUCT_DEFINE(TYPE) void TYPE##_Print(const TYPE* s)
#endif
#define PRINT_STRUCT(TYPE,PTR) TYPE##_Print(PTR)
// enums

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
#if 0
typedef enum {
  URB_IDLE = 0U,
  URB_DONE,
  URB_NOTREADY,
  URB_NYET,
  URB_ERROR,
  URB_STALL
}USB_OTG_URBStateTypeDef;
// mirrors this
#endif
typedef enum {
	USBH_URB_IDLE=0,
	USBH_URB_DONE,
	USBH_URB_NOTREADY,
	USBH_URB_NYET,
	USBH_URB_ERROR,
	USBH_URB_STALL,
	USBH_URB_NON_ALLOCATED,
	USBH_URB_TIMEOUT
} USBH_URBStateTypeDef;
ENUM_TO_STRING_DEFINE(USBH_URBStateTypeDef);
ENUM_TO_STRING_DEFINE(USB_OTG_HCStateTypeDef);

typedef struct __EndpointInit {
	uint8_t Number;
	uint8_t DevAddr;
	uint8_t MaxPacketSize;
	USBH_PortSpeedTypeDef Speed;
	USBH_EpRequestTypeDef Type;
} USBH_EndpointInitTypeDef;

typedef struct __Endpoint {
	USBH_EndpointInitTypeDef Init;
	uint8_t Channel;
	uint8_t Direction;
	uint8_t Type;
	uint8_t DoPing;
	__IO USBH_EndpointStatusTypeDef status;
	__IO USBH_URBStateTypeDef urb_state;
} USBH_EndpointTypeDef;
void USBH_Print_Endpoint(const USBH_EndpointTypeDef* value);

typedef struct _USB_RequestInit {
	uint8_t  RequestType;
	uint8_t  Request;
	uint16_t Value;
	uint16_t Index;
	uint16_t Length;
}USBH_RequestInitTypeDef;
void USBH_Print_RequestInit(const USBH_RequestInitTypeDef* value);

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

/* Control request structure */
typedef  struct ___CtrlTyp
{
  __IO uint8_t  		req_state;
  __IO uint8_t    		state;
  uint8_t               pipe_in;
  uint8_t               pipe_out;
  uint8_t               pipe_size;
  uint8_t               *buff;
  uint16_t              length;
  USB_Setup_TypeDef     setup;
  uint8_t               errorcount;
  void (*Callback)(struct ___CtrlTyp*, void*);
  void* pData;
} USBH_CtrlTypeDef;

typedef struct __RequestStatus {

} USBH_RequestStatusTypeDef;

typedef struct __CtrlTyp2 {
	uint8_t DevAddr;
	uint8_t MaxPacketSize;
	const USBH_EndpointTypeDef* ep_in;
	const USBH_EndpointTypeDef* ep_out;
	USB_Setup_TypeDef Setup;
	__IO USBH_EndpointStatusTypeDef status;
	__IO USBH_URBStateTypeDef urb_state;
} USBH_CtrlType2TypeDef;

typedef enum {
	USBH_LOG_USER=0,
	USBH_LOG_ERROR,
	USBH_LOG_WARN,
	USBH_LOG_DEBUG
} USBH_LogLevelTypeDef;
// utility functions
/* Memory management macros */

void* USBH_malloc(size_t size, void** owner);
void USBH_free(void * ptr);
void USBH_memset(void* ptr, uint8_t value, size_t size);
void USBH_memcpy(void* dst, const void* src, size_t size);
void USBH_Log(USBH_LogLevelTypeDef level, const char* msg, ...);
#define USBH_DEBUG_LEVEL 4
/* DEBUG macros */
#if (USBH_DEBUG_LEVEL > 0)
void USBH_UsrLog(const char* fmt, ...);
#define USBH_UsrLog(...)   USBH_Log(USBH_LOG_USER, __VA_ARGS__);
#else
#define USBH_UsrLog(...)
#endif
#if (USBH_DEBUG_LEVEL > 1)

#define USBH_ErrLog(...)  USBH_Log(USBH_LOG_ERROR, __VA_ARGS__);
#else
#define USBH_ErrLog(...)
#endif
#if (USBH_DEBUG_LEVEL > 2)

#define USBH_WarnLog(...)  USBH_Log(USBH_LOG_WARN, __VA_ARGS__);
#else
#define USBH_WarnLog(...)
#endif
#if (USBH_DEBUG_LEVEL > 3)
#define USBH_DbgLog(...)   USBH_Log(USBH_LOG_DEBUG, __VA_ARGS__);
#else
#define USBH_DbgLog(...)
#endif

#define USBH_EndpointStatus(EP) (ep->status)
// returns the channel being used or 0 on no free channel
const USBH_EndpointTypeDef* USBH_OpenEndpoint(const USBH_EndpointInitTypeDef* Init);
void USBH_ModifyEndpoint(const USBH_EndpointTypeDef* ep, const USBH_EndpointInitTypeDef* Init);
void USBH_EndpointSetMaxPacketSize(const USBH_EndpointTypeDef* ep, uint8_t size);

void USBH_ModifyEndpointCallback(const USBH_EndpointTypeDef* ep,   void (*Callback)(const USBH_EndpointTypeDef*, void*), void* userdata);
void CloseEndpoint(const USBH_EndpointTypeDef* ep);

const USBH_CtrlType2TypeDef* USBH_OpenControl(uint8_t DevAddr, uint8_t MaxPacketSize);
void USBH_ModifyControl(const USBH_CtrlType2TypeDef* ctl, uint8_t DevAddr, uint8_t MaxPacketSize);
void USBH_CloseControl(const USBH_CtrlType2TypeDef* ctl);

USBH_StatusTypeDef 	 USBH_Init(void* user_data);
USBH_StatusTypeDef 	 USBH_Start();
USBH_StatusTypeDef 	 USBH_Stop();
void  USBH_SubmitRequest(const USBH_EndpointTypeDef* ep, uint8_t* data, uint16_t length);
void  USBH_SubmitSetup(const USBH_EndpointTypeDef* ep, const USB_Setup_TypeDef* setup);
USBH_URBStateTypeDef  USBH_SubmitRequestBlocking(const USBH_EndpointTypeDef* ep, uint8_t* data, uint16_t length);
USBH_URBStateTypeDef  USBH_SubmitSetupBlocking(const USBH_EndpointTypeDef* ep, const USB_Setup_TypeDef* setup);
USBH_URBStateTypeDef USBH_Poll(); // run this a
void				 USBH_SetUserData(void* userdata);
void 				 USBH_PortConnectCallback(void* userdata);
void 				 USBH_PortDisconnectCallback(void* userdata);
USBH_URBStateTypeDef USBH_PipeStatus(uint8_t pipe);
// More complcated functions, blocking for now
void USBH_GetDescriptorBlocking(
		const USBH_EndpointTypeDef* ep_out,const USBH_EndpointTypeDef* ep_in,  const USBH_RequestInitTypeDef* init, void* data);
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

void USBH_CtrlRequest(USBH_CtrlTypeDef* req);

#ifdef __cplusplus
}
#endif

#endif /* USB_HOST_H_ */
