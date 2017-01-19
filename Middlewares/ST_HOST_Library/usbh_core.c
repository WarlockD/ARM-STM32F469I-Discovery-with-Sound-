/**
  ******************************************************************************
  * @file    usbh_core.c 
  * @author  MCD Application Team
  * @version V3.2.2
  * @date    07-July-2015
  * @brief   This file implements the functions for the core state machine process
  *          the enumeration and the control transfer process
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 
/* Includes ------------------------------------------------------------------*/

#include "usbh_core.h"
#include <assert.h>

/** @addtogroup USBH_LIB
  * @{
  */

/** @addtogroup USBH_LIB_CORE
  * @{
  */

/** @defgroup USBH_CORE 
  * @brief This file handles the basic enumeration when a device is connected 
  *          to the host.
  * @{
  */ 


/** @defgroup USBH_CORE_Private_Defines
  * @{
  */ 
#define USBH_ADDRESS_DEFAULT                     0
#define USBH_ADDRESS_ASSIGNED                    1      
#define USBH_MPS_DEFAULT                         0x40
/**
  * @}
  */ 

/** @defgroup USBH_CORE_Private_Macros
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup USBH_CORE_Private_Variables
  * @{
  */ 
/**
  * @}
  */ 
 

/** @defgroup USBH_CORE_Private_Functions
  * @{
  */ 

static void                USBH_HandleSof     (USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef  DeInitStateMachine(USBH_HandleTypeDef *phost);

static HOST_StateTypeDef USBH_PopState(USBH_HandleTypeDef* phost) {
	assert(phost->StackSize > 0);
	--phost->StackSize;
	return phost->StateStack[phost->StackSize];
}
static void USBH_PushState(USBH_HandleTypeDef* phost,HOST_StateTypeDef state) {
	assert(MAX_STATE_STACK > phost->StackSize);
	phost->StateStack[phost->StackSize]=state;
	++phost->StackSize;
}
#if (USBH_USE_OS == 1)  
static void USBH_Process_OS(void const * argument);
#endif

/**
  * @brief  HCD_Init 
  *         Initialize the HOST Core.
  * @param  phost: Host Handle
  * @param  pUsrFunc: User Callback
  * @retval USBH Status
  */
USBH_StatusTypeDef  USBH_Init(USBH_HandleTypeDef *phost, void (*pUsrFunc)(USBH_HandleTypeDef *phost, uint8_t ), uint8_t id)
{
  /* Check whether the USB Host handle is valid */
  if(phost == NULL)
  {
    USBH_ErrLog("Invalid Host handle");
    return USBH_FAIL; 
  }
  
  /* Set DRiver ID */
  phost->id = id;

  /* Unlink class*/
  phost->pActiveClass = NULL;
  phost->ClassNumber = 0;

  /* Restore default states and prepare EP0 */ 
  DeInitStateMachine(phost);
  
  /* Assign User process */
  if(pUsrFunc != NULL)
  {
    phost->pUser = pUsrFunc;
  }
  
#if (USBH_USE_OS == 1) 
  
  /* Create USB Host Queue */
  osMessageQDef(USBH_Queue, 10, uint16_t);
  phost->os_event = osMessageCreate (osMessageQ(USBH_Queue), NULL); 
  
  /*Create USB Host Task */
#if defined (USBH_PROCESS_STACK_SIZE)
  osThreadDef(USBH_Thread, USBH_Process_OS, USBH_PROCESS_PRIO, 0, USBH_PROCESS_STACK_SIZE);
#else
  osThreadDef(USBH_Thread, USBH_Process_OS, USBH_PROCESS_PRIO, 0, 8 * configMINIMAL_STACK_SIZE);
#endif  
  phost->thread = osThreadCreate (osThread(USBH_Thread), phost);
#endif  
  
  /* Initialize low level driver */
  USBH_LL_Init(phost);
  return USBH_OK;
}

/**
  * @brief  HCD_Init 
  *         De-Initialize the Host portion of the driver.
  * @param  phost: Host Handle
  * @retval USBH Status
  */
USBH_StatusTypeDef  USBH_DeInit(USBH_HandleTypeDef *phost)
{
  DeInitStateMachine(phost);
  
  if(phost->pData != NULL)
  {
    phost->pActiveClass->pData = NULL;
    USBH_LL_Stop(phost);
  }

  return USBH_OK;
}

/**
  * @brief  DeInitStateMachine 
  *         De-Initialize the Host state machine.
  * @param  phost: Host Handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef  DeInitStateMachine(USBH_HandleTypeDef *phost)
{
  uint32_t i = 0;

  /* Clear Pipes flags*/
  for ( ; i < USBH_MAX_PIPES_NBR; i++) phost->Pipes[i] = 0;
  memset(&phost->device,0,sizeof(USBH_DeviceTypeDef));

  phost->gState = HOST_IDLE;
  phost->RequestState = CMD_SEND;
  phost->Timer = 0;  
  
  phost->Control.state = CTRL_SETUP;
  phost->Control.pipe_size = USBH_MPS_DEFAULT;  
  phost->Control.errorcount = 0;
  
  phost->device.address = USBH_ADDRESS_DEFAULT;
  phost->device.speed   = USBH_SPEED_FULL;

  return USBH_OK;
}

/**
  * @brief  USBH_RegisterClass 
  *         Link class driver to Host Core.
  * @param  phost : Host Handle
  * @param  pclass: Class handle
  * @retval USBH Status
  */
USBH_StatusTypeDef  USBH_RegisterClass(USBH_HandleTypeDef *phost, USBH_ClassTypeDef *pclass)
{
  USBH_StatusTypeDef   status = USBH_OK;
  
  if(pclass != 0)
  {
    if(phost->ClassNumber < USBH_MAX_NUM_SUPPORTED_CLASS)
    {
      /* link the class to the USB Host handle */
      phost->pClass[phost->ClassNumber++] = pclass;
      status = USBH_OK;
    }
    else
    {
      USBH_ErrLog("Max Class Number reached");
      status = USBH_FAIL; 
    }
  }
  else
  {
    USBH_ErrLog("Invalid Class handle");
    status = USBH_FAIL; 
  }
  
  return status;
}

/**
  * @brief  USBH_SelectInterface 
  *         Select current interface.
  * @param  phost: Host Handle
  * @param  interface: Interface number
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_SelectInterface(USBH_HandleTypeDef *phost, uint8_t interface)
{
  USBH_StatusTypeDef   status = USBH_OK;
  
  USBH_UsrLog ("This device has %i interfaces\r\n", phost->device.CfgDesc->bNumInterfaces);
  if(interface < phost->device.CfgDesc->bNumInterfaces)
  {
    phost->device.current_interface = interface;
    USBH_UsrLog ("Switching to Interface (#%d)", interface);
    USBH_UsrLog ("Class    : %xh", phost->device.CfgDesc->Itf_Desc[interface].bInterfaceClass );
    USBH_UsrLog ("SubClass : %xh", phost->device.CfgDesc->Itf_Desc[interface].bInterfaceSubClass );
    USBH_UsrLog ("Protocol : %xh", phost->device.CfgDesc->Itf_Desc[interface].bInterfaceProtocol );
  }
  else
  {
    USBH_ErrLog ("Cannot Select This Interface.");
    status = USBH_FAIL; 
  }
  return status;  
}

/**
  * @brief  USBH_GetActiveClass 
  *         Return Device Class.
  * @param  phost: Host Handle
  * @param  interface: Interface index
  * @retval Class Code
  */
uint8_t USBH_GetActiveClass(USBH_HandleTypeDef *phost)
{
   return (phost->device.CfgDesc->Itf_Desc[phost->device.current_interface].bInterfaceClass);
}
/**
  * @brief  USBH_FindInterface 
  *         Find the interface index for a specific class.
  * @param  phost: Host Handle
  * @param  Class: Class code
  * @param  SubClass: SubClass code
  * @param  Protocol: Protocol code
  * @retval interface index in the configuration structure
  * @note : (1)interface index 0xFF means interface index not found
  */
uint8_t  USBH_FindInterface(USBH_HandleTypeDef *phost, uint8_t Class, uint8_t SubClass, uint8_t Protocol)
{
  USBH_InterfaceDescTypeDef    *pif ;
  USBH_CfgDescTypeDef          *pcfg ;
  int8_t                        if_ix = 0;
  
  pif = (USBH_InterfaceDescTypeDef *)0;
  pcfg = phost->device.CfgDesc;
  
  while (if_ix < phost->device.CfgDesc->bNumInterfaces)
  {
    pif = &pcfg->Itf_Desc[if_ix];
    if(((pif->bInterfaceClass == Class) || (Class == 0xFF))&&
       ((pif->bInterfaceSubClass == SubClass) || (SubClass == 0xFF))&&
         ((pif->bInterfaceProtocol == Protocol) || (Protocol == 0xFF)))
    {
      return  if_ix;
    }
    if_ix++;
  }
  return 0xFF;
}

/**
  * @brief  USBH_FindInterfaceIndex 
  *         Find the interface index for a specific class interface and alternate setting number.
  * @param  phost: Host Handle
  * @param  interface_number: interface number
  * @param  alt_settings    : alternate setting number
  * @retval interface index in the configuration structure
  * @note : (1)interface index 0xFF means interface index not found
  */
uint8_t  USBH_FindInterfaceIndex(USBH_HandleTypeDef *phost, uint8_t interface_number, uint8_t alt_settings)
{
  USBH_InterfaceDescTypeDef    *pif ;
  USBH_CfgDescTypeDef          *pcfg ;
  int8_t                        if_ix = 0;
  
  pif = (USBH_InterfaceDescTypeDef *)0;
  pcfg = phost->device.CfgDesc;
  
  while (if_ix < phost->device.CfgDesc->bNumInterfaces)
  {
    pif = &pcfg->Itf_Desc[if_ix];
    if((pif->bInterfaceNumber == interface_number) && (pif->bAlternateSetting == alt_settings))
    {
      return  if_ix;
    }
    if_ix++;
  }
  return 0xFF;
}

/**
  * @brief  USBH_Start 
  *         Start the USB Host Core.
  * @param  phost: Host Handle
  * @retval USBH Status
  */
USBH_StatusTypeDef  USBH_Start  (USBH_HandleTypeDef *phost)
{
  /* Start the low level driver  */
  USBH_LL_Start(phost);
  
  /* Activate VBUS on the port */ 
  USBH_LL_DriverVBUS (phost, TRUE);
  
  return USBH_OK;  
}

/**
  * @brief  USBH_Stop 
  *         Stop the USB Host Core.
  * @param  phost: Host Handle
  * @retval USBH Status
  */
USBH_StatusTypeDef  USBH_Stop   (USBH_HandleTypeDef *phost)
{
  /* Stop and cleanup the low level driver  */
  USBH_LL_Stop(phost);  
  
  /* DeActivate VBUS on the port */ 
  USBH_LL_DriverVBUS (phost, FALSE);
  
  /* FRee Control Pipes */
  USBH_FreePipe  (phost, phost->Control.pipe_in);
  USBH_FreePipe  (phost, phost->Control.pipe_out);  
  
  return USBH_OK;  
}

/**
  * @brief  HCD_ReEnumerate 
  *         Perform a new Enumeration phase.
  * @param  phost: Host Handle
  * @retval USBH Status
  */
USBH_StatusTypeDef  USBH_ReEnumerate   (USBH_HandleTypeDef *phost)
{
  /*Stop Host */ 
  USBH_Stop(phost);

  /*Device has disconnected, so wait for 200 ms */  
  USBH_Delay(200);
  
  /* Set State machines in default state */
  DeInitStateMachine(phost);
   
  /* Start again the host */
  USBH_Start(phost);
      
#if (USBH_USE_OS == 1)
      osMessagePut ( phost->os_event, USBH_PORT_EVENT, 0);
#endif  
  return USBH_OK;  
}
#define PRINT_ENUM(ENUM) case ENUM: return #ENUM;
const char* ENUM_TO_STRING(HOST_StateTypeDef t){
	switch(t){
		PRINT_ENUM(UTILITY_GET_STRING_DISCRIPTOR);
		PRINT_ENUM(UTILITY_GET_DISCRIPTOR);
		PRINT_ENUM(UTILITY_PUSH_STATE);
		PRINT_ENUM(UTILITY_POP_STATE);
		PRINT_ENUM(HOST_IDLE);
		PRINT_ENUM(HOST_DEV_WAIT_FOR_ATTACHMENT);
		PRINT_ENUM(HOST_DEV_ATTACHED);
		PRINT_ENUM(HOST_DEV_DISCONNECTED);
		PRINT_ENUM(HOST_DETECT_DEVICE_SPEED);
		PRINT_ENUM(HOST_ENUMERATION);
		PRINT_ENUM(HOST_ENUMERATION_FINISHED);
		PRINT_ENUM(HOST_CLASS_REQUEST);
		PRINT_ENUM(HOST_CHECK_CLASS);
		PRINT_ENUM(HOST_CLASS);
		PRINT_ENUM(HOST_SUSPENDED);
		PRINT_ENUM(HOST_ABORT_STATE);
		PRINT_ENUM(ENUM_IDLE);
		PRINT_ENUM(ENUM_GET_FULL_DEV_DESC);
		PRINT_ENUM(ENUM_SET_ADDR);
		PRINT_ENUM(ENUM_GET_CFG_DESC);
		PRINT_ENUM(ENUM_GET_FULL_CFG_DESC);
		PRINT_ENUM(ENUM_GET_MFC_STRING_DESC);
		PRINT_ENUM(ENUM_GET_PRODUCT_STRING_DESC);
		PRINT_ENUM(ENUM_GET_SERIALNUM_STRING_DESC);
		PRINT_ENUM(CTRL_IDLE);
		PRINT_ENUM(CTRL_SETUP);
		PRINT_ENUM(CTRL_SETUP_WAIT);
		PRINT_ENUM(CTRL_DATA_IN);
		PRINT_ENUM(CTRL_DATA_IN_WAIT);
		PRINT_ENUM(CTRL_DATA_OUT);
		PRINT_ENUM(CTRL_DATA_OUT_WAIT);
		PRINT_ENUM(CTRL_STATUS_IN);
		PRINT_ENUM(CTRL_STATUS_IN_WAIT);
		PRINT_ENUM(CTRL_STATUS_OUT);
		PRINT_ENUM(CTRL_STATUS_OUT_WAIT);
		PRINT_ENUM(CTRL_ERROR);
		PRINT_ENUM(CTRL_STALLED);
		PRINT_ENUM(CTRL_COMPLETE);
		PRINT_ENUM(CMD_IDLE);
		PRINT_ENUM(CMD_SEND);
		PRINT_ENUM(CMD_WAIT);
	}
	return "UNKNOWN";
}
#define FUCTION_NAME(ENUM) ENUM##_Function
#define FUNCTION_DEFINE(ENUM) \
		USBH_StateStatusTypeDef FUCTION_NAME(ENUM)(USBH_HandleTypeDef* phost) {


FUNCTION_DEFINE(HOST_IDLE)
	if (phost->device.is_connected)
	{
	 /* Wait for 200 ms after connection */
	 USBH_Delay(200);
	 USBH_LL_ResetPort(phost);
#if (USBH_USE_OS == 1)
	 o	sMessagePut ( phost->os_event, USBH_PORT_EVENT, 0);
#endif
	 phost->gState = HOST_DEV_WAIT_FOR_ATTACHMENT;
	}
	return USBH_STATE_OK;
}
FUNCTION_DEFINE(HOST_DEV_WAIT_FOR_ATTACHMENT)
	return USBH_STATE_OK;
}
FUNCTION_DEFINE(HOST_DEV_ATTACHED)
    USBH_UsrLog("USB Device Attached");
    /* Wait for 100 ms after Reset */
    USBH_Delay(100);

    phost->device.speed = USBH_LL_GetSpeed(phost);

    phost->Control.pipe_out = USBH_AllocPipe (phost, 0x00);
    phost->Control.pipe_in  = USBH_AllocPipe (phost, 0x80);


    /* Open Control pipes */
    USBH_OpenPipe (phost,
                   phost->Control.pipe_in,
				   0x80,
                   phost->device.address,
                   phost->device.speed,
                   USBH_EP_CONTROL,
                   phost->Control.pipe_size);

    /* Open Control pipes */
    USBH_OpenPipe (phost,
                   phost->Control.pipe_out,
                   0x00,
                   phost->device.address,
                   phost->device.speed,
                   USBH_EP_CONTROL,
                   phost->Control.pipe_size);

#if (USBH_USE_OS == 1)
    osMessagePut ( phost->os_event, USBH_PORT_EVENT, 0);
#endif

    phost->gState =HOST_ENUMERATION;
    return USBH_STATE_OK;
}
FUNCTION_DEFINE(HOST_ENUMERATION)
    /* Check for enumeration status */
    USBH_PushState(phost, HOST_ENUMERATION_FINISHED);
	phost->gState = ENUM_IDLE;
    return USBH_STATE_OK;
}
FUNCTION_DEFINE(HOST_ENUMERATION_FINISHED)
      USBH_UsrLog ("Enumeration done.");
      phost->device.current_interface = 0;
      if(phost->device.DevDesc->bNumConfigurations == 1)
      {
        USBH_UsrLog ("This device has only 1 configuration.");
        phost->gState = HOST_SET_CONFIGURATION;
      }
      else
      {
    	  phost->gState = HOST_INPUT;
      }
      return USBH_STATE_OK;
}
FUNCTION_DEFINE(HOST_INPUT)
  /* user callback for end of device basic enumeration */
  if(phost->pUser != NULL)
  {
	phost->pUser(phost, HOST_USER_SELECT_CONFIGURATION);

#if (USBH_USE_OS == 1)
	osMessagePut ( phost->os_event, USBH_STATE_CHANGED_EVENT, 0);
#endif
	phost->gState = HOST_SET_CONFIGURATION;
  }
  return USBH_OK;
}
FUNCTION_DEFINE(HOST_SET_CONFIGURATION)
    /* set configuration */
    if (USBH_SetCfg(phost, phost->device.CfgDesc->bConfigurationValue) == USBH_OK)
    {
      	  USBH_UsrLog ("Default configuration set.");
      	phost->gState = HOST_CHECK_CLASS;
    }
    return  USBH_STATE_OK;
}
FUNCTION_DEFINE(HOST_CHECK_CLASS)
    if(phost->ClassNumber == 0){
      USBH_UsrLog ("No Class has been registered.");
    }
    else
    {
      phost->pActiveClass = NULL;

      for (uint8_t idx = 0; idx < USBH_MAX_NUM_SUPPORTED_CLASS ; idx ++)
      {
        if(phost->pClass[idx]->ClassCode == phost->device.CfgDesc->Itf_Desc[0].bInterfaceClass)
        {
          phost->pActiveClass = phost->pClass[idx];
        }
      }

      if(phost->pActiveClass != NULL)
      {
        if(phost->pActiveClass->Init(phost)== USBH_OK)
        {
          USBH_UsrLog ("%s class started.", phost->pActiveClass->Name);
          phost->pUser(phost, HOST_USER_CLASS_SELECTED);
          phost->gState =  HOST_CLASS_REQUEST;
          /* Inform user that a class has been activated */

        }
        else
          USBH_UsrLog ("Device not supporting %s class.", phost->pActiveClass->Name);
          phost->gState = HOST_ABORT_STATE;
      }
      else
        USBH_UsrLog ("No registered class for this device.");
      phost->gState = HOST_ABORT_STATE;
    }
    return USBH_STATE_OK;
}
FUNCTION_DEFINE(HOST_CLASS_REQUEST)
    /* process class standard control requests state machine */
    if(phost->pActiveClass != NULL)
    {
    	USBH_StatusTypeDef status = phost->pActiveClass->Requests(phost);
    	if(status == USBH_OK)  phost->gState =  HOST_CLASS;
    }
    else
    {
      USBH_ErrLog ("Invalid Class Driver.");
      phost->gState =  HOST_ABORT_STATE;
    }

#if (USBH_USE_OS == 1)
    osMessagePut ( phost->os_event, USBH_STATE_CHANGED_EVENT, 0);
#endif
    return USBH_STATE_OK;
}
FUNCTION_DEFINE(HOST_CLASS)
    /* process class state machine */
    if(phost->pActiveClass != NULL)
       phost->pActiveClass->BgndProcess(phost);
    return USBH_OK;
}
FUNCTION_DEFINE(HOST_DEV_DISCONNECTED)
	DeInitStateMachine(phost);

	/* Re-Initilaize Host for new Enumeration */
	if(phost->pActiveClass != NULL)
	{
	  phost->pActiveClass->DeInit(phost);
	  phost->pActiveClass = NULL;
	}
	return USBH_STATE_OK;
}
FUNCTION_DEFINE(HOST_ABORT_STATE)
	DeInitStateMachine(phost);

	/* Re-Initilaize Host for new Enumeration */
	if(phost->pActiveClass != NULL)
	{
	  phost->pActiveClass->DeInit(phost);
	  phost->pActiveClass = NULL;
	}
	return USBH_STATE_OK;
}
FUNCTION_DEFINE(ENUM_IDLE)
// is this REALLY nessary?
  /* Get Device Desc for only 1st 8 bytes : To get EP0 MaxPacketSize */
  if ( USBH_Get_DevDesc(phost, 8) == USBH_OK)
  {

    phost->Control.pipe_size = phost->device.DevDesc->bMaxPacketSize;



    /* modify control channels configuration for MaxPacket size */
    USBH_OpenPipe (phost,
                         phost->Control.pipe_in,
                         0x80,
                         phost->device.address,
                         phost->device.speed,
                         USBH_EP_CONTROL,
                         phost->Control.pipe_size);

    /* Open Control pipes */
    USBH_OpenPipe (phost,
                         phost->Control.pipe_out,
                         0x00,
                         phost->device.address,
                         phost->device.speed,
                         USBH_EP_CONTROL,
                         phost->Control.pipe_size);
    phost->gState = ENUM_GET_FULL_DEV_DESC;
  }
	return USBH_STATE_OK;
}
FUNCTION_DEFINE(ENUM_GET_FULL_DEV_DESC)
/* Get FULL Device Desc  */
  if ( USBH_Get_DevDesc(phost, USB_DEVICE_DESC_SIZE)== USBH_OK)
  {
    USBH_UsrLog("PID: %xh", phost->device.DevDesc->idProduct );
    USBH_UsrLog("VID: %xh", phost->device.DevDesc->idVendor );

    phost->gState = ENUM_SET_ADDR;

  }
	return USBH_STATE_OK;
}
FUNCTION_DEFINE(ENUM_SET_ADDR)
/* set address */
   if ( USBH_SetAddress(phost, USBH_DEVICE_ADDRESS) == USBH_OK)
   {
     USBH_Delay(2);
     phost->device.address = USBH_DEVICE_ADDRESS;

     /* user callback for device address assigned */
     USBH_UsrLog("Address (#%d) assigned.", phost->device.address);
     phost->gState = ENUM_GET_CFG_DESC;

     /* modify control channels to update device address */
     USBH_OpenPipe (phost,
                          phost->Control.pipe_in,
                          0x80,
                          phost->device.address,
                          phost->device.speed,
                          USBH_EP_CONTROL,
                          phost->Control.pipe_size);

     /* Open Control pipes */
     USBH_OpenPipe (phost,
                          phost->Control.pipe_out,
                          0x00,
                          phost->device.address,
                          phost->device.speed,
                          USBH_EP_CONTROL,
                          phost->Control.pipe_size);
   }
   return USBH_STATE_OK;
}
FUNCTION_DEFINE(ENUM_GET_CFG_DESC)
/* get standard configuration descriptor */
   if ( USBH_Get_CfgDesc(phost,
                         USB_CONFIGURATION_DESC_SIZE) == USBH_OK)
   {
     phost->gState = ENUM_GET_FULL_CFG_DESC;
   }
return USBH_STATE_OK;
}

FUNCTION_DEFINE(ENUM_GET_FULL_CFG_DESC)
/* get FULL config descriptor (config, interface, endpoints) */
  if (USBH_Get_CfgDesc(phost,
                       phost->device.CfgDesc->wTotalLength) == USBH_OK)
  {
    phost->gState = ENUM_GET_MFC_STRING_DESC;
  }
	return USBH_STATE_OK;
}
FUNCTION_DEFINE(ENUM_GET_MFC_STRING_DESC)
    USBH_DescStringCacheTypeDef* str;
    if (phost->device.DevDesc->iManufacturer != 0)
    { /* Check that Manufacturer String is available */

      if ( USBH_Get_StringDesc(phost,
                               phost->device.DevDesc->iManufacturer,
							   &str) == USBH_OK)
      {
        /* User callback for Manufacturing string */
        USBH_UsrLog("Manufacturer : %s",  str->strData);
        phost->gState = ENUM_GET_PRODUCT_STRING_DESC;

#if (USBH_USE_OS == 1)
    osMessagePut ( phost->os_event, USBH_STATE_CHANGED_EVENT, 0);
#endif
      }
    }
    else
    {
     USBH_UsrLog("Manufacturer : N/A");
     phost->gState = ENUM_GET_PRODUCT_STRING_DESC;
#if (USBH_USE_OS == 1)
    osMessagePut ( phost->os_event, USBH_STATE_CHANGED_EVENT, 0);
#endif
    }
return USBH_STATE_OK;
}
FUNCTION_DEFINE(ENUM_GET_PRODUCT_STRING_DESC)
    USBH_DescStringCacheTypeDef* str;
    if (phost->device.DevDesc->iProduct != 0)
    { /* Check that Product string is available */
      if ( USBH_Get_StringDesc(phost,
                               phost->device.DevDesc->iProduct,
							   &str) == USBH_OK)
      {
        /* User callback for Product string */
        USBH_UsrLog("Product : %s",  str->strData);
        phost->gState = ENUM_GET_SERIALNUM_STRING_DESC;
      }
    }
    else
    {
      USBH_UsrLog("Product : N/A");
      phost->gState = ENUM_GET_SERIALNUM_STRING_DESC;
#if (USBH_USE_OS == 1)
    osMessagePut ( phost->os_event, USBH_STATE_CHANGED_EVENT, 0);
#endif
    }
    return USBH_STATE_OK;
}
FUNCTION_DEFINE(ENUM_GET_SERIALNUM_STRING_DESC)
	  USBH_DescStringCacheTypeDef* str;
    if (phost->device.DevDesc->iSerialNumber != 0)
    { /* Check that Serial number string is available */
      if ( USBH_Get_StringDesc(phost,
                               phost->device.DevDesc->iSerialNumber,
							   &str) == USBH_OK)
      {
        /* User callback for Serial number string */
         USBH_UsrLog("Serial Number : %s",  str->strData);
      }
    }
    else
    {
      USBH_UsrLog("Serial Number : N/A");
#if (USBH_USE_OS == 1)
    osMessagePut ( phost->os_event, USBH_STATE_CHANGED_EVENT, 0);
#endif
    }
    return USBH_STATE_POP; // end of enum, return to main process
 }
typedef  struct {
	USBH_CallbackTypeDef Callback;
	HOST_StateTypeDef State;
	const char* StateString;
} USBH_StateCallBackTypeDef;

#define FUNTION_NONE(ENUM) { NULL, ENUM, #ENUM  }
#define FUNCTION_POINTER(ENUM) { FUCTION_NAME(ENUM), (ENUM), #ENUM }


const USBH_StateCallBackTypeDef ProcessTable[] = {
		FUNTION_NONE(HOST_INIT),
		FUNCTION_POINTER(HOST_IDLE),
		FUNCTION_POINTER(HOST_DEV_WAIT_FOR_ATTACHMENT),
		FUNCTION_POINTER(HOST_DEV_ATTACHED),
		FUNCTION_POINTER(HOST_DEV_DISCONNECTED),
		FUNTION_NONE(HOST_DETECT_DEVICE_SPEED),
		FUNCTION_POINTER(HOST_ENUMERATION),
		FUNCTION_POINTER(HOST_ENUMERATION_FINISHED),
		FUNCTION_POINTER(HOST_CLASS_REQUEST),
		FUNCTION_POINTER(HOST_INPUT),
		FUNCTION_POINTER(HOST_SET_CONFIGURATION),
		FUNCTION_POINTER(HOST_CHECK_CLASS),
		FUNCTION_POINTER(HOST_CLASS),
		FUNTION_NONE(HOST_SUSPENDED),
		FUNCTION_POINTER(HOST_ABORT_STATE),
		FUNCTION_POINTER(ENUM_IDLE) ,
		FUNCTION_POINTER(ENUM_GET_FULL_DEV_DESC),
		FUNCTION_POINTER(ENUM_SET_ADDR),
		FUNCTION_POINTER(ENUM_GET_CFG_DESC),
		FUNCTION_POINTER(ENUM_GET_FULL_CFG_DESC),
		FUNCTION_POINTER(ENUM_GET_MFC_STRING_DESC),
		FUNCTION_POINTER(ENUM_GET_PRODUCT_STRING_DESC),
		FUNCTION_POINTER(ENUM_GET_SERIALNUM_STRING_DESC),
};


/**
  * @brief  USBH_Process 
  *         Background process of the USB Core.
  * @param  phost: Host Handle
  * @retval USBH Status
  */
USBH_StatusTypeDef  USBH_Process(USBH_HandleTypeDef *phost)
{
	  // debug checking
  USBH_StateCallBackTypeDef* entry = &ProcessTable[phost->gState];
  if(entry->State != phost->gState){
	  USBH_ErrLog("ProcessTable has an invalid entry!  Expected '%' but found '%'!",
	  ENUM_TO_STRING(phost->gState),entry->StateString);
	  return USBH_FAIL;
  }
  if(entry->Callback == NULL){
	  USBH_ErrLog("ProcessTable has no callback for '%'", entry->StateString);
	  return USBH_FAIL;
  }

	phost->gPrevState = phost->gState;
	__IO USBH_StateStatusTypeDef status = entry->Callback(phost);
  switch(status) {
	  case USBH_STATE_OK:
	  case USBH_STATE_BUSY:
		  break; // normal stuff
	  case USBH_STATE_FAIL:
		  return USBH_FAIL;
		  break; //todo: abort the connection
	  case USBH_STATE_PUSH:
		  if(phost->StackSize == MAX_STATE_STACK ){
			  USBH_ErrLog("State Stck Maxed out!");
			  return USBH_FAIL;
		  }
		  USBH_PushState(phost, phost->gPrevState);
		  break;
	  case USBH_STATE_POP:
		  if(phost->StackSize == 0 ){
			  USBH_ErrLog("Poping an empty StateStack!");
			  return USBH_FAIL;
		  }
		  phost->gState = USBH_PopState(phost);
		  break;
  }
  if(phost->gPrevState != phost->gState){
	  printf("%s -> %s\r\n", ENUM_TO_STRING(phost->gPrevState), ENUM_TO_STRING(phost->gState));
  }
  return USBH_OK;
}



/**
  * @brief  USBH_LL_SetTimer 
  *         Set the initial Host Timer tick
  * @param  phost: Host Handle
  * @retval None
  */
void  USBH_LL_SetTimer  (USBH_HandleTypeDef *phost, uint32_t time)
{
  phost->Timer = time;
}
/**
  * @brief  USBH_LL_IncTimer 
  *         Increment Host Timer tick
  * @param  phost: Host Handle
  * @retval None
  */
void  USBH_LL_IncTimer  (USBH_HandleTypeDef *phost)
{
  phost->Timer ++;
  USBH_HandleSof(phost);
}

/**
  * @brief  USBH_HandleSof 
  *         Call SOF process
  * @param  phost: Host Handle
  * @retval None
  */
void  USBH_HandleSof  (USBH_HandleTypeDef *phost)
{
  if((phost->gState == HOST_CLASS)&&(phost->pActiveClass != NULL))
  {
    phost->pActiveClass->SOFProcess(phost);
  }
}
/**
  * @brief  USBH_LL_Connect 
  *         Handle USB Host connexion event
  * @param  phost: Host Handle
  * @retval USBH_Status
  */
USBH_StatusTypeDef  USBH_LL_Connect  (USBH_HandleTypeDef *phost)
{
	switch(phost->gState){
	case HOST_IDLE:
		phost->device.is_connected = 1;
		if(phost->pUser != NULL) phost->pUser(phost, HOST_USER_CONNECTION);
		break;
	case HOST_DEV_WAIT_FOR_ATTACHMENT:
		phost->gState = HOST_DEV_ATTACHED;
		break;
	}
#if (USBH_USE_OS == 1)
  osMessagePut ( phost->os_event, USBH_PORT_EVENT, 0);
#endif 
  
  return USBH_OK;
}

/**
  * @brief  USBH_LL_Disconnect 
  *         Handle USB Host disconnection event
  * @param  phost: Host Handle
  * @retval USBH_Status
  */
USBH_StatusTypeDef  USBH_LL_Disconnect  (USBH_HandleTypeDef *phost)
{
  /*Stop Host */ 
  USBH_LL_Stop(phost);  
  
  /* FRee Control Pipes */
  USBH_FreePipe  (phost, phost->Control.pipe_in);
  USBH_FreePipe  (phost, phost->Control.pipe_out);  
   
  phost->device.is_connected = 0; 
   
  if(phost->pUser != NULL)
    phost->pUser(phost, HOST_USER_DISCONNECTION);
  USBH_UsrLog("USB Device disconnected"); 
  
  /* Start the low level driver  */
  USBH_LL_Start(phost);
  
  phost->gState = HOST_DEV_DISCONNECTED;
  
#if (USBH_USE_OS == 1)
  osMessagePut ( phost->os_event, USBH_PORT_EVENT, 0);
#endif 
  
  return USBH_OK;
}


#if (USBH_USE_OS == 1)  
/**
  * @brief  USB Host Thread task
  * @param  pvParameters not used
  * @retval None
  */
static void USBH_Process_OS(void const * argument)
{
  osEvent event;
  
  for(;;)
  {
    event = osMessageGet(((USBH_HandleTypeDef *)argument)->os_event, osWaitForever );
    
    if( event.status == osEventMessage )
    {
      USBH_Process((USBH_HandleTypeDef *)argument);
    }
   }
}

/**
* @brief  USBH_LL_NotifyURBChange 
*         Notify URB state Change
* @param  phost: Host handle
* @retval USBH Status
*/
USBH_StatusTypeDef  USBH_LL_NotifyURBChange (USBH_HandleTypeDef *phost)
{
  osMessagePut ( phost->os_event, USBH_URB_EVENT, 0);
  return USBH_OK;
}
#endif  
/**
  * @}
  */ 

/**
  * @}
  */ 

/**
  * @}
  */

/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
