#include "usbh_hid_keybd.h"
#include "usbh_hid_parser.h"
#include "HIDParser.h"

static HID_ReportInfo_t parser_data;
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

#define MAX_BUTTONS 20
#define MAX_HATS 3
HID_ReportItem_t* buttons[MAX_BUTTONS];
HID_ReportItem_t* x_axes = NULL;
HID_ReportItem_t* y_axes = NULL;
HID_ReportItem_t* z_axes = NULL;
HID_ReportItem_t* rz_axes = NULL;
HID_ReportItem_t* hats[MAX_HATS];
int hat_count = 0;
int button_count=0;

#if 0
static const HID_Report_ItemTypedef imp_0_lctrl={
  (uint8_t*)keybd_report_data+0, /*data*/
  1,     /*size*/
  0,     /*shift*/
  0,     /*count (only for array items)*/
  0,     /*signed?*/
  0,     /*min value read can return*/
  1,     /*max value read can return*/
  0,     /*min vale device can report*/
  1,     /*max value device can report*/
  1      /*resolution*/
};
#endif
static bool joystick_found = false;
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
		USBH_UsrLog("Button BitOffset=%u, ItemType=%X ReportID=%u Page=%X Usage=%X",
				CurrentItem->BitOffset, CurrentItem->ReportID, CurrentItem->Attributes.Usage.Page, CurrentItem->Attributes.Usage.Usage);
		buttons[button_count++] = CurrentItem; // found a button;
		return true;
	}
	if(CurrentItem->Attributes.Usage.Page == USAGE_PAGE_GENERIC_DCTRL) {
		// all the joysticks are here
		switch(CurrentItem->Attributes.Usage.Usage){
		case USAGE_X:  x_axes = CurrentItem; break;
		case USAGE_Y:	y_axes = CurrentItem; break;
		case USAGE_Z:	z_axes = CurrentItem; break;
		case USAGE_RZ:	rz_axes = CurrentItem; break;
		case USAGE_HAT:
			if(hat_count == MAX_HATS) return false; // to many hats?
			hats[hat_count++] = CurrentItem;
			break;
		default:
			USBH_UsrLog("Unkonwn joypad useage %u", CurrentItem->Attributes.Usage.Usage);
			return false;
		}
		return true;
	}
	return false;
//	USBH_UsrLog("CurrPath Type=%X Page=%X Usage=%X", CurrentItem->Type,CurrentItem->Usage.Page, CurrentItem->Usage.Usage);

	/* Check the attributes of the current item - see if we are interested in it or not;
	 * only store BUTTON and GENERIC_DESKTOP_CONTROL items into the Processed HID Report
	 * structure to save RAM and ignore the rest
	 */
	//return ((CurrentItem->Attributes.Usage.Page == USAGE_PAGE_BUTTON) ||
	 //      (CurrentItem->Attributes.Usage.Page == USAGE_PAGE_GENERIC_DCTRL));
}


/**
  * @brief  USBH_HID_KeybdInit
  *         The function init the HID keyboard.
  * @param  phost: Host handle
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_HID_JoystickDeInit(HID_HandleTypeDef *HID_Handle){
	joystick_found = false;
	button_count=0;
	hat_count=0;
	x_axes = NULL;
	y_axes = NULL;
	z_axes = NULL;
	rz_axes = NULL;
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

	USBH_UsrLog("USBH_HID_JoystickProcess Callback");
	 USBH_StatusTypeDef status = USBH_OK;
		  switch (HID_Handle->state)
		  {
		  case HID_INIT:
			if(HID_Handle->Init) HID_Handle->Init(HID_Handle);
		  case HID_IDLE:
			if(USBH_HID_GetReport (phost,
								   0x01,
									0,
									HID_Handle->pData,
									HID_Handle->length) == USBH_OK)
			{

			  fifo_write(&HID_Handle->fifo, HID_Handle->pData, HID_Handle->length);
			  HID_Handle->state = HID_SYNC;
			}

			break;

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
				if(HID_Handle->Callback) HID_Handle->Callback(HID_Handle);
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
bool DoAxes(const char* name, HID_ReportItem_t* axes) {
	if(axes && USB_GetHIDReportItemInfo(temp_buffer, axes)) {
		USBH_UsrLog("%s %i=%i",name, axes->Value);
		return true;
	}
	return false;
}

USBH_StatusTypeDef USBH_HID_Callback(HID_HandleTypeDef *HID_Handle){
	USBH_UsrLog("USBH_HID_Callback Callback");
	if(HID_Handle->length == 0) return USBH_FAIL;
		  /*Fill report */
	if(fifo_read(&HID_Handle->fifo, &temp_buffer, HID_Handle->length) ==  HID_Handle->length)
	{
		bool FoundData = true;
		for(uint32_t i=0;i < button_count;i++) {
			FoundData = USB_GetHIDReportItemInfo(temp_buffer, &buttons[i]);
			if(FoundData) USBH_UsrLog("Button %i=%i",i, buttons[i]->Value);
		}
		DoAxes("X", x_axes);
		DoAxes("Y",y_axes);
		DoAxes("Z", z_axes);
		DoAxes("RZ", rz_axes);
		return USBH_OK;
	}
	return USBH_OK;
}

USBH_StatusTypeDef USBH_HID_JoystickInit(HID_HandleTypeDef *HID_Handle)
{
	joystick_found = false;
	USBH_UsrLog("USBH_HID_GetHIDDescriptor length=%i", (int)HID_Handle->HID_Desc.wItemLength);
	print_buffer(HID_Handle->pData, HID_Handle->HID_Desc.wItemLength);
	uint8_t status = USB_ProcessHIDReport(HID_Handle->pData, HID_Handle->HID_Desc.wItemLength,&parser_data,GamePadFilter,NULL);
	USBH_UsrLog("USB_ProcessHIDReport status=%s",HD_Parse_ErrorCodes_String(status));
	if(!joystick_found) return USBH_FAIL;
	USBH_UsrLog("Joystick found with %u buttons and %u hats", button_count,hat_count);
	HID_Handle->Callback = USBH_HID_Callback;
	HID_Handle->DeInit = USBH_HID_JoystickDeInit;
	HID_Handle->Process = USBH_HID_JoystickProcess;
  //fifo_init(&HID_Handle->fifo, HID_Handle->pData, HID_QUEUE_SIZE * HID_Handle->pData);

  return USBH_OK;
}
