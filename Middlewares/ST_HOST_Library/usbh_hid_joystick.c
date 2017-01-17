#include "usbh_hid_joystick.h"
#include "usbh_hid_parser.h"

#if 0

// bit slow though
HID_ReportItem_t* gamepad_buttons[30];
int gamepad_button_count = 0;
#define LEDMASK_USB_NOTREADY         LEDS_LED1

/** LED mask for the library LED driver, to indicate that the USB interface is enumerating. */
#define LEDMASK_USB_ENUMERATING     (LEDS_LED2 | LEDS_LED3)

/** LED mask for the library LED driver, to indicate that the USB interface is ready. */
#define LEDMASK_USB_READY           (LEDS_LED2 | LEDS_LED4)

/** LED mask for the library LED driver, to indicate that an error has occurred in the USB interface. */
#define LEDMASK_USB_ERROR           (LEDS_LED1 | LEDS_LED3)

/** HID Report Descriptor Usage Page value for a toggle button. */
#define USAGE_PAGE_BUTTON           0x09
#define USAGE_X          			0x30
#define USAGE_Y          			0x31
#define USAGE_Z          			0x32
#define USAGE_RZ          			0x35
#define USAGE_HAT          			0x39
#define PAGE_LED					0x08
#define USAGE_PAGE_LED_SLOW_BLINK_ON	0x43
#define USAGE_PAGE_LED_SLOW_BLINK_OFF	0x44
#define USAGE_PAGE_LED_FAST_BLINK_ON	0x45
#define USAGE_PAGE_LED_FAST_BLINK_OFF	0x46
/** HID Report Descriptor Usage Page value for a Generic Desktop Control. */
#define USAGE_PAGE_GENERIC_DCTRL    0x01

/** HID Report Descriptor Usage for a Joystick. */
#define USAGE_JOYSTICK              0x04
#define USAGE_GAMEPAD               0x05
/** HID Report Descriptor Usage value for a X axis movement. */
#define USAGE_X                     0x30

/** HID Report Descriptor Usage value for a Y axis movement. */
#define USAGE_Y                     0x31

USBH_ButtonStateTypeDef buttons = {0,0,0};
USBH_JoystickAxisValueTypeDef axis[USBH_JOYSTICK_MAX_AXIS];
uint32_t axis_mask = 0;
static bool joystick_found = false;
#define SET_AXIS_MASK(AXIS) { axis_mask |= (1<<((uint32_t)(AXIS))); }
#define TEST_AXIS_MASK(AXIS) ((axis_mask & (1<<((uint32_t)AXIS))) != 0)
USBH_JoystickAxisValueTypeDef* USBH_HID_GetJoystickAxisValue(USBH_JoystickAxisTypeDef type){
	if(TEST_AXIS_MASK(type))
		return &axis[type];
	else
		return NULL;
}
USBH_ButtonStateTypeDef* USBH_HID_GetJoystickButtons(){
	if(buttons.ButtonCount == 0) return NULL;
	return &buttons;
}


uint32_t bit_decode(const uint32_t* data, uint16_t offset, uint16_t size);


bool GamePadFilter(HID_ReportItem_t* const CurrentItem, void * context)
{
	bool IsJoystick = false;

	/* Iterate through the item's collection path, until either the root collection node or a collection with the
	 * Joystick Usage is found - this prevents Mice, which use identical descriptors except for the Joystick usage
	 * parent node, from being erroneously treated as a joystick by the demo
	 */
	for (HID_CollectionPath_t* CurrPath = CurrentItem->CollectionPath; CurrPath != NULL; CurrPath = CurrPath->Parent)
	{
		if ((CurrPath->Usage.Page  == USAGE_PAGE_GENERIC_DCTRL) &&
		    ((CurrPath->Usage.Usage == USAGE_JOYSTICK) || (CurrPath->Usage.Usage == USAGE_GAMEPAD)))
		{
			IsJoystick = true;
			break;
		}
	}

	/* If a collection with the joystick usage was not found, indicate that we are not interested in this item */
	if (!IsJoystick) return false;
	joystick_found = true;
	if(CurrentItem->Attributes.Usage.Page == USAGE_PAGE_BUTTON) {
		buttons.ButtonCount++;
		return true;
	}
	if(CurrentItem->Attributes.Usage.Page == USAGE_PAGE_GENERIC_DCTRL) {
		// all the joysticks are here
		switch(CurrentItem->Attributes.Usage.Usage){
		case USAGE_X:  SET_AXIS_MASK(USBH_JOYSTICK_X_AXIS);  break;
		case USAGE_Y:	SET_AXIS_MASK(USBH_JOYSTICK_Y_AXIS);  break;
		case USAGE_Z:	SET_AXIS_MASK(USBH_JOYSTICK_Z_AXIS);  break;
		case USAGE_RZ:	SET_AXIS_MASK(USBH_JOYSTICK_RZ_AXIS);  break;
		case USAGE_HAT:
			if(TEST_AXIS_MASK(USBH_JOYSTICK_HAT0))
				SET_AXIS_MASK(USBH_JOYSTICK_HAT1)
			else
				SET_AXIS_MASK(USBH_JOYSTICK_HAT0)
			break;
		default:
			DebugItem("Unkonwn", CurrentItem);
			return false;
		}
		return true;
	}
	return false;
}


/**
  * @brief  USBH_HID_KeybdInit
  *         The function init the HID keyboard.
  * @param  phost: Host handle
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_HID_JoystickDeInit(HID_HandleTypeDef *HID_Handle){
	joystick_found = false;
	buttons.ButtonCount = 0;
	axis_mask = 0;
	return USBH_OK;
}
/**
  * @brief  USBH_HID_KeybdDecode
  *         The function decode keyboard data.
  * @param  phost: Host handle
  * @retval USBH Status
  */
static uint8_t temp_buffer[2000];

USBH_StatusTypeDef USBH_HID_JoystickProcess(HID_HandleTypeDef *HID_Handle){
	USBH_HandleTypeDef *phost = HID_Handle->phost;
	USBH_StatusTypeDef status = USBH_OK;
	switch (HID_Handle->state)
	{
	// no idle stuff, purly interrupt driven
	  case HID_INIT:
		//if(HID_Handle->Init) HID_Handle->Init(HID_Handle);
		//HID_Handle->state = HID_IDLE;
		//break;
	  case HID_IDLE:
	  case HID_SYNC:

		/* Sync with start of Even Frame */
		if(phost->Timer & 1)
		{
		  HID_Handle->state = HID_GET_DATA;
		}
	#if (USBH_USE_OS == 1)
		osMessagePut ( phost->os_event, USBH_URB_EVENT, 0);
	#endif
		break;

	  case HID_GET_DATA:

		USBH_InterruptReceiveData(phost,
								  HID_Handle->pData,
								  HID_Handle->length,
								  HID_Handle->InPipe);

		HID_Handle->state = HID_POLL;
		HID_Handle->timer = phost->Timer;
		HID_Handle->DataReady = 0;
		break;

	  case HID_POLL:

		if(USBH_LL_GetURBState(phost , HID_Handle->InPipe) == USBH_URB_DONE)
		{
		  if(HID_Handle->DataReady == 0)
		  {
			fifo_write(&HID_Handle->fifo, HID_Handle->pData, HID_Handle->length);
			HID_Handle->DataReady = 1;
			if(HID_Handle->InterruptReceiveCallback) HID_Handle->InterruptReceiveCallback(HID_Handle);
			USBH_HID_EventCallback(phost,HID_Handle);
	#if (USBH_USE_OS == 1)
		osMessagePut ( phost->os_event, USBH_URB_EVENT, 0);
	#endif
		  }
		}
		else if(USBH_LL_GetURBState(phost , HID_Handle->InPipe) == USBH_URB_STALL) /* IN Endpoint Stalled */
		{

		  /* Issue Clear Feature on interrupt IN endpoint */
		  if(USBH_ClrFeature(phost,
							 HID_Handle->ep_addr) == USBH_OK)
		  {
			/* Change state to issue next IN token */
			HID_Handle->state = HID_GET_DATA;
		  }
		}


		break;

	  default:
		break;
	  }
  return status;
}


bool screen_cleared = false;
static HID_ReportInfo_t parser_data;
USBH_StatusTypeDef USBH_HID_Callback(HID_HandleTypeDef *HID_Handle){
	if(!screen_cleared){
		printf("\033[2J\033[1;1H");
		screen_cleared= true;
	}
//	printf("\033[2J\033[1;1H");
	printf("\033[0;0f");

	USBH_UsrLog("Report(%u) Incomming!",HID_Handle->fifo.buf[HID_Handle->fifo.tail]);
	if(HID_Handle->length == 0) return USBH_FAIL;
		  /*Fill report */
	if(fifo_read(&HID_Handle->fifo, &temp_buffer, HID_Handle->length) ==  HID_Handle->length)
	{
		for(uint32_t i=0; i<parser_data.TotalReportItems; i++){
			if(USB_GetHIDReportItemInfo(&temp_buffer[0], &parser_data.ReportItems[i]))
					printf("%i %2i=%i\r\n", i, parser_data.ReportItems[i].ItemType, parser_data.ReportItems[i].Value);
		}
		return USBH_OK;

	}
	return USBH_OK;
}
#endif
USBH_StatusTypeDef USBH_HID_JoystickInit(HID_HandleTypeDef *HID_Handle, const uint8_t* hid_report)
{

#if 0
	joystick_found = false;
	USBH_UsrLog("USBH_HID_GetHIDDescriptor length=%i", (int)HID_Handle->HID_Desc.wItemLength);
	memset(&parser_data,0,sizeof(HID_ReportInfo_t));
	print_buffer(HID_Handle->pData, HID_Handle->HID_Desc.wItemLength);
	uint8_t status = USB_ProcessHIDReport(hid_report, HID_Handle->HID_Desc.wItemLength,&parser_data,GamePadFilter,NULL);
	USBH_UsrLog("USB_ProcessHIDReport status=%s len=%u",HD_Parse_ErrorCodes_String(status),HID_Handle->length);
	//if(parser_data.UsingReportIDs) USBH_UsrLog("Joystick uses report id?", button_count,hat_count);
	if(!joystick_found) return USBH_FAIL;
	//USBH_UsrLog("Joystick found with %u buttons and %u hats", button_count,hat_count);
//	HID_Handle->Callback = USBH_HID_Callback;
	HID_Handle->DeInit = USBH_HID_JoystickDeInit;
	HID_Handle->DeInit = NULL;
	//HID_Handle->Process = USBH_HID_JoystickProcess;
  //fifo_init(&HID_Handle->fifo, HID_Handle->pData, HID_QUEUE_SIZE * HID_Handle->pData);
//	USBH_HID_GetReport(HID_Handle->phost, 0x01,buttons[0]->ReportID,HID_Handle->pData, HID_Handle->length);
#endif
  return USBH_OK;
}
