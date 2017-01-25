#include "usbh_core.h"
#include <assert.h>

typedef  struct  _DescHeader
{
    uint8_t  bLength;
    uint8_t  bDescriptorType;
} __attribute__((packed))  USBH_DescHeaderTypeDef;



typedef struct
{
	USBH_DescHeaderTypeDef Header;
  uint16_t  bcdUSB;        /* USB Specification Number which device complies too */
  uint8_t   bDeviceClass;
  uint8_t   bDeviceSubClass;
  uint8_t   bDeviceProtocol;
  /* If equal to Zero, each interface specifies its own class
  code if equal to 0xFF, the class code is vendor specified.
  Otherwise field is valid Class Code.*/
  uint8_t   bMaxPacketSize;
  uint16_t  idVendor;      /* Vendor ID (Assigned by USB Org) */
  uint16_t  idProduct;     /* Product ID (Assigned by Manufacturer) */
  uint16_t  bcdDevice;     /* Device Release Number */
  uint8_t   iManufacturer;  /* Index of Manufacturer String Descriptor */
  uint8_t   iProduct;       /* Index of Product String Descriptor */
  uint8_t   iSerialNumber;  /* Index of Serial Number String Descriptor */
  uint8_t   bNumConfigurations; /* Number of Possible Configurations */
}__attribute__((packed))  P_USBH_DevDescTypeDef;

typedef struct
{
	USBH_DescHeaderTypeDef Header;
  uint8_t   bEndpointAddress;   /* indicates what endpoint this descriptor is describing */
  uint8_t   bmAttributes;       /* specifies the transfer type. */
  uint16_t  wMaxPacketSize;    /* Maximum Packet Size this endpoint is capable of sending or receiving */
  uint8_t   bInterval;          /* is used to specify the polling interval of certain transfers. */
}__attribute__((packed))  P_USBH_EpDescTypeDef;

typedef struct
{
	USBH_DescHeaderTypeDef Header;
  uint8_t bInterfaceNumber;
  uint8_t bAlternateSetting;    /* Value used to select alternative setting */
  uint8_t bNumEndpoints;        /* Number of Endpoints used for this interface */
  uint8_t bInterfaceClass;      /* Class Code (Assigned by USB Org) */
  uint8_t bInterfaceSubClass;   /* Subclass Code (Assigned by USB Org) */
  uint8_t bInterfaceProtocol;   /* Protocol Code */
  uint8_t iInterface;           /* Index of String Descriptor Describing this interface */
}__attribute__((packed))  P_USBH_InterfaceDescTypeDef;


typedef struct
{
	USBH_DescHeaderTypeDef Header;
  uint16_t  wTotalLength;        /* Total Length of Data Returned */
  uint8_t   bNumInterfaces;       /* Number of Interfaces */
  uint8_t   bConfigurationValue;  /* Value to use as an argument to select this configuration*/
  uint8_t   iConfiguration;       /*Index of String Descriptor Describing this configuration */
  uint8_t   bmAttributes;         /* D7 Bus Powered , D6 Self Powered, D5 Remote Wakeup , D4..0 Reserved (0)*/
  uint8_t   bMaxPower;            /*Maximum Power Consumption */
} __attribute__((packed)) P_USBH_CfgDescTypeDef;

typedef struct _StringSupportedDescriptor {
	USBH_DescHeaderTypeDef Header;
	uint16_t langs[3];
} __attribute__((packed))  P_USBH_StringSupportedDescTypeDef;

typedef struct _StringDescriptor {
	USBH_DescHeaderTypeDef Header;
	uint16_t wStr[1];
} __attribute__((packed)) P_USBH_StringDescTypeDef;



int USBH_StartStackProcess(USBH_HandleTypeDef* phost, USBH_DeviceTypeDef* dev, HOST_StateTypeDef state, void* data, int value) {
	phost->StackPos++;
	USBH_StateInfoTypeDef* stack = &phost->StateStack[phost->StackPos];
	stack->Data = data;
	stack->Device = dev;
	stack->Value = value;
	stack->PrevState = HOST_IDLE;
	stack->State = state;
	return phost->StackPos;
}
void USBH_GetStringDiscrptor(USBH_HandleTypeDef* phost, USBH_DeviceTypeDef* dev, uint8_t index, const char** s) {
	if(index == 0) *s = NULL; // save some time doing this
	else {
		USBH_DbgLog("USBH_GetStringDiscrptor(%i)", index);
		USBH_StartStackProcess(phost, dev, HOST_STACK_GET_STRING, s, index);
	}
}

// from core, move here?
void DeInitStateMachine(USBH_HandleTypeDef* phost);
/**
  * @brief  USBH_HandleEnum
  *         This function includes the complete enumeration process
  * @param  phost: Host Handle
  * @retval USBH_Status
  */

typedef HOST_StateTypeDef(*USBH_StateCallbackTypeDef)(USBH_HandleTypeDef *phost, USBH_DeviceTypeDef* dev, void* data_ptr, int value);
#define PROCESS_ACTION(ENUM) ENUM##_Action
#define PROCESS_ACTION_DEFINE(ENUM) static HOST_StateTypeDef PROCESS_ACTION(ENUM)(USBH_HandleTypeDef *phost, USBH_DeviceTypeDef* dev, void* data_ptr, int value)
#define PROCESS_CALL_ACTION(ENUM) CTRL_ACTION(ENUM)(phost)

PROCESS_ACTION_DEFINE(HOST_STACK_GET_CFG_DESC);
PROCESS_ACTION_DEFINE(HOST_STACK_GET_FULL_CFG_DESC);
PROCESS_ACTION_DEFINE(HOST_STACK_GET_STRING);
PROCESS_ACTION_DEFINE(HOST_STACK_GET_STRING_DESC);

// we are set up here to return state because an interrupt can change these two
PROCESS_ACTION_DEFINE(HOST_IDLE) { return HOST_IDLE; }
PROCESS_ACTION_DEFINE(HOST_FINISHED) {
	if(phost->StackPos > 0){
		printf("%*s", phost->StackPos*3, "");
		USBH_DbgLog("POP STATE %i", phost->StackPos);
		--phost->StackPos; // back a state
	}
	return HOST_IDLE;
}


// PROCESS_ACTION_DEFINE(HOST_DETECT_DEVICE_SPEED) { return USBH_BUSY;} // not used
PROCESS_ACTION_DEFINE(HOST_ENUMERATION){
	USBH_UsrLog("USB Device Attached");

	   /* Wait for 100 ms after Reset */
	   USBH_Delay(100);  // phost->Control.pipe_size = USBH_MPS_DEFAULT;
	   // set up the control pipes
		phost->Control.pipe_in->Init.EpNumber   = 0;
		phost->Control.pipe_in->Init.Address  	 = 0x80;
		phost->Control.pipe_in->Init.Direction 	 = PIPE_DIR_IN;
		phost->Control.pipe_in->Init.DataType 	 = PIPE_PID_SETUP;
		phost->Control.pipe_in->Init.EpType 	 = PIPE_EP_CONTROL;
		phost->Control.pipe_in->Init.PacketSize  = USBH_SETUP_PKT_SIZE;
		phost->Control.pipe_in->Init.Speed 		 = phost->port_speed;

		phost->Control.pipe_out->Init.EpNumber   = 0;
		phost->Control.pipe_out->Init.Address  	 = 0x00;
		phost->Control.pipe_out->Init.Direction  = PIPE_DIR_OUT;
		phost->Control.pipe_out->Init.DataType 	 = PIPE_PID_SETUP;
		phost->Control.pipe_out->Init.EpType 	 = PIPE_EP_CONTROL;
		phost->Control.pipe_out->Init.PacketSize = USBH_SETUP_PKT_SIZE;
		phost->Control.pipe_out->Init.Speed 	 = phost->port_speed;

	   /* Open Control pipes */
	   USBH_OpenPipe (phost,phost->Control.pipe_in);
	   USBH_OpenPipe (phost,phost->Control.pipe_out);

	// buffer used to allocate the data
	staticmem_init(&dev->Memory,dev->DescData, USBH_MAX_DATA_BUFFER);
	return ENUM_GET_DEV_DESC;
}
PROCESS_ACTION_DEFINE(ENUM_GET_DEV_DESC){
	if(phost->Control.state == CTRL_SETUP){
		phost->Control.Init.RequestType = USB_D2H | USB_REQ_RECIPIENT_DEVICE | USB_REQ_TYPE_STANDARD;
		phost->Control.Init.Request  	= USB_REQ_GET_DESCRIPTOR;
		phost->Control.Init.Value  		= USB_DESC_DEVICE;
		phost->Control.Init.Index  		= 0;
		phost->Control.Init.Length  	= 8;
		phost->Control.Init.Data  		= phost->PacketData;
		}
		if (USBH_CtlReq(phost) == USBH_OK)
		{
			P_USBH_DevDescTypeDef* dev_desc = (P_USBH_DevDescTypeDef*)phost->PacketData;
			phost->Control.pipe_in->Init.PacketSize = dev_desc->bMaxPacketSize; // got the packet size
			phost->Control.pipe_out->Init.PacketSize = dev_desc->bMaxPacketSize; // got the packet size

			/* modify control channels configuration for MaxPacket size */
			USBH_OpenPipe (phost,phost->Control.pipe_in);
			USBH_OpenPipe (phost,phost->Control.pipe_out);
			// should be the last time we need to modify the
			// pipes
			return ENUM_SET_ADDR; // do the full thing now
		}
		return ENUM_GET_DEV_DESC;
}
PROCESS_ACTION_DEFINE(ENUM_SET_ADDR){
	if(phost->Control.state == CTRL_SETUP){
			if(dev->address == 0) dev->address = phost->DeviceCount;
			phost->Control.Init.RequestType = USB_H2D | USB_REQ_RECIPIENT_DEVICE |USB_REQ_TYPE_STANDARD;
			phost->Control.Init.Request  	= USB_REQ_SET_ADDRESS;
			phost->Control.Init.Value  		= dev->address ;
			phost->Control.Init.Index  		= 0;
			phost->Control.Init.Length  	= 0;
			phost->Control.Init.Data  		= NULL;
		}
	   if ( USBH_CtlReq(phost) == USBH_OK)
	   {
	     USBH_Delay(2);

	     /* user callback for device address assigned */
	     USBH_UsrLog("Address (#%d) assigned.", dev->address);
	     phost->Control.pipe_in->Init.Address = dev->address; // got the packet size
	     phost->Control.pipe_out->Init.Address = dev->address; // got the packet size

	     /* modify control channels to update device address */
	     USBH_OpenPipe (phost,phost->Control.pipe_in);
	     USBH_OpenPipe (phost,phost->Control.pipe_out);

	     return ENUM_GET_STRING_LANGS;
	   }
	   return ENUM_SET_ADDR;
}
PROCESS_ACTION_DEFINE(ENUM_GET_STRING_LANGS){
	if(phost->Control.state == CTRL_SETUP){
		phost->Control.Init.RequestType  = USB_D2H | USB_REQ_RECIPIENT_DEVICE | USB_REQ_TYPE_STANDARD;
		phost->Control.Init.Request = USB_REQ_GET_DESCRIPTOR;
		phost->Control.Init.Value= USB_DESC_STRING;
		phost->Control.Init.Index = 0;
		phost->Control.Init.Length =0xff;
		phost->Control.Init.Data = phost->PacketData;
	}
	if(USBH_CtlReq(phost) == USBH_OK){
		memcpy(&phost->StringLangSupport,phost->PacketData+2,sizeof(uint16_t)*3);
		USBH_UsrLog("String Languages 0x%4.4X, 0x%4.4X, 0x%4.4X",phost->StringLangSupport[0],phost->StringLangSupport[1],phost->StringLangSupport[2]);
		return ENUM_GET_FULL_DEV_DESC;
	}
	return ENUM_GET_STRING_LANGS;
}


PROCESS_ACTION_DEFINE(ENUM_GET_FULL_DEV_DESC){
	if(phost->Control.state == CTRL_SETUP){
		phost->Control.Init.RequestType = USB_D2H | USB_REQ_RECIPIENT_DEVICE | USB_REQ_TYPE_STANDARD;
		phost->Control.Init.Request  	= USB_REQ_GET_DESCRIPTOR;
		phost->Control.Init.Value  		= USB_DESC_DEVICE;
		phost->Control.Init.Index  		= 0;
		phost->Control.Init.Length  	= USB_DEVICE_DESC_SIZE;
		phost->Control.Init.Data  		= phost->PacketData;
	}
    /* Get FULL Device Desc and parse it */
    if(USBH_CtlReq(phost)== USBH_OK)
    {
    	P_USBH_DevDescTypeDef* pdev = (P_USBH_DevDescTypeDef*)phost->PacketData;
    	USBH_DevDescTypeDef* dev_desc = (USBH_DevDescTypeDef*)staticmem_alloc(&dev->Memory,sizeof(USBH_DevDescTypeDef),NULL);
    	dev->DevDesc = dev_desc;
    	dev_desc->USB = pdev->bcdUSB;
    	dev_desc->DeviceClass = pdev->bDeviceClass;
    	dev_desc->DeviceSubClass = pdev->bDeviceSubClass;
    	dev_desc->DeviceProtocol = pdev->bDeviceProtocol;
    	dev_desc->MaxPacketSize = pdev->bMaxPacketSize;
    	dev_desc->VendorID = pdev->idVendor;
    	dev_desc->ProductID = pdev->idProduct;
    	dev_desc->Device = pdev->bcdDevice;

    	dev_desc->NumConfigurations = pdev->bNumConfigurations;
    	dev_desc->Configurations = (USBH_CfgDescTypeDef*)staticmem_alloc(&dev->Memory,sizeof(USBH_CfgDescTypeDef)*dev_desc->NumConfigurations,NULL);
    	USBH_UsrLog("PID: %xh", dev_desc->ProductID );
    	USBH_UsrLog("VID: %xh", dev_desc->VendorID );
    	USBH_UsrLog("Configurations: %i", dev_desc->NumConfigurations );
		dev->CfgDesc = &dev_desc->Configurations[0];


    	for(int c=0;c < dev_desc->NumConfigurations; c++){
    		USBH_StartStackProcess(phost, dev, HOST_STACK_GET_CFG_DESC, &dev_desc->Configurations[c], c);
    	}
    	USBH_GetStringDiscrptor(phost,pdev->iManufacturer, dev, &dev_desc->Manufacturer );
    	USBH_GetStringDiscrptor(phost,pdev->iProduct, dev, &dev_desc->Product );
    	USBH_GetStringDiscrptor(phost,pdev->iSerialNumber, dev, &dev_desc->SerialNumber );
    	return HOST_ENUMERATION_FINISHED; // next state
    }
    return ENUM_GET_FULL_DEV_DESC;
}



PROCESS_ACTION_DEFINE(HOST_ENUMERATION_FINISHED){
	 /* user callback for end of device basic enumeration */
	 USBH_UsrLog ("Enumeration done.");
	 USBH_Print_DevDesc(0, dev->DevDesc);
	 USBH_Print_FullCfgDesc(0, dev->CfgDesc); // debug print the cfg
	  if(dev->DevDesc->NumConfigurations == 1)
	  {
		  USBH_UsrLog ("This device has only 1 configuration.");
		  return HOST_SET_CONFIGURATION;
	  }
	  else return HOST_INPUT;
}
PROCESS_ACTION_DEFINE(HOST_INPUT){
      /* user callback for end of device basic enumeration */
      if(phost->pUser != NULL)
      {
        phost->pUser(phost, HOST_USER_SELECT_CONFIGURATION);
#if (USBH_USE_OS == 1)
        osMessagePut ( phost->os_event, USBH_STATE_CHANGED_EVENT, 0);
#endif
      }
      return HOST_SET_CONFIGURATION;
}
PROCESS_ACTION_DEFINE(HOST_SET_CONFIGURATION){
	/* set configuration */
	if (USBH_SetCfg(phost, dev->CfgDesc->ConfigurationValue) == USBH_OK)
	{
	  USBH_UsrLog ("Default configuration set.");
	}
	return HOST_CHECK_CLASS;
}
PROCESS_ACTION_DEFINE(HOST_CHECK_CLASS){
	if(phost->ClassNumber == 0)
	    {
	      USBH_UsrLog ("No Class has been registered.");
	    }
	    else
	    {
	      phost->pActiveClass = NULL;

	      for (int idx = 0; idx < USBH_MAX_NUM_SUPPORTED_CLASS ; idx ++)
	      {
	        if(phost->pClass[idx]->ClassCode == dev->CfgDesc->Interfaces[0].InterfaceClass)
	        {
	          phost->pActiveClass = phost->pClass[idx];
	        }
	      }

	      if(phost->pActiveClass != NULL)
	      {
	        if(phost->pActiveClass->Init(phost,dev)== USBH_OK)
	        {
	          USBH_UsrLog ("%s class started.", phost->pActiveClass->Name);

	          /* Inform user that a class has been activated */
	          phost->pUser(phost, HOST_USER_CLASS_SELECTED);
	          return HOST_CLASS_REQUEST;
	        }
	        else USBH_UsrLog ("Device not supporting %s class.", phost->pActiveClass->Name);
	      }
	      else USBH_UsrLog ("No registered class for this device.");
	    }
	return HOST_ABORT_STATE;
}
PROCESS_ACTION_DEFINE(HOST_CLASS_REQUEST){
    /* process class standard control requests state machine */
	USBH_StatusTypeDef status = USBH_BUSY;
    if(phost->pActiveClass != NULL)
    {
    	USBH_StatusTypeDef status = phost->pActiveClass->Requests(phost,dev);

      if(status == USBH_OK) return HOST_CLASS;
    }
    USBH_ErrLog ("Invalid Class Driver.");
    return HOST_ABORT_STATE;
}
PROCESS_ACTION_DEFINE(HOST_CLASS){
    /* process class state machine */
    if(phost->pActiveClass == NULL) return HOST_ABORT_STATE;
     phost->pActiveClass->BgndProcess(phost,dev);
    return HOST_CLASS;
}

PROCESS_ACTION_DEFINE(HOST_ABORT_STATE) {
	// deinit till we restart
	return HOST_ABORT_STATE;
}



typedef struct {
	USBH_StateCallbackTypeDef action;
	HOST_StateTypeDef value;
	const char* name;
} PROCESS_StateTableEntry;

#define  PROCESS_TABLE_ENTRY(ENUM) { PROCESS_ACTION(ENUM), ENUM, #ENUM }
#define  PROCESS_TABLE_NULL(ENUM) { NULL, ENUM, #ENUM }

static const PROCESS_StateTableEntry process_state_table[USBH_MAX_STATES]={
	PROCESS_TABLE_ENTRY(HOST_IDLE),
	PROCESS_TABLE_NULL(HOST_FINISHED),
	PROCESS_TABLE_ENTRY(HOST_ENUMERATION),
		PROCESS_TABLE_ENTRY(ENUM_GET_DEV_DESC),
		PROCESS_TABLE_ENTRY(ENUM_SET_ADDR),
		PROCESS_TABLE_ENTRY(ENUM_GET_STRING_LANGS),
		PROCESS_TABLE_ENTRY(ENUM_GET_FULL_DEV_DESC),
	PROCESS_TABLE_ENTRY(HOST_ENUMERATION_FINISHED),
	PROCESS_TABLE_ENTRY(HOST_CLASS_REQUEST),
	PROCESS_TABLE_ENTRY(HOST_INPUT),
	PROCESS_TABLE_ENTRY(HOST_SET_CONFIGURATION),
	PROCESS_TABLE_ENTRY(HOST_CHECK_CLASS),
	PROCESS_TABLE_ENTRY(HOST_CLASS),
	PROCESS_TABLE_NULL(HOST_SUSPENDED),
	PROCESS_TABLE_ENTRY(HOST_ABORT_STATE),
	PROCESS_TABLE_ENTRY(HOST_STACK_GET_STRING),
	PROCESS_TABLE_ENTRY(HOST_STACK_GET_STRING_DESC),
	PROCESS_TABLE_ENTRY(HOST_STACK_GET_CFG_DESC),
	PROCESS_TABLE_ENTRY(HOST_STACK_GET_FULL_CFG_DESC),
};

const char* HOST_StateTypeToString(HOST_StateTypeDef t){
	const PROCESS_StateTableEntry* e = &process_state_table[t];
	return e->name;
}

static void ParseCfgDesc(USBH_HandleTypeDef* phost, USBH_DeviceTypeDef* dev, USBH_CfgDescTypeDef* cfg, uint8_t*data) {
	P_USBH_CfgDescTypeDef* pcfg = (P_USBH_CfgDescTypeDef*)data;
	uint8_t* end = data + pcfg->wTotalLength;
	data += pcfg->Header.bLength;
	assert(pcfg->Header.bDescriptorType == USB_DESC_TYPE_CONFIGURATION);
	cfg->NumInterfaces = pcfg->bNumInterfaces;
	cfg->ConfigurationValue = pcfg->bConfigurationValue;
	USBH_GetStringDiscrptor(phost,dev, pcfg->iConfiguration, &cfg->Configuration );
	cfg->Attributes = pcfg->bmAttributes;
	cfg->MaxPower = pcfg->bMaxPower;
	cfg->Interfaces = (USBH_InterfaceDescTypeDef*)staticmem_alloc(&dev->Memory,sizeof(USBH_InterfaceDescTypeDef)*cfg->NumInterfaces,NULL);
	// cause I was burned by the order the descrptors were in, I need to do a full parse of
	// all the data
	USBH_InterfaceDescTypeDef* iface = &cfg->Interfaces[0];
	USBH_EpDescTypeDef* ep = NULL;
	P_USBH_InterfaceDescTypeDef* pface;
	P_USBH_EpDescTypeDef* pep;
	int iface_count=0;
	int ep_count = 0;
	while(data < end){
		switch(data[1]) { // get type
		case USB_DESC_TYPE_INTERFACE:
			assert(iface_count < cfg->NumInterfaces);
			iface = &cfg->Interfaces[iface_count];
			USBH_ErrLog("Processing iface=%i", iface_count);
			pface = (P_USBH_InterfaceDescTypeDef*)data;
			data += pface->Header.bLength;
			iface->InterfaceNumber = pface->bInterfaceNumber;
			iface->AlternateSetting = pface->bAlternateSetting;
			iface->NumEndpoints = pface->bNumEndpoints;
			iface->InterfaceClass = pface->bInterfaceClass;
			iface->InterfaceSubClass = pface->bInterfaceSubClass;
			iface->InterfaceProtocol = pface->bInterfaceProtocol;
			USBH_GetStringDiscrptor(phost,dev, pface->iInterface, &iface->Interface );
			iface->InterfaceNumber = pface->bInterfaceNumber;
			iface->Endpoints = (USBH_EpDescTypeDef*)staticmem_alloc(&dev->Memory,sizeof(USBH_EpDescTypeDef)*iface->NumEndpoints,NULL);
			iface_count++;
			ep_count=0; // clear ep count for loading
			break;
		case USB_DESC_TYPE_ENDPOINT:
			assert(iface);
			assert(ep_count < iface->NumEndpoints);
			ep = &iface->Endpoints[ep_count];
			USBH_ErrLog("Processing endpoint=%i", ep_count);
			pep = (P_USBH_EpDescTypeDef*)data;
			data += pep->Header.bLength;
			ep->EndpointAddress =  pep->bEndpointAddress ;
			ep->Attributes =  pep->bmAttributes ;
			ep->MaxPacketSize =  pep->wMaxPacketSize ;
			ep->Interval =  pep->bInterval ;
			ep_count++;
			break;
		default:
			USBH_ErrLog("Unprocessed Discriptor %i", (int)data[1]);
			assert(data[0]);
			data +=data[0];
			break;
		}
	}
}

PROCESS_ACTION_DEFINE(HOST_STACK_GET_CFG_DESC){
	USBH_StatusTypeDef status;
	if(phost->Control.state == CTRL_SETUP){
			USBH_StateInfoTypeDef* current = USBH_CurrentState(phost);
			phost->Control.Init.RequestType = USB_D2H | USB_REQ_RECIPIENT_DEVICE | USB_REQ_TYPE_STANDARD;
			phost->Control.Init.Request  	= USB_REQ_GET_DESCRIPTOR;
			phost->Control.Init.Value  		= USB_DESC_CONFIGURATION;
			phost->Control.Init.Index  		= current->Value;
			phost->Control.Init.Length  	= USB_CONFIGURATION_DESC_SIZE;
			phost->Control.Init.Data  		= phost->PacketData;
	}
	if((status = USBH_CtlReq(phost)) == USBH_OK) {
		P_USBH_CfgDescTypeDef* pdev = (P_USBH_CfgDescTypeDef*)phost->PacketData;
		phost->Control.Init.Length  	= pdev->wTotalLength;
		//USBH_CtlReq(phost); // start it up again with the new length
		return HOST_STACK_GET_FULL_CFG_DESC;
	}
	return HOST_STACK_GET_CFG_DESC;
}

PROCESS_ACTION_DEFINE(HOST_STACK_GET_FULL_CFG_DESC){
	USBH_StatusTypeDef status = USBH_CtlReq(phost);
	assert(status != USBH_FAIL);
	if(status == USBH_OK) {
		USBH_StateInfoTypeDef* current = USBH_CurrentState(phost);
		print_buffer(phost->Control.Init.Data, phost->Control.Init.Length);
		USBH_CfgDescTypeDef* cfg = (USBH_CfgDescTypeDef*)current->Data;
		ParseCfgDesc(phost, dev, cfg, phost->PacketData);
		dev->CfgDesc =  cfg;
		return HOST_FINISHED;
	}
	return HOST_STACK_GET_FULL_CFG_DESC;
}
PROCESS_ACTION_DEFINE(HOST_STACK_GET_STRING){
	USBH_StateInfoTypeDef* current = USBH_CurrentState(phost);
	uint8_t string_index= current->Value;
	const char** c_str = (const char**)current->Data;
	if(string_index == 0){
		*c_str = NULL; // string dosn't exist
		return USBH_FAIL;
	}
	// search for it
	USBH_DescStringCacheTypeDef* str = dev->StringDesc;
	while(str) {
		if(string_index == str->Index) {
			*c_str = str->Data;
			return USBH_OK;
		}
		str = str->Next;
	}
	// no go, we have to get it from the device
	phost->Control.Init.RequestType = USB_D2H | USB_REQ_RECIPIENT_DEVICE | USB_REQ_TYPE_STANDARD;
	phost->Control.Init.Request=  USB_REQ_GET_DESCRIPTOR;
	phost->Control.Init.Value=  (0x03 << 8) | string_index; //USB_DESC_STRING | string_index;
	phost->Control.Init.Index= phost->StringLangSupport[0];
	phost->Control.Init.Data  = phost->PacketData;
	phost->Control.Init.Length = 0xFF;
	return HOST_STACK_GET_STRING_DESC;
}
PROCESS_ACTION_DEFINE(HOST_STACK_GET_STRING_DESC){
	USBH_StatusTypeDef status = USBH_CtlReq(phost);
	if(status == USBH_BUSY) return HOST_STACK_GET_STRING_DESC;
	if(status==USBH_OK) {
		USBH_StateInfoTypeDef* current = USBH_CurrentState(phost);
		uint8_t string_index= current->Value;
		//const char** c_str = (const char**)phost->Current->Data;
		uint8_t len = (phost->PacketData[0] -2)/2;
		USBH_DescStringCacheTypeDef* str = staticmem_alloc(&dev->Memory,sizeof(USBH_DescStringCacheTypeDef)+ len,NULL);
		str->Index = string_index;
		str->Length = len;
		str->Next = dev->StringDesc;
		dev->StringDesc = str; // link
		// now copy, 16-bit unicode
		char* c_str = str->Data;
		char* u_str = phost->PacketData+2;
		while(len--) {
			*c_str = *u_str;
			c_str++;
			u_str+=2;
		}
		*c_str=0;
		*((const char**)current->Data) = str->Data;
		USBH_DbgLog("Loaded String %i=%s", str->Index, str->Data);
	}else {
		USBH_StateInfoTypeDef* current = USBH_CurrentState(phost);
		*((const char**)current->Data) = NULL;
	}
	return HOST_FINISHED;
}

USBH_StatusTypeDef  USBH_Process(USBH_HandleTypeDef *phost){
		switch(phost->port_state){
	case HOST_PORT_IDLE:
	{
		USBH_StateInfoTypeDef* current = &phost->StateStack[phost->StackPos];
			HOST_StateTypeDef prev = current->State;
			if(prev !=process_state_table[prev].value){
				USBH_UsrLog("USBH_Process BAD ORDER! %s != %s", HOST_StateTypeToString(prev),HOST_StateTypeToString(process_state_table[prev].value));
				while(1);
			}
			current->State = process_state_table[current->State].action(phost,current->Device,current->Data,current->Value);
			if(current->State != prev){
				// debug change state
				if(phost->StackPos > 0) printf("%*s", phost->StackPos*3, "");
				USBH_DbgLog("%i: %s -> %s", phost->StackPos, HOST_StateTypeToString(prev),HOST_StateTypeToString(current->State));
			}
			current->PrevState = prev;
	}
	break;
		break; // doing nothing
	case HOST_PORT_CONNECTED:
	{
		phost->port_state =HOST_PORT_WAIT_FOR_ATTACHMENT;
		USBH_Delay(200); // ORDER MATTERS
		USBH_LL_ResetPort(phost);
		break;
	}
	case HOST_PORT_WAIT_FOR_ATTACHMENT: // wait for state change
		phost->port_state = HOST_PORT_IDLE;
		phost->StateStack[0].State = HOST_ENUMERATION; // start it up
		phost->StateStack[0].Device = &phost->Devices[0];
		phost->DeviceCount=1;
		phost->StackPos = 0;
		phost->port_speed = USBH_LL_GetSpeed(phost);
		if(!phost->Control.pipe_in) USBH_AllocPipe(phost,&phost->Control.pipe_in);
		if(!phost->Control.pipe_out) USBH_AllocPipe(phost,&phost->Control.pipe_out);
		break;
	case HOST_PORT_DISCONNECTED:// wait for connect callback
	case HOST_PORT_ACTIVE: // wait for state change
	default: break;
	}

	return USBH_BUSY;
}
