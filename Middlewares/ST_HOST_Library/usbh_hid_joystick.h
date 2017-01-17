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

USBH_StatusTypeDef USBH_HID_JoystickInit(HID_HandleTypeDef *HID_Handle, const uint8_t* hid_report);

typedef enum {
	USBH_JOYSTICK_NOAXIS		=0,
	USBH_JOYSTICK_X_AXIS		,
	USBH_JOYSTICK_Y_AXIS		,
	USBH_JOYSTICK_Z_AXIS		,
	USBH_JOYSTICK_RZ_AXIS		,
	USBH_JOYSTICK_BRAKE_AXIS	,
	USBH_JOYSTICK_THROTTLE_AXIS	,
	USBH_JOYSTICK_HAT0			,
	USBH_JOYSTICK_HAT1			,
	USBH_JOYSTICK_MAX_AXIS		,
} USBH_JoystickAxisTypeDef;

typedef struct _USBH_JoystickAxisState {
	USBH_JoystickAxisTypeDef type;
	int32_t Value;
	int32_t Max;
	int32_t Min;
} USBH_JoystickAxisValueTypeDef;

typedef struct _USBH_ButtonState {
	uint8_t ButtonCount;	// count of buttons
	uint32_t PreviousState;	// bit array of buttons
	uint32_t ButtonState;	// bit array of buttons
} USBH_ButtonStateTypeDef ;

#define BUTTON_DOWN(STATE,BUTTON) ((STATE)->ButtonState & ((uint32_t)(1 << (BUTTON))))

USBH_JoystickAxisValueTypeDef* USBH_HID_GetJoystickAxisValue(USBH_JoystickAxisTypeDef type);
USBH_ButtonStateTypeDef* USBH_HID_GetJoystickButtons();


#ifdef __cplusplus
 };
#endif

/* Includes ------------------------------------------------------------------*/


#endif /* ST_HOST_LIBRARY_USBH_HID_JOYSTICK_H_ */
