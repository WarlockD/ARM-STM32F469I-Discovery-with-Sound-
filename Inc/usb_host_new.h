/*
 * usb_host_new.h
 *
 *  Created on: Feb 10, 2017
 *      Author: Paul
 */

#ifndef USB_HOST_NEW_H_
#define USB_HOST_NEW_H_

#include "usbh_desc.h"

typedef enum {
	USB_HOST_WORKING=0,
	USB_HOST_COMPLETE,
	USB_HOST_ERROR
} USB_HOST_Status;

struct __ControlRequest;
typedef void (*ControlCallback)(struct __ControlRequest*);
typedef struct __ControlRequest {
	uint8_t   RequestType;
	uint8_t   Request;
	uint16_t  Value;
	uint16_t  Index;
	uint16_t  Length;
	uint8_t* Data; // needs to be 32 bit aligned
	ControlCallback CompleteCallback;
	void* pData;
	__IO USB_HOST_Status status;
} __attribute__ ((aligned(4))) __attribute__((packed)) USBH_ControlRequestTypeDef;


USB_OTG_URBStateTypeDef USB_HOST_PipeState(uint8_t pipe);
void USB_SubmitSetup(uint8_t ch_num, uint8_t* data);
void USB_SubmitTrasfer(uint8_t ch_num, uint8_t* data, size_t length);


void USB_HOST_SetPipeMaxPacketSize(uint8_t pipe, uint16_t mps) ;
void USB_HOST_SetPipeDevAddress(uint8_t pipe, uint8_t dev_address);
void USB_HOST_SetPipeSpeed(uint8_t pipe,  uint8_t low_speed);
void USB_HOST_InitEndpoint(uint8_t pipe, uint8_t ep_type, uint8_t ep_num, uint8_t ep_dir);

void USB_HOST_SubmitControlRequest(uint8_t pipe, USBH_ControlRequestTypeDef* req);
// debbug
void USB_HOST_PrintPipe(uint8_t pipe);

#endif /* USB_HOST_NEW_H_ */
