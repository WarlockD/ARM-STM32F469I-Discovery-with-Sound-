/*
 * usbh_hid_joystick.h
 *
 *  Created on: Jan 15, 2017
 *      Author: Paul
 */

#ifndef ST_HOST_LIBRARY_USBH_HID_JOYSTICK_H_
#define ST_HOST_LIBRARY_USBH_HID_JOYSTICK_H_


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbh_hid.h"

USBH_StatusTypeDef USBH_HID_JoystickInit(HID_HandleTypeDef *HID_Handle);



#ifdef __cplusplus
 };
#endif

/* Includes ------------------------------------------------------------------*/


#endif /* ST_HOST_LIBRARY_USBH_HID_JOYSTICK_H_ */
