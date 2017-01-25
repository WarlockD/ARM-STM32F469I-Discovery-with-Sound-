/*
 * usbh_hub.c
 *
 *  Created on: Jan 24, 2017
 *      Author: Paul
 */
#include "usbh_hub.h"

#if 0
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
  "HUB",
  USB_HID_CLASS,
  USBH_HID_InterfaceInit,
  USBH_HID_InterfaceDeInit,
  USBH_HID_ClassRequest,
  USBH_HID_Process,
  USBH_HID_SOFProcess,
  NULL,
};


extern USBH_ClassTypeDef  USB_HUB_Class;
#define USBH_HUB_CLASS    &USB_HUB_Class

USBH_StatusTypeDef USBH_HUB_Init(HID_HandleTypeDef *phost){

}
#endif
