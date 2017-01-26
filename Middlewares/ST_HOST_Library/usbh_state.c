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


void DeInitStateMachine(USBH_HandleTypeDef* phost);




static void ParseCfgDesc(USBH_HandleTypeDef* phost, USBH_DeviceTypeDef* dev, USBH_CfgDescTypeDef* cfg, uint8_t*data) {
	P_USBH_CfgDescTypeDef* pcfg = (P_USBH_CfgDescTypeDef*)data;
	uint8_t* end = data + pcfg->wTotalLength;
	data += pcfg->Header.bLength;
	assert(pcfg->Header.bDescriptorType == USB_DESC_TYPE_CONFIGURATION);
	cfg->NumInterfaces = pcfg->bNumInterfaces;
	cfg->ConfigurationValue = pcfg->bConfigurationValue;
	cfg->Configuration = (const char*)(int)pcfg->iConfiguration;
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
			iface->Interface = (const char*)(int)pface->iInterface;
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



void InitControlPipes(USBH_HandleTypeDef* phost) {
	// set up the control pipes
	if(!phost->Control.pipe_in) phost->Control.pipe_in=USBH_AllocPipe(phost,&phost->Control.pipe_in);
	if(!phost->Control.pipe_out) phost->Control.pipe_out=USBH_AllocPipe(phost,&phost->Control.pipe_out);

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
}

USBH_THREAD_BEGIN(GetMaxPacketSize)
	phost->Control.Init.RequestType = USB_D2H | USB_REQ_RECIPIENT_DEVICE | USB_REQ_TYPE_STANDARD;
	phost->Control.Init.Request  	= USB_REQ_GET_DESCRIPTOR;
	phost->Control.Init.Value  		= USB_DESC_DEVICE;
	phost->Control.Init.Index  		= 0;
	phost->Control.Init.Length  	= 8;
	phost->Control.Init.Data  		= phost->PacketData;
	USBH_WAIT_CTRL_REQUEST();			// Get max packet size

	//USBH_WAIT_UNTILL(phost->port_state == HOST_PORT_IDLE);

	P_USBH_DevDescTypeDef* dev_desc = (P_USBH_DevDescTypeDef*)phost->PacketData;
	USBH_DbgLog("Max Packet Size %i",dev_desc->bMaxPacketSize);
	phost->Control.pipe_in->Init.PacketSize = dev_desc->bMaxPacketSize; // got the packet size
	phost->Control.pipe_out->Init.PacketSize = dev_desc->bMaxPacketSize; // got the packet size

	/* modify control channels configuration for MaxPacket size */
	USBH_OpenPipe (phost,phost->Control.pipe_in);
	USBH_OpenPipe (phost,phost->Control.pipe_out);
	// should be the last time we need to modify the
	// pipes
USBH_THREAD_END()

USBH_THREAD_BEGIN(SetUSBAddress)
	if(dev->address == 0) dev->address = phost->DeviceCount;
	phost->Control.Init.RequestType = USB_H2D | USB_REQ_RECIPIENT_DEVICE |USB_REQ_TYPE_STANDARD;
	phost->Control.Init.Request  	= USB_REQ_SET_ADDRESS;
	phost->Control.Init.Value  		= dev->address ;
	phost->Control.Init.Index  		= 0;
	phost->Control.Init.Length  	= 0;
	phost->Control.Init.Data  		= NULL;
	USBH_WAIT_CTRL_REQUEST();			// Get max packet size
	USBH_Delay(2); // why the delay?

	/* user callback for device address assigned */
	USBH_UsrLog("Address (#%d) assigned.", dev->address);
	phost->Control.pipe_in->Init.Address = dev->address; // got the packet size
	phost->Control.pipe_out->Init.Address = dev->address; // got the packet size

	/* modify control channels to update device address */
	USBH_OpenPipe (phost,phost->Control.pipe_in);
	USBH_OpenPipe (phost,phost->Control.pipe_out);
USBH_THREAD_END()


USBH_THREAD_BEGIN(GetStringDescLangs)
	phost->Control.Init.RequestType  = USB_D2H | USB_REQ_RECIPIENT_DEVICE | USB_REQ_TYPE_STANDARD;
	phost->Control.Init.Request = USB_REQ_GET_DESCRIPTOR;
	phost->Control.Init.Value= USB_DESC_STRING;
	phost->Control.Init.Index = 0;
	phost->Control.Init.Length =0xff;
	phost->Control.Init.Data = phost->PacketData;
	USBH_WAIT_CTRL_REQUEST();			// Get max packet size

	memcpy(&dev->StringLangSupport,phost->PacketData+2,sizeof(uint16_t)*3);
	USBH_UsrLog("String Languages 0x%4.4X, 0x%4.4X, 0x%4.4X",dev->StringLangSupport[0],dev->StringLangSupport[1],dev->StringLangSupport[2]);
USBH_THREAD_END()


USBH_THREAD_BEGIN(GetStringDescrptor)
	USBH_USING_VALUE(string_index);
	USBH_USING_DATA(cc_str,const char**);
	if(string_index == 0){
		*cc_str = NULL; // string dosn't exist
		USBH_EXIT();
	}
	// search for it
	USBH_DescStringCacheTypeDef* str = dev->StringDesc;
	while(str) {
		if(string_index == str->Index) {
			*cc_str = str->Data;
			USBH_EXIT();
		}
		str = str->Next;
	}
	// no go, we have to get it from the device
	phost->Control.Init.RequestType = USB_D2H | USB_REQ_RECIPIENT_DEVICE | USB_REQ_TYPE_STANDARD;
	phost->Control.Init.Request=  USB_REQ_GET_DESCRIPTOR;
	phost->Control.Init.Value=  (0x03 << 8) | string_index; //USB_DESC_STRING | string_index;
	phost->Control.Init.Index= dev->StringLangSupport[0];
	phost->Control.Init.Data  = phost->PacketData;
	phost->Control.Init.Length = 0xFF;
	USBH_WAIT_CTRL_REQUEST();	// get string
	if(USBH_THREAD_STATUS==USBH_OK) {
		uint8_t len = (phost->PacketData[0] -2)/2;
		USBH_DescStringCacheTypeDef* str = staticmem_alloc(&dev->Memory,sizeof(USBH_DescStringCacheTypeDef)+ len,NULL);
		str->Index = string_index;
		str->Length = len;
		str->Next = dev->StringDesc;
		dev->StringDesc = str; // link
		// now copy, 16-bit unicode
		char* c_str = str->Data;
		char* u_str = (char*)phost->PacketData+2;
		while(len--) {
			*c_str = *u_str;
			c_str++;
			u_str+=2;
		}
		*c_str=0;
		USBH_USING_DATA(cc_str,const char**);
		*cc_str = (const char*)str->Data;
		USBH_DbgLog("Loaded String %i=%s", str->Index, str->Data);
	} else {
		USBH_USING_DATA(cc_str,const char**);
		*cc_str = NULL;
	}
USBH_THREAD_END()

USBH_THREAD_BEGIN(GetConfigDescrptor)
	USBH_USING_VALUE(cfg_index);
	USBH_USING_DATA(cfg,USBH_CfgDescTypeDef*);
	phost->Control.Init.RequestType = USB_D2H | USB_REQ_RECIPIENT_DEVICE | USB_REQ_TYPE_STANDARD;
	phost->Control.Init.Request  	= USB_REQ_GET_DESCRIPTOR;
	phost->Control.Init.Value  		= USB_DESC_CONFIGURATION;
	phost->Control.Init.Index  		= cfg_index;
	phost->Control.Init.Length  	= USB_CONFIGURATION_DESC_SIZE;
	phost->Control.Init.Data  		= phost->PacketData;
	USBH_WAIT_CTRL_REQUEST();		// size
	//if(status!=USBH_OK) // error, figurte out
	P_USBH_CfgDescTypeDef* pdev = (P_USBH_CfgDescTypeDef*)phost->PacketData;
	phost->Control.Init.Length  	= pdev->wTotalLength;
	USBH_WAIT_CTRL_REQUEST();		//  full
	print_buffer(phost->Control.Init.Data, phost->Control.Init.Length);
	dev->CfgDesc =  cfg;
	// we need to make this a thread so we can parse the strings in side of it
	ParseCfgDesc(phost, dev,  cfg,phost->PacketData);
USBH_THREAD_END()

USBH_THREAD_BEGIN(SetUSBConfig)
	USBH_USING_VALUE(cfg_index);
	phost->Control.Init.RequestType = USB_H2D | USB_REQ_RECIPIENT_DEVICE |USB_REQ_TYPE_STANDARD;
	phost->Control.Init.Request  	= USB_REQ_SET_CONFIGURATION;
	phost->Control.Init.Value  		= cfg_index;
	phost->Control.Init.Index  		= 0;
	phost->Control.Init.Length  	= 0;
	phost->Control.Init.Data  		= NULL;
	USBH_WAIT_CTRL_REQUEST();		// config set
USBH_THREAD_END()


USBH_THREAD_BEGIN(GetDeviceDescrptor)
	phost->Control.Init.RequestType = USB_D2H | USB_REQ_RECIPIENT_DEVICE | USB_REQ_TYPE_STANDARD;
	phost->Control.Init.Request  	= USB_REQ_GET_DESCRIPTOR;
	phost->Control.Init.Value  		= USB_DESC_DEVICE;
	phost->Control.Init.Index  		= 0;
	phost->Control.Init.Length  	= USB_DEVICE_DESC_SIZE;
	phost->Control.Init.Data  		= phost->PacketData;
	USBH_WAIT_CTRL_REQUEST();		// get discriptor
    /* Get FULL Device Desc and parse it */
	P_USBH_DevDescTypeDef* pdev = (P_USBH_DevDescTypeDef*)phost->PacketData;
	dev->DevDesc = (USBH_DevDescTypeDef*)staticmem_alloc(&dev->Memory,sizeof(USBH_DevDescTypeDef),NULL);
	dev->DevDesc->USB = pdev->bcdUSB;
	dev->DevDesc->DeviceClass = pdev->bDeviceClass;
	dev->DevDesc->DeviceSubClass = pdev->bDeviceSubClass;
	dev->DevDesc->DeviceProtocol = pdev->bDeviceProtocol;
	dev->DevDesc->MaxPacketSize = pdev->bMaxPacketSize;
	dev->DevDesc->VendorID = pdev->idVendor;
	dev->DevDesc->ProductID = pdev->idProduct;
	dev->DevDesc->Device = pdev->bcdDevice;
	dev->DevDesc->Manufacturer = (const char*)(int)pdev->iManufacturer;
	dev->DevDesc->Product = (const char*)(int)pdev->iProduct;
	dev->DevDesc->SerialNumber = (const char*)(int)pdev->iSerialNumber; // hack needed to cross the thread barrer

	dev->DevDesc->NumConfigurations = pdev->bNumConfigurations;
	dev->DevDesc->Configurations = (USBH_CfgDescTypeDef*)staticmem_alloc(&dev->Memory,sizeof(USBH_CfgDescTypeDef)*dev->DevDesc->NumConfigurations,NULL);
	USBH_UsrLog("PID: %xh", dev->DevDesc->ProductID );
	USBH_UsrLog("VID: %xh", dev->DevDesc->VendorID );
	USBH_UsrLog("Configurations: %i", dev->DevDesc->NumConfigurations );
	dev->CfgDesc = &dev->DevDesc->Configurations[0];

	USBH_SPAWN(GetStringDescrptor,&dev->DevDesc->Manufacturer,(int)dev->DevDesc->Manufacturer);
	USBH_SPAWN(GetStringDescrptor,&dev->DevDesc->Product,(int)dev->DevDesc->Product);
	USBH_SPAWN(GetStringDescrptor,&dev->DevDesc->SerialNumber,(int)dev->DevDesc->SerialNumber);
	USBH_SPAWN(GetConfigDescrptor, NULL,0);
	if(dev->DevDesc->NumConfigurations > 0) USBH_SPAWN(GetConfigDescrptor, NULL,1);
	if(dev->DevDesc->NumConfigurations > 1) USBH_SPAWN(GetConfigDescrptor, NULL,2);
	if(dev->DevDesc->NumConfigurations > 2) USBH_SPAWN(GetConfigDescrptor, NULL,3);
USBH_THREAD_END()

USBH_THREAD_BEGIN(Enumerate)
	USBH_UsrLog("USB Device Attached");
	/* Wait for 100 ms after Reset */
	USBH_Delay(100);  // phost->Control.pipe_size = USBH_MPS_DEFAULT;
	InitControlPipes(phost);
	// buffer used to allocate the data
	staticmem_init(&dev->Memory,dev->DescData, USBH_MAX_DATA_BUFFER);
	USBH_SPAWN(GetMaxPacketSize,NULL,0);
	USBH_SPAWN(SetUSBAddress,NULL,0);
	USBH_SPAWN(GetStringDescLangs,NULL,0);
	USBH_SPAWN(GetDeviceDescrptor,NULL,0);
	USBH_Print_DevDesc(0, dev->DevDesc);
	USBH_SPAWN(GetConfigDescrptor, &dev->DevDesc->Configurations[0],0); // just get the first comfig
	dev->CfgDesc = &dev->DevDesc->Configurations[0];
	USBH_SPAWN(SetUSBConfig,NULL,0);
	 // then set it
	USBH_Print_FullCfgDesc(0, dev->CfgDesc);

	USBH_UsrLog ("Enumeration done.");
USBH_THREAD_END()


USBH_StatusTypeDef  USBH_Process(USBH_HandleTypeDef *phost){
		switch(phost->port_state){
	case HOST_PORT_IDLE:
			USBH_RUN_PHOST(phost,Enumerate);
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

		// start the threading
		USBH_THREAD_INIT(&phost->PTThreads[0]);
		phost->PTThreadPos = 0;
		break;
	case HOST_PORT_DISCONNECTED:// wait for connect callback
	case HOST_PORT_ACTIVE: // wait for state change
	default: break;
	}

	return USBH_BUSY;
}
