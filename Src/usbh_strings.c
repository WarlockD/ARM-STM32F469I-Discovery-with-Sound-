/*
 * usbh_strings.c
 *
 *  Created on: Jan 30, 2017
 *      Author: Paul
 */
#include "usb_host.h"
#include "usbh_desc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

const char* ENUM_OUT_OF_RANGE(const char* e, uint8_t value) {
	static char msg[64];
	sprintf(msg, "%s(%i)", e,value);
	return msg;
}

#define ENUM_TO_STRING_START(TYPE) \
		ENUM_TO_STRING_DEFINE(TYPE) { \
		static const char* tbl[] = {

#define ENUM_TO_STRING_ADD(ENUM) #ENUM,

#define ENUM_TO_STRING_END(TYPE) \
	}; \
	uint8_t tbl_size = (sizeof(tbl)/sizeof(*tbl)); \
	if(((uint8_t)t) > tbl_size) return ENUM_OUT_OF_RANGE(#TYPE,(uint8_t)t);\
	else return tbl[(uint8_t)t]; \
	}
ENUM_TO_STRING_START(USBH_EpRequestTypeDef)
	ENUM_TO_STRING_ADD(USBH_EP_CONTROL_OUT)
	ENUM_TO_STRING_ADD(USBH_EP_CONTROL_IN)
	ENUM_TO_STRING_ADD(USBH_EP_ISO_OUT)
	ENUM_TO_STRING_ADD(USBH_EP_ISO_IN)
	ENUM_TO_STRING_ADD(USBH_EP_BULK_OUT)
	ENUM_TO_STRING_ADD(USBH_EP_BULK_IN)
	ENUM_TO_STRING_ADD(USBH_EP_INTERRUPT_OUT)
	ENUM_TO_STRING_ADD(USBH_EP_INTERRUPT_IN)
ENUM_TO_STRING_END(USBH_EpRequestTypeDef)

ENUM_TO_STRING_START(USBH_PortSpeedTypeDef)
	ENUM_TO_STRING_ADD(USBH_SPEED_LOW)
	ENUM_TO_STRING_ADD(USBH_SPEED_FULL)
	ENUM_TO_STRING_ADD(USBH_SPEED_HIGH)
ENUM_TO_STRING_END(USBH_PortSpeedTypeDef)


ENUM_TO_STRING_START(USBH_EndpointStatusTypeDef)
	ENUM_TO_STRING_ADD(USBH_PIPE_NOT_ALLOCATED)
	ENUM_TO_STRING_ADD(USBH_PIPE_IDLE)
	ENUM_TO_STRING_ADD(USBH_PIPE_WORKING)
ENUM_TO_STRING_END(USBH_EndpointStatusTypeDef)

ENUM_TO_STRING_START(USBH_URBStateTypeDef)
	ENUM_TO_STRING_ADD(USBH_URB_IDLE)
	ENUM_TO_STRING_ADD(USBH_URB_DONE)
	ENUM_TO_STRING_ADD(USBH_URB_NOTREADY)
	ENUM_TO_STRING_ADD(USBH_URB_NYET)
	ENUM_TO_STRING_ADD(USBH_URB_ERROR)
	ENUM_TO_STRING_ADD(USBH_URB_STALL)
	ENUM_TO_STRING_ADD(USBH_URB_NON_ALLOCATED)
ENUM_TO_STRING_END(USBH_URBStateTypeDef)


ENUM_TO_STRING_START(USBH_StatusTypeDef)
	ENUM_TO_STRING_ADD(USBH_OK)
	ENUM_TO_STRING_ADD(USBH_BUSY)
	ENUM_TO_STRING_ADD(USBH_FAIL)
	ENUM_TO_STRING_ADD(USBH_NOT_SUPPORTED)
	ENUM_TO_STRING_ADD(USBH_UNRECOVERED_ERROR)
	ENUM_TO_STRING_ADD(USBH_ERROR_SPEED_UNKNOWN)
ENUM_TO_STRING_END(USBH_StatusTypeDef)

ENUM_TO_STRING_START(USB_OTG_URBStateTypeDef)
	ENUM_TO_STRING_ADD(URB_IDLE)
	ENUM_TO_STRING_ADD(URB_DONE)
	ENUM_TO_STRING_ADD(USBH_FAIL)
	ENUM_TO_STRING_ADD(URB_NOTREADY)
	ENUM_TO_STRING_ADD(URB_NYET)
	ENUM_TO_STRING_ADD(URB_ERROR)
	ENUM_TO_STRING_ADD(URB_STALL)
ENUM_TO_STRING_END(USB_OTG_URBStateTypeDef)

ENUM_TO_STRING_START(USB_OTG_HCStateTypeDef)
	ENUM_TO_STRING_ADD(HC_IDLE)
	ENUM_TO_STRING_ADD(HC_XFRC)
	ENUM_TO_STRING_ADD(HC_HALTED)
	ENUM_TO_STRING_ADD(HC_NAK)
	ENUM_TO_STRING_ADD(HC_NYET)
	ENUM_TO_STRING_ADD(HC_STALL)
	ENUM_TO_STRING_ADD(HC_XACTERR)
	ENUM_TO_STRING_ADD(HC_BBLERR)
	ENUM_TO_STRING_ADD(HC_DATATGLERR)
ENUM_TO_STRING_END(USB_OTG_HCStateTypeDef)


#define PRINT_BYTE_VALUE(DEV,NAME) printf("%*s%-18s = %i\r\n",ident,"", #NAME, (int)DEV->NAME);
#define PRINT_BCD_VALUE(DEV,NAME) printf("%*s%-18s = %2.2X%2.2X\r\n",ident,"", #NAME, DEV->NAME<<8, DEV->NAME & 0xFF);
#define PRINT_HEX_VALUE(DEV,NAME) printf("%*s%-18s = 0x%4.4X\r\n",ident,"", #NAME, (int)DEV->NAME);
typedef union  {
	const char* str;
	int idx;
} str_index;
void PrintStringValue(int ident, const char* name, const char* s){
	if(s) {
		str_index bob;
		bob.str = s;
		uart_print("IDX=%i\r\n",bob.idx);
		if(bob.idx >0 && bob.idx < 0xFF) uart_print("\%*s%-18s = IDX(%i) {",ident,"", name, bob.idx);
		else {
			uart_print("\%*s%-18s = \"%s\"",ident,"", name, s);
		}
	}
	else uart_print("\%*s%-18s = \"N\\A\"",ident,"", name);
	uart_print("\r\n");
}

#define PRINT_VALUE_INT(NAME) printf("\%*s%-18s = %i\r\n",ident,"", #NAME, (int)(s->NAME));
#define PRINT_VALUE_BYTE(NAME) printf("\%*s%-18s = %2.2X\r\n",ident,"", #NAME, (int)(s->NAME));
#define PRINT_VALUE_WORD(NAME) printf("\%*s%-18s = %4.4X\r\n",ident,"", #NAME, (int)(s->NAME));
#define PRINT_VALUE_BCD(NAME) printf("\%*s%-18s = %X.%X\r\n",ident,"", #NAME,  (((int)(s->NAME)>>8)&0xFF),(int)((s->NAME)&0xFF));

#define PRINT_STRING_START(NAME) {  printf("\%*s%-18s = 0x%4.4X {",ident,"", #NAME, (int)DEV->NAME);
#define PRINT_STRING(STRING) printf(" %s",(STRING));
#define PRINT_STRING_END printf(" }\r\n"); }
#define PRINT_STRING_HEADER(...) printf("%*s",ident,""); printf(__VA_ARGS__); printf("\r\n");

#define PRINT_STRUCT_BEGIN(TYPE ) PRINT_STRUCT_DEFINE(TYPE) { \
	int ident = 3; \
	printf(#TYPE "\r\n"); \

#define PRINT_STRUCT_END(TYPE) }

PRINT_STRUCT_BEGIN(USB_DEVICE_DESCRIPTOR)
	PRINT_VALUE_INT(bLength);
	PRINT_VALUE_BYTE(bDescriptorType);
	PRINT_VALUE_BCD(bcdUSB);
	PRINT_VALUE_BYTE(bDeviceClass);
	PRINT_VALUE_BYTE(bDeviceSubClass);
	PRINT_VALUE_BYTE(bDeviceProtocol);
	PRINT_VALUE_INT(bMaxPacketSize0);
	PRINT_VALUE_WORD(idVendor);
	PRINT_VALUE_WORD(idProduct);
	PRINT_VALUE_BCD(bcdDevice);
	PRINT_VALUE_BYTE(iManufacturer);
	PRINT_VALUE_BYTE(iProduct);
	PRINT_VALUE_BYTE(iSerialNumber);
	PRINT_VALUE_INT(bNumConfigurations);
PRINT_STRUCT_END(USB_DEVICE_DESCRIPTOR)

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
void USBH_Print_RequestInit(const USBH_RequestInitTypeDef* value) {
	static const char* request_tbl[] = {
		"GET_STATUS",
		"CLEAR_FEATURE",
		"SET_FEATURE",
		"UNKNOWN(4)",
		"SET_ADDRESS",
		"GET_DESCRIPTOR",
		"SET_DESCRIPTOR",
		"GET_CONFIGURATION",
		"SET_CONFIGURATION",
		"GET_INTERFACE",
		"SET_INTERFACE",
		"SYNCH_FRAME",
	};
	puts("RequestType(");
	puts((value->RequestType&0x80) ? "DEVICE_TO_HOST " : "HOST_TO_DEVICE ");
	if(value->RequestType&(0x40|0x20)){
		if(value->RequestType & 0x20) puts("TYPE_CLASS ");
		if(value->RequestType & 0x40) puts("TYPE_VENDOR ");
	} else puts("TYPE_STANDARD ");
	switch(value->RequestType&0x03){
	case USB_SETUP_RECIPIENT_DEVICE: 	puts("RECIPIENT_DEVICE) "); break;
	case USB_SETUP_RECIPIENT_INTERFACE: puts("RECIPIENT_INTERFACE) "); break;
	case USB_SETUP_RECIPIENT_ENDPOINT:  puts("RECIPIENT_ENDPOINT) "); break;
	case USB_SETUP_RECIPIENT_OTHER:     puts("RECIPIENT_OTHER) "); break;
	}
	puts("Request(");
	puts(request_tbl[value->Request]);
	printf(" Value(%4.4X) Index(%4.4X) Length(%4.4X)", value->Value,value->Index,value->Length);
	puts("\r\n");
}
void USBH_Print_EndpointInit(const USBH_EndpointInitTypeDef* value){
	printf("DevAddr(%2.2X) EpAddr(%2.2X) MaxPacketSize(%i) Speed(%s) Type(%s)\r\n",
			value->DevAddr, value->Number, value->MaxPacketSize,
			ENUM_TO_STRING(USBH_PortSpeedTypeDef,value->Speed),
			ENUM_TO_STRING(USBH_EpRequestTypeDef,value->Type));
}

void USBH_Print_Endpoint(const USBH_EndpointTypeDef* value) {
	assert(value);
	static const char* ctrlTbl[] = {
		"EP_TYPE_CTRL",
		"EP_TYPE_ISOC",
		"EP_TYPE_BULK",
		"EP_TYPE_INTR",
	};
	printf("Pipe(%i) Type(%s) Direction(%s) Status(%s) UrbState(%s)\r\n",
			value->Channel,
			ctrlTbl[value->Type],
			value->Direction ? "IN" : "OUT",
			ENUM_TO_STRING(USBH_EndpointStatusTypeDef, value->status),
			ENUM_TO_STRING(USBH_URBStateTypeDef, value->urb_state));
	printf("    "); USBH_Print_EndpointInit(&value->Init);
}
void USBH_Print_Buffer(uint8_t* data, size_t length) {
		while(length){
			if(length>8) {
				printf("%2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X \r\n",
						data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);
				length-=8;
				data+=8;
			} else {
				 while(length--) printf("%2.2X ", *data++);
				 printf("\r\n");
				 break;
			}
		}
}
