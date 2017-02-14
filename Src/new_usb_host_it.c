#include "main.h"
#include "usb_host_new.h"
//#define DEBUG_USB_HOST_IN_IRQHandler
//#define DEBUG_USB_HOST_OUT_IRQHandler
#define USBH_DEBUG_LEVEL 4
/* DEBUG macros */
#if (USBH_DEBUG_LEVEL > 0)
#define USBH_UsrLog(...)   PrintDebugMessage(LOG_USER, __VA_ARGS__);
#else
#define USBH_UsrLog(...)
#endif
#if (USBH_DEBUG_LEVEL > 1)

#define USBH_ErrLog(...)   PrintDebugMessage(LOG_ERROR, __VA_ARGS__);
#else
#define USBH_ErrLog(...)
#endif
#if (USBH_DEBUG_LEVEL > 2)

#define USBH_WarnLog(...)   PrintDebugMessage(LOG_WARN, __VA_ARGS__);
#else
#define USBH_WarnLog(...)
#endif
#if (USBH_DEBUG_LEVEL > 3)
#define USBH_DbgLog(...)    PrintDebugMessage(LOG_DEBUG, __VA_ARGS__);
#else
#define USBH_DbgLog(...)
#endif

#define USB_IS_FS

#ifdef USB_IS_FS
#define USB (USB_OTG_FS)
#undef DMA_ENABLED
#else
#define USB (USB_OTG_HS)
#define DMA_ENABLED 1
#endif

// why this? cause I hate thoes damn warnings that eclipse gives you:P
#ifdef __weak
#undef __weak
#define __weak __attribute__((weak))
#endif

typedef enum {
	HOST_PORT_DISCONNECTED=0,
	HOST_PORT_CONNECTED,
	HOST_PORT_WAIT_FOR_ATTACHMENT,
	HOST_PORT_DISCONNECTING_PORT_OFF,
	HOST_PORT_DISCONNECTING_PORT_ON,
	HOST_PORT_IDLE,		// no data going in or out
	HOST_PORT_ACTIVE,	// waiting for packet or finish trasmit
} PORT_StateTypeDef;


typedef enum {
  PIPE_IDLE=0,
  PIPE_XFRC,  // grovy and done
  PIPE_ACK,
  PIPE_HALTED,
  PIPE_NAK,
  PIPE_NYET,
  PIPE_STALL,
  PIPE_XACTERR,
  PIPE_BBLERR,
  PIPE_DATATGLERR,
  PIPE_ERROR,		// frame error, sequance, something I did basicly
} e_USB_HOST_PipeState;


struct __t_pipe_state;
typedef bool (*PipeCallback)(struct __t_pipe_state *);

typedef struct __t_pipe_state {
	volatile e_USB_HOST_PipeState state;
	uint8_t ch_num;
	uint8_t ep_speed;
	size_t data_length;
	size_t xfer_length; // raw data recived or sent
	uint8_t err_cnt;
	uint8_t* data; // make sure we are 32 bit allligned
	PipeCallback TransferComplete;
	PipeCallback TransferError;
	PipeCallback TransferIdle;
	void* pData;
} t_pipe_state;


static t_pipe_state pipe_state[16];
static USB_OTG_CfgTypeDef usb_settings;
volatile uint32_t usb_host_sof_timer=0;
static uint8_t port_speed;
static PORT_StateTypeDef port_state = HOST_PORT_DISCONNECTED;
volatile uint32_t  irq_count=0;

static void ResetPipeStates() {
	memset(pipe_state,0,sizeof(t_pipe_state)*16);
	for(uint8_t i=0;i < 16;i++){
		pipe_state[i].ep_speed = port_speed;
	}
}

#define USBH_HCTSIZ_XFRSIZ(VALUE) ( (VALUE)          & USB_OTG_HCTSIZ_XFRSIZ)
#define USBH_HCTSIZ_PKTCNT(VALUE) ( ((VALUE) << 19U) & USB_OTG_HCTSIZ_PKTCNT)
#define HCTSIZ_GET_PID(REG)   	 (((REG)   & USB_OTG_HCTSIZ_DPID) >>   29U)
#define HCTSIZ_SET_PID(VALUE) 	 (((VALUE)<<   29U) & USB_OTG_HCTSIZ_DPID)
#define HCTSIZ_GET_XFRSIZ(REG)   (((REG)   & USB_OTG_HCTSIZ_XFRSIZ) >>  0U)
#define HCTSIZ_SET_XFRSIZ(VALUE) (((VALUE)<<  0U) & USB_OTG_HCTSIZ_XFRSIZ)
#define HCTSIZ_GET_PKTCNT(REG)   (((REG)   & USB_OTG_HCTSIZ_PKTCNT) >> 19U)
#define HCTSIZ_SET_PKTCNT(VALUE) (((VALUE)<< 19U) & USB_OTG_HCTSIZ_PKTCNT)


#define HCTSIZ_DATA2(__HCTSIZ__)  (((__HCTSIZ__) & USB_OTG_HCTSIZ_DPID) == 1)
#define HCTSIZ_DATA1(__HCTSIZ__)  (((__HCTSIZ__) & USB_OTG_HCTSIZ_DPID) == 2)
#define HCTSIZ_SETUP(__HCTSIZ__)  (((__HCTSIZ__) & USB_OTG_HCTSIZ_DPID) == 3)
#define HCTSIZ_TOGLE_PID_FS(__HCTSIZ__) \
	do {\
		if( ( (__HCTSIZ__)&USB_OTG_HCTSIZ_DPID_0) ==0){ /* if its not a SETUP or DATA2 */\
			if(((__HCTSIZ__)&USB_OTG_HCTSIZ_DPID_1)) /* if its a DATA1 */\
				(__HCTSIZ__) &= ~USB_OTG_HCTSIZ_DPID_1;/* set to DATA0 */\
			else\
				(__HCTSIZ__) |= USB_OTG_HCTSIZ_DPID_1;/* else  DATA1 */\
		}} while(0)


#define HCCHAR_SET_DAD(VALUE)   		  (((VALUE) << 22U) & USB_OTG_HCCHAR_DAD)
#define HCCHAR_GET_DAD(VALUE)             (((VALUE) & USB_OTG_HCCHAR_DAD) >> 22U)
#define HCCHAR_SET_MPSIZ(VALUE)   	      (((VALUE) << 0U) & USB_OTG_HCCHAR_MPSIZ)
#define HCCHAR_GET_MPSIZ(VALUE)           (((VALUE) & USB_OTG_HCCHAR_MPSIZ) >> 0U)
#define HCCHAR_SET_EPTYP(VALUE)           (((VALUE) << 18U) & USB_OTG_HCCHAR_EPTYP)
#define HCCHAR_GET_EPTYP(VALUE)           (((VALUE) & USB_OTG_HCCHAR_EPTYP) >> 18U)
#define HCCHAR_SET_EPNUM(VALUE)   		  (((VALUE) << 11U) & USB_OTG_HCCHAR_EPNUM)
#define HCCHAR_GET_EPNUM(VALUE)           (((VALUE) & USB_OTG_HCCHAR_EPNUM) >> 11U)
#define HCCHAR_SET_EPDIR(VALUE)   		  ((VALUE) ? USB_OTG_HCCHAR_EPDIR : 0)
#define HCCHAR_GET_EPDIR(VALUE)           (((VALUE) & USB_OTG_HCCHAR_EPDIR) ? 1 : 0)


#define HCCHAR_GET_PERIODIC_TYPE(REG) 				((REG) & USB_OTG_HCCHAR_EPTYP_0 ? 1: 0)
#define IS_EP_IN(EP) (((EP)->HCCAR &  USB_OTG_HCCHAR_EPDIR) !=0)

#define IS_ODD_FRAME ((USB_HOST(hhcd.Instance)->HFNUM & 0x01U) ? 0U : 1U)
#ifdef USB_DFIFO
#undef USB_DFIFO
#undef USB_HOST
#undef USB_HC

#define USB_HPRT0      *(__IO uint32_t *)((uint32_t)USB + USB_OTG_HOST_PORT_BASE)
#define USB_DFIFO(i)   *(__IO uint32_t *)((uint32_t)USB + USB_OTG_FIFO_BASE + (i) * USB_OTG_FIFO_SIZE)
#define USB_HOST       ((USB_OTG_HostTypeDef *)((uint32_t )USB + USB_OTG_HOST_BASE))
#define USB_HC(i)      ((USB_OTG_HostChannelTypeDef *)((uint32_t)USB + USB_OTG_HOST_CHANNEL_BASE + (i)*USB_OTG_HOST_CHANNEL_SIZE))
#define HAIT_INTERRUPT ((USB_HOST->HAINT) & 0xFFFFU)
#define USB_HOST_ENABLE               USB_EnableGlobalInt(USB)
#define USB_HOST_DISABLE              USB_DisableGlobalInt(USB)
#define USB_READ_INTERRUPTS     				((uint32_t)((USB)->GINTSTS & (USB)->GINTMSK))
#define USB_HOST_READ_PIPE_INTERRUPTS         ((USB_HOST->HAINT) & 0xFFFFU)
#define USB_HOST_GET_FLAG(__INTERRUPT__)      ((USB_READ_INTERRUPTS & (__INTERRUPT__)) == (__INTERRUPT__))
#define USB_HOST_CLEAR_FLAG(__INTERRUPT__)    ((USB->GINTSTS) = (__INTERRUPT__))
#define USB_HOST_IS_INVALID_INTERRUPT         (USB_READ_INTERRUPTS == 0U)

#define USB_HOST_EP_GET_FLAG(__CHANNEL__, __INTERRUPT__)      ((USB_HC(__CHANNEL__)->HCINT) & (__INTERRUPT__)) == (__INTERRUPT__))
#define USB_HOST_CLEAR_HC_INT(chnum, __INTERRUPT__)  (USB_HC(chnum)->HCINT = (__INTERRUPT__))
#define USB_HOST_MASK_HALT_HC_INT(chnum)             (USB_HC(chnum)->HCINTMSK &= ~USB_OTG_HCINTMSK_CHHM)
#define USB_HOST_UNMASK_HALT_HC_INT(chnum)           (USB_HC(chnum)->HCINTMSK |= USB_OTG_HCINTMSK_CHHM)
#define USB_HOST_MASK_ACK_HC_INT(chnum)              (USB_HC(chnum)->HCINTMSK &= ~USB_OTG_HCINTMSK_ACKM)
#define USB_HOST_UNMASK_ACK_HC_INT(chnum)            (USB_HC(chnum)->HCINTMSK |= USB_OTG_HCINTMSK_ACKM)
#define USB_IN_HOST_MODE 						     (((USB->GINTSTS ) & 0x1U)==0U)


#define USB_HOST_HALT_CHANNEL(CHANNEL) do {\
		USB_HC(chnum)->HCINTMSK |= USB_OTG_HCINTMSK_CHHM;\
		USB_HC(chnum)->HCCHAR |= USB_OTG_HCCHAR_CHDIS;\
		USB_HC(chnum)->HCCHAR |= USB_OTG_HCCHAR_CHENA; } while(0)
#define USB_HOST_ENABLE_CHANNEL(CHANNEL) do {\
		uint32_t tmpreg = USB_HC(CHANNEL)->HCCHAR;\
		tmpreg &= ~USB_OTG_HCCHAR_CHDIS; \
		tmpreg |= USB_OTG_HCCHAR_CHENA; \
		USB_HC(CHANNEL)->HCCHAR=tmpreg; } while(0)

#endif


void USB_HOST_SetPipeMaxPacketSize(uint8_t pipe, uint16_t mps) {
	//assert(pipe_state[pipe].state == HC_IDLE);
	USB_OTG_HostChannelTypeDef* hc = USB_HC(pipe);
	hc->HCCHAR = (hc->HCCHAR & ~(USB_OTG_HCCHAR_MPSIZ)) | ((mps << 0U) & USB_OTG_HCCHAR_MPSIZ);
}
void USB_HOST_SetPipeDevAddress(uint8_t pipe, uint8_t dev_address) {
	//assert(pipe_state[pipe].state == HC_IDLE);
	USB_OTG_HostChannelTypeDef* hc = USB_HC(pipe);
	hc->HCCHAR = (hc->HCCHAR & ~(USB_OTG_HCCHAR_DAD)) | ((dev_address << 22U) & USB_OTG_HCCHAR_DAD);

}
void USB_HOST_SetPipeSpeed(uint8_t pipe,  uint8_t low_speed) {
	USB_OTG_HostChannelTypeDef* hc = USB_HC(pipe);
	if(low_speed)
		hc->HCCHAR |= USB_OTG_HCCHAR_LSDEV;
	else
		hc->HCCHAR &= ~USB_OTG_HCCHAR_LSDEV;
}
static void USB_HOST_InitEndpointSetupRequest(uint8_t pipe) {
	pipe_state[pipe].state = HC_IDLE;
	USB_OTG_HostChannelTypeDef* hc = USB_HC(pipe);
	hc->HCINT = 0xFFFFFFFFU; // Clear old interrupt conditions for this host channel.
	hc->HCINTMSK = USB_OTG_HCINTMSK_XFRCM  |\
		               USB_OTG_HCINTMSK_STALLM |\
		               USB_OTG_HCINTMSK_TXERRM |\
		               USB_OTG_HCINTMSK_DTERRM |\
		               USB_OTG_HCINTMSK_AHBERR |\
		               USB_OTG_HCINTMSK_NAKM ;
	/* Enable the top level host channel interrupt. */
	USB_HOST->HAINTMSK |= (1 << pipe);

	/* Make sure host channel interrupts are enabled. */
	USB->GINTMSK |= USB_OTG_GINTMSK_HCIM;

	/* Program the HCCHAR register */
	hc->HCCHAR &= ~(USB_OTG_HCCHAR_EPNUM | USB_OTG_HCCHAR_EPDIR| USB_OTG_HCCHAR_EPTYP);
	hc->HCCHAR |= HCCHAR_SET_EPNUM(0) | HCCHAR_SET_EPDIR(0) | HCCHAR_GET_EPTYP(EP_TYPE_CTRL);
	hc->HCTSIZ = HCTSIZ_SET_XFRSIZ(8) | HCTSIZ_SET_PKTCNT(1) | HCTSIZ_SET_PID(HC_PID_SETUP);
}
void USB_HOST_InitEndpoint(uint8_t pipe, uint8_t ep_type, uint8_t ep_num, uint8_t ep_dir) {
	pipe_state[pipe].state = HC_IDLE;
	USB_OTG_HostChannelTypeDef* hc = USB_HC(pipe);

	/* Enable channel interrupts required for this transfer. */

	switch (ep_type)
	{
	case EP_TYPE_CTRL:
	case EP_TYPE_BULK:

	hc->HCINTMSK = USB_OTG_HCINTMSK_XFRCM  |\
	               USB_OTG_HCINTMSK_STALLM |\
	               USB_OTG_HCINTMSK_TXERRM |\
	               USB_OTG_HCINTMSK_DTERRM |\
	               USB_OTG_HCINTMSK_AHBERR |\
	               USB_OTG_HCINTMSK_NAKM ;

	if (ep_dir) hc->HCINTMSK |= USB_OTG_HCINTMSK_BBERRM;
#ifndef USB_IS_FS
	else
	{
		if(USBx != USB_OTG_FS)
			hc->HCINTMSK |= (USB_OTG_HCINTMSK_NYET | USB_OTG_HCINTMSK_ACKM);
	}
#endif
	break;
	case EP_TYPE_INTR:
	hc->HCCHAR |= USB_OTG_HCCHAR_ODDFRM;
	hc->HCINTMSK = USB_OTG_HCINTMSK_XFRCM  |\
	               USB_OTG_HCINTMSK_STALLM |\
	               USB_OTG_HCINTMSK_TXERRM |\
	               USB_OTG_HCINTMSK_DTERRM |\
	               USB_OTG_HCINTMSK_NAKM   |\
	               USB_OTG_HCINTMSK_AHBERR |\
	               USB_OTG_HCINTMSK_FRMORM ;
	if (ep_dir) hc->HCINTMSK |= USB_OTG_HCINTMSK_BBERRM;
	break;
	case EP_TYPE_ISOC:
	hc->HCINTMSK = USB_OTG_HCINTMSK_XFRCM  |\
	               USB_OTG_HCINTMSK_ACKM   |\
	               USB_OTG_HCINTMSK_AHBERR |\
	               USB_OTG_HCINTMSK_FRMORM ;

	if (ep_dir) hc->HCINTMSK |= (USB_OTG_HCINTMSK_TXERRM | USB_OTG_HCINTMSK_BBERRM);
	break;
	}

	/* Enable the top level host channel interrupt. */
	USB_HOST->HAINTMSK |= (1 << pipe);

	/* Make sure host channel interrupts are enabled. */
	USB->GINTMSK |= USB_OTG_GINTMSK_HCIM;

	/* Program the HCCHAR register */
	hc->HCCHAR &= ~(USB_OTG_HCCHAR_EPNUM | USB_OTG_HCCHAR_EPDIR| USB_OTG_HCCHAR_EPTYP);
	hc->HCCHAR |= HCCHAR_SET_EPNUM(ep_num) | HCCHAR_SET_EPDIR(ep_dir) | HCCHAR_SET_EPTYP(ep_type);
	USBH_DbgLog("USB_HOST_InitEndpoint");
	USB_HOST_PrintPipe(pipe);
}


/**
  * @brief Read all host channel interrupts status
  * @param  USBx : Selected device
  * @retval HAL state
  */
void USB_HOST_IRQHandler();
static void USB_HOST_Port_IRQHandler();

static void USB_HOST_RXQLVL_IRQHandler();

__weak void USB_HOST_NotifyURBChange_Callback(uint8_t chan_num, USB_OTG_URBStateTypeDef state) {
	(void)chan_num;
	(void)state;
}
__weak void USB_HOST_SOF_Callback(uint32_t time) {
	(void)time;
}
__weak void USB_HOST_Disconnect() {

}
__weak void USB_HOST_Connect() {

}


__weak void USB_HOST_SetVBUS(bool value) { // needs to be setup in conf but here for the 469i discovery
	  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, value ? GPIO_PIN_SET: GPIO_PIN_RESET);
	  // HAL_Delay(200); // don't put this dealy in here
}
void OTG_FS_IRQHandler(void)
{
	USB_HOST_IRQHandler ();
}
void OTG_FS_WKUP_IRQHandler(){
	USB_HOST_IRQHandler();
}



// state machine used just for making solid connections

void USB_HOST_Start()
{
  USB_HOST_ENABLE;
  USB_HOST_SetVBUS(true);
}

/**
  * @brief  Stop the host driver.
  * @param  hhcd: HCD handle
  * @retval HAL status
  */

void USB_HOST_Stop()
{
  USB_StopHost(USB);
}

void USB_HOST_ResetPort() {
	USB_ResetPort(USB);
}


void HAL_HCD_MspInit(HCD_HandleTypeDef *hhcd); // make this better for latter


void USB_HOST_Init()
{
	  port_state = HOST_PORT_DISCONNECTED;
#ifdef USB_IS_FS
	usb_settings.Host_channels = 11;
	usb_settings.dma_enable = DISABLE; // nno dma on full speed
	usb_settings.speed = HCD_SPEED_FULL;
#define USB (USB_OTG_FS)
#undef DMA_ENABLED
#else
#define USB (USB_OTG_HS)
#define DMA_ENABLED 1
#endif
	usb_settings.low_power_enable = 0;
	usb_settings.phy_itface = HCD_PHY_EMBEDDED;
	usb_settings.Sof_enable = DISABLE;//0;
	usb_settings.speed = HCD_SPEED_FULL;
	usb_settings.vbus_sensing_enable = 0;
	usb_settings.lpm_enable = 0;

	HAL_HCD_MspInit(NULL); // dosn't use nill here yet
	USB_HOST_DISABLE;

	 /* Init the Core (common init.) */
	USB_CoreInit(USB, usb_settings);
	  /* Force Host Mode*/
	  USB_SetCurrentMode(USB , USB_OTG_HOST_MODE);
	  /* Init Host */
	  USB_HostInit(USB, usb_settings);
	//InitUSBTimer(); // init the process timer

	  usb_host_sof_timer = USB_GetCurrentFrame(USB);
	  ResetPipeStates();

  USB_HOST_Stop();
  USB_HOST_Start();

}


static void _USB_HOST_Disconnect() {
	port_state = HOST_PORT_DISCONNECTING_PORT_OFF;
	USB_HOST_Stop();
	USB_HOST_SetVBUS(false);
	HAL_Delay(200);// reset the port in 200 ms
	USB_HOST_Disconnect();
	USB_HOST_Start();
	USB_HOST_SetVBUS(true);
	port_state = HOST_PORT_DISCONNECTING_PORT_ON;
	HAL_Delay(200);// reset the port in 200 ms
	port_state = HOST_PORT_DISCONNECTED;
}
uint32_t USB_HOST_GetHostSpeed() {
	  __IO uint32_t hprt0 = *(__IO uint32_t *)((uint32_t)USB + USB_OTG_HOST_PORT_BASE);
	  return ((hprt0 & USB_OTG_HPRT_PSPD) >> 17U);
}

static void _USB_HOST_Connect(){
	switch(port_state) {
	case HOST_PORT_DISCONNECTED:
		USB_HOST_ResetPort();
		HAL_Delay(200);
		USBH_DbgLog("HOST_PORT_WAIT_FOR_ATTACHMENT");
		port_state = HOST_PORT_WAIT_FOR_ATTACHMENT;
		break;
	case HOST_PORT_WAIT_FOR_ATTACHMENT:
		port_state = HOST_PORT_CONNECTED;
		port_speed = USB_GetHostSpeed(USB);
		ResetPipeStates();
		USBH_DbgLog("HOST_PORT_CONNECTED");
		USB_HOST_Connect(); // callback the connection
		break;
	default: break;
	}
}



/**
  * @brief  Handle HCD interrupt request.
  * @param  hhcd: HCD handle
  * @retval None
  */


#define CHECK_THEN_CLEAR_FLAG(__INTERRUPT__) \
		do { if(USB_HOST_GET_FLAG(__INTERRUPT__)) USB_HOST_CLEAR_FLAG(__INTERRUPT__); } while(0)

#define HCINT_IS_FLAG_SET(FLAG) (hcint & (FLAG))
#define ERROR_CALLBACK(DIR, FLAG, STATE) \
		else if(HCINT_IS_FLAG_SET((FLAG))) { \
			USBH_DbgLog("%i - " #DIR "(%i) " #FLAG, irq_count, chnum); \
			 ps->err_cnt++; \
			ps->state = (STATE); \
			if(!ps->TransferError && ps->TransferError(ps))\
				 USB_HOST_HALT_CHANNEL(chnum);\
			USB_HOST_HALT_CHANNEL(chnum);  }
#define ERROR_CALLBACK_NAK(DIR, FLAG, STATE) \
		else if(HCINT_IS_FLAG_SET((FLAG))) { \
			USBH_DbgLog("%i - " #DIR "(%i) " #FLAG, irq_count, chnum); \
			 ps->err_cnt++; \
			ps->state = (STATE); \
			USB_HOST_CLEAR_HC_INT(chnum,USB_OTG_HCINT_NAK);\
			if(!ps->TransferError || ps->TransferError(ps))\
				 USB_HOST_HALT_CHANNEL(chnum);\
			USB_HOST_HALT_CHANNEL(chnum);  }
// handles all had errors, that may or maynot be myfault
static void USB_HOST_INOUT_Process (t_pipe_state* ps){
	uint8_t chnum = ps->ch_num;
	USB_OTG_HostChannelTypeDef* hc = USB_HC(chnum);
	 uint32_t hcint = hc->HCINT;
	if(HCINT_IS_FLAG_SET(USB_OTG_HCINT_ACK)){
	#ifdef DEBUG_USB_HOST_IN_IRQHandler
		  USBH_DbgLog("%i - INOUT(%i) USB_OTG_HCINT_ACK",irq_count, chnum);
	#endif
		  USB_HOST_CLEAR_HC_INT(chnum, USB_OTG_HCINT_ACK);
	}
	ERROR_CALLBACK(INOUT, USB_OTG_HCINT_AHBERR, PIPE_ERROR)
	ERROR_CALLBACK(INOUT, USB_OTG_HCINT_FRMOR, PIPE_ERROR)
	ERROR_CALLBACK(INOUT, USB_OTG_HCINT_TXERR, PIPE_ERROR)
	ERROR_CALLBACK_NAK(INOUT, USB_OTG_HCINT_DTERR, PIPE_ERROR)
	ERROR_CALLBACK_NAK(INOUT, USB_OTG_HCINT_STALL, PIPE_STALL)
	ERROR_CALLBACK(INOUT, USB_OTG_HCINT_NAK, PIPE_NAK)

	if(HCINT_IS_FLAG_SET(USB_OTG_HCINT_XFRC)) { // trasfer complete
		 if(!ps->TransferComplete || ps->TransferComplete(ps)) USB_HOST_HALT_CHANNEL(chnum);
		  ps->state = PIPE_XFRC;
		  USB_HOST_CLEAR_HC_INT(chnum, USB_OTG_HCINT_XFRC);
	} else if(HCINT_IS_FLAG_SET(USB_OTG_HCINT_CHH)){  // halt
		ps->state = PIPE_IDLE;
		if(ps->TransferIdle) ps->TransferIdle(ps); // we trasfer the last state
		ps->err_cnt=0;
		USB_HOST_CLEAR_HC_INT(chnum, USB_OTG_HCINT_CHH);
	}
}

void USB_HOST_IRQHandler()
{

  /* Ensure that we are in device mode */
	 if (USB_GetMode(USB) == USB_OTG_MODE_HOST)
 // if (USB_IN_HOST_MODE)
  {
		 ++irq_count;
    if(USB_HOST_IS_INVALID_INTERRUPT) return; // Avoid spurious interrupt

    CHECK_THEN_CLEAR_FLAG(USB_OTG_GINTSTS_PXFR_INCOMPISOOUT); // Incorrect mode, acknowledge the interrupt
    CHECK_THEN_CLEAR_FLAG(USB_OTG_GINTSTS_IISOIXFR); // Incorrect mode, acknowledge the interrupt
    CHECK_THEN_CLEAR_FLAG(USB_OTG_GINTSTS_PTXFE); // Incorrect mode, acknowledge the interrupt
    CHECK_THEN_CLEAR_FLAG(USB_OTG_GINTSTS_MMIS); // Incorrect mode, acknowledge the interrupt

    /* Handle Host Disconnect Interrupts */
    if(USB_HOST_GET_FLAG(USB_OTG_GINTSTS_DISCINT))
    {
      /* Cleanup HPRT */
    	USB_HPRT0 &= ~(USB_OTG_HPRT_PENA | USB_OTG_HPRT_PCDET |\
    				   USB_OTG_HPRT_PENCHNG | USB_OTG_HPRT_POCCHNG );
      /* Handle Host Port Interrupts */
      _USB_HOST_Disconnect();
      USB_InitFSLSPClkSel(USB ,HCFG_48_MHZ);
      USB_HOST_CLEAR_FLAG(USB_OTG_GINTSTS_DISCINT);
    }

    /* Handle Host Port Interrupts */
    if(USB_HOST_GET_FLAG(USB_OTG_GINTSTS_HPRTINT)) USB_HOST_Port_IRQHandler();

    /* Handle Host SOF Interrupts */
    if(USB_HOST_GET_FLAG(USB_OTG_GINTSTS_SOF))
    {
    	usb_host_sof_timer++;
    	USB_HOST_SOF_Callback(usb_host_sof_timer);
    	USB_HOST_CLEAR_FLAG(USB_OTG_GINTSTS_SOF);
    }

    /* Handle Host channel Interrupts */
    if(USB_HOST_GET_FLAG(USB_OTG_GINTSTS_HCINT))
    {
      uint32_t interrupt = USB_HOST_READ_PIPE_INTERRUPTS;
#ifdef USB_IS_FS
     for (uint8_t i = 0U; i < 11; i++)
#else
     for (uint8_t i = 0U; i < 16; i++)
#endif
      {

        if (interrupt & (1U << i))
        {
        	t_pipe_state* ps = &pipe_state[i];
        	ps->ch_num = i; // not needed, sanity
        	USB_HOST_INOUT_Process(ps);
        //	assert(ps->Callback);
        	//ps->Callback(ps);
        }
      }
      USB_HOST_CLEAR_FLAG(USB_OTG_GINTSTS_HCINT);
    }

    /* Handle Rx Queue Level Interrupts */
    if(USB_HOST_GET_FLAG(USB_OTG_GINTSTS_RXFLVL))
    {
      USB_MASK_INTERRUPT(USB, USB_OTG_GINTSTS_RXFLVL);
      USB_HOST_RXQLVL_IRQHandler ();
      USB_UNMASK_INTERRUPT(USB, USB_OTG_GINTSTS_RXFLVL);
    }
  }
}





#define GRXSTSP_GET_EPNUM(REG)   (((REG)& USB_OTG_GRXSTSP_EPNUM) >> 0U)
#define GRXSTSP_GET_PKTSTS(REG)   (((REG)& USB_OTG_GRXSTSP_PKTSTS) >> 17U)
#define GRXSTSP_GET_BCNT(REG)   (((REG)& USB_OTG_GRXSTSP_BCNT) >> 4U)
#define GRXSTSP_DPID(REG)       (((REG)& USB_OTG_GRXSTSP_DPID) >> 15U)
static void USB_HOST_RXQLVL_IRQHandler(){
  uint32_t temp = USB->GRXSTSP;
  uint32_t channelnum = GRXSTSP_GET_EPNUM(temp);
  uint32_t pktsts = GRXSTSP_GET_PKTSTS(temp);
  uint32_t byte_count = GRXSTSP_GET_BCNT(temp);

  switch (pktsts)
  {
  case GRXSTS_PKTSTS_IN:
   /* Read the data into the host buffer. */
    if ((byte_count > 0U) && (pipe_state[channelnum].data != (void  *)0U))
    {
    	t_pipe_state* ps = &pipe_state[channelnum];
    	 while(byte_count){
    		 uint32_t in_word = USB_DFIFO(0U);
    		 if(byte_count < 4){
    			 uint8_t* in_byte = (uint8_t*)&in_word;
    			 while(byte_count-- && ps->xfer_length < ps->data_length)
    				 ps->data[ps->xfer_length++] = *in_byte++;
    			 byte_count=0;
    		 } else {
    			 if(ps->xfer_length < ps->data_length){
    				 *((__packed uint32_t *)(&ps->data[ps->xfer_length])) = in_word;
    				 ps->xfer_length+=4;
    			 }
    			 byte_count-=4;
    		 }
    	 }
      if((USB_HC(channelnum)->HCTSIZ & USB_OTG_HCTSIZ_PKTCNT) > 0U)
      {
    	  USBH_DbgLog("USB_HOST_RXQLVL_IRQHandler - More packets?");
        /* re-activate the channel when more packets are expected */
    	  USB_HOST_ENABLE_CHANNEL(channelnum);
    	  //pipe_state[channelnum].toggle_in ^= 1U;
      }
    }
    break;
  case GRXSTS_PKTSTS_DATA_TOGGLE_ERR:
  case GRXSTS_PKTSTS_IN_XFER_COMP:
  case GRXSTS_PKTSTS_CH_HALTED:
  default:
    break;
    // all these are handled by the main int
  }
}


static void USB_HOST_Port_IRQHandler()
{
  __IO uint32_t hprt0, hprt0_dup;

  /* Handle Host Port Interrupts */
  hprt0 = USB_HPRT0;
  hprt0_dup = USB_HPRT0;

  hprt0_dup &= ~(USB_OTG_HPRT_PENA | USB_OTG_HPRT_PCDET |\
                 USB_OTG_HPRT_PENCHNG | USB_OTG_HPRT_POCCHNG );

  /* Check whether Port Connect Detected */
  if((hprt0 & USB_OTG_HPRT_PCDET) == USB_OTG_HPRT_PCDET)
  {
    if((hprt0 & USB_OTG_HPRT_PCSTS) == USB_OTG_HPRT_PCSTS)
    {
      USB_MASK_INTERRUPT(USB, USB_OTG_GINTSTS_DISCINT);
      _USB_HOST_Connect();
    }
    hprt0_dup  |= USB_OTG_HPRT_PCDET;

  }

  /* Check whether Port Enable Changed */
  if((hprt0 & USB_OTG_HPRT_PENCHNG) == USB_OTG_HPRT_PENCHNG)
  {
    hprt0_dup |= USB_OTG_HPRT_PENCHNG;

    if((hprt0 & USB_OTG_HPRT_PENA) == USB_OTG_HPRT_PENA)
    {
      if(usb_settings.phy_itface  == USB_OTG_EMBEDDED_PHY)
      {
        if ((hprt0 & USB_OTG_HPRT_PSPD) == (HPRT0_PRTSPD_LOW_SPEED << 17U))
          USB_InitFSLSPClkSel(USB ,HCFG_6_MHZ );
        else
          USB_InitFSLSPClkSel(USB ,HCFG_48_MHZ );
      }
      else
      {
        if(usb_settings.speed == HCD_SPEED_FULL)
          USB_HOST->HFIR = (uint32_t)60000U;
      }
      _USB_HOST_Connect();
    }
    else
    {
      /* Clean up HPRT */
      USB_HPRT0 &= ~(USB_OTG_HPRT_PENA | USB_OTG_HPRT_PCDET |\
        USB_OTG_HPRT_PENCHNG | USB_OTG_HPRT_POCCHNG );

      USB_UNMASK_INTERRUPT(USB, USB_OTG_GINTSTS_DISCINT);
    }
  }

  /* Check for an over current */
  if((hprt0 & USB_OTG_HPRT_POCCHNG) == USB_OTG_HPRT_POCCHNG)
    hprt0_dup |= USB_OTG_HPRT_POCCHNG;

  /* Clear Port Interrupts */
  USB_HPRT0 = hprt0_dup;
}
typedef union {
	uint8_t b[4];
	uint32_t w;
} t_uint32_union;

void USB_HOST_WritePacket(uint8_t ch_num,  uint8_t* src, size_t length) {
	t_uint32_union word;
	while(length) {
		if(length < sizeof(uint32_t)) {
			word.b[0] = src[0];
			if(length > 1) word.b[1] = src[1];
			if(length > 2) word.b[2] = src[2];
			word.b[3] = 0;
			length=0;
		} else {
			word.w = *((__packed uint32_t *)src);
			src+= sizeof(uint32_t);
			length -= sizeof(uint32_t);
		}
		USB_DFIFO(ch_num) = word.w;
	}
}
#if 0

void USBH_ReadPacket(uint8_t ch_num, uint16_t byte_count, uint8_t* dest, size_t dest_length) {

}
#endif
void USB_HOST_PrintPipe(uint8_t ch_num){
	static const char* ep_type_to_string[] = { "CTRL" , "ISOC"  ,  "BULK" ,  "INTR" };
	static const char* pid_to_string[] = { "DATA0" , "DATA2"  ,  "DATA1" ,  "SETUP" };
	uint32_t hcchar = USB_HC(ch_num)->HCCHAR;
	uint32_t hctsiz = USB_HC(ch_num)->HCTSIZ;
	USBH_DbgLog("HCCHAR(%i) dad=%i epnum=%i eptype=%s epdir=%s speed=%s mps=%i",
			ch_num, HCCHAR_GET_DAD(hcchar), HCCHAR_GET_EPNUM(hcchar),
			ep_type_to_string[HCCHAR_GET_EPTYP(hcchar)], HCCHAR_GET_EPDIR(hcchar) ? "IN" : "OUT",
			(hcchar& USB_OTG_HCCHAR_LSDEV) ? "LOW" : "FULL", HCCHAR_GET_MPSIZ(hcchar)
					);
	USBH_DbgLog("HCTSIZ(%i) xfrsiz=%i pktcnt=%i pid=%s",
			ch_num, HCTSIZ_GET_XFRSIZ(hctsiz), HCTSIZ_GET_PKTCNT(hctsiz), pid_to_string[HCTSIZ_GET_PID(hctsiz)]);
}
inline void CtrlEnableChannel(uint8_t ch_num){
	if(USB_HOST->HFNUM & 0x01U)
		USB_HC(ch_num)->HCCHAR |= USB_OTG_HCCHAR_ODDFRM;
	else
		USB_HC(ch_num)->HCCHAR &= ~USB_OTG_HCCHAR_ODDFRM;
	USB_HOST_ENABLE_CHANNEL(ch_num);
}


bool CtrlPipeError(t_pipe_state * ps) {
	USBH_ControlRequestTypeDef* req = ps->pData;
	ps->TransferIdle = NULL;
	req->status = USB_HOST_ERROR;
	USBH_DbgLog("CtrlPipeError ch=%i i=%i",ps->ch_num, ps->state);
	USB_HOST_PrintPipe(ps->ch_num);
	return true; // just cancel?
}
bool PipeCompleteDefault(t_pipe_state * ps) {
	(void)ps;
	return true;
}
bool CtrlPipeComplete(t_pipe_state * ps) {
USBH_ControlRequestTypeDef* req = ps->pData;
	if(req->status == USB_HOST_WORKING) req->status  = USB_HOST_COMPLETE;
	ps->TransferIdle = NULL;
	return true;
}
inline uint32_t CtrlSetupDataTrasfer(t_pipe_state * ps, uint8_t dir, uint8_t pid, uint8_t byte_count){
	USB_OTG_HostChannelTypeDef* hc = USB_HC(ps->ch_num);
	uint16_t max_packet_size = HCCHAR_GET_MPSIZ(hc->HCCHAR);
	uint32_t num_packets = (byte_count + max_packet_size - 1U) / max_packet_size;
	if(dir){
		hc->HCINTMSK |= USB_OTG_HCINTMSK_BBERRM;
		hc->HCCHAR |= USB_OTG_HCCHAR_EPDIR;
	} else {
		hc->HCINTMSK &= ~USB_OTG_HCINTMSK_BBERRM;
		hc->HCCHAR &= ~USB_OTG_HCCHAR_EPDIR;
	}
	hc->HCTSIZ = HCTSIZ_SET_XFRSIZ(byte_count) | HCTSIZ_SET_PKTCNT(num_packets) | HCTSIZ_SET_PID(pid);
	return num_packets;
}

bool CtrlPipeSendStatus(t_pipe_state * ps) {
	USB_OTG_HostChannelTypeDef* hc = USB_HC(ps->ch_num);
	hc->HCINTMSK &= ~USB_OTG_HCINTMSK_BBERRM;
	hc->HCCHAR &= ~USB_OTG_HCCHAR_EPDIR;
	hc->HCTSIZ = HCTSIZ_SET_XFRSIZ(0) | HCTSIZ_SET_PKTCNT(1) | HCTSIZ_SET_PID(HC_PID_DATA1);
	ps->TransferIdle =CtrlPipeComplete;
	USB_HOST_ENABLE_CHANNEL(ps->ch_num);
	return false;
}
bool CtrlPipeRecvStatus(t_pipe_state * ps) {
	USB_OTG_HostChannelTypeDef* hc = USB_HC(ps->ch_num);
	hc->HCINTMSK |= USB_OTG_HCINTMSK_BBERRM;
	hc->HCCHAR |= USB_OTG_HCCHAR_EPDIR;
	hc->HCTSIZ = HCTSIZ_SET_XFRSIZ(0) | HCTSIZ_SET_PKTCNT(1) | HCTSIZ_SET_PID(HC_PID_DATA1);
	ps->TransferIdle = CtrlPipeComplete;
	USB_HOST_ENABLE_CHANNEL(ps->ch_num);
	return false;
}
void TrasferData(t_pipe_state * ps, uint8_t dir, uint8_t* data, uint32_t length) {
	USB_OTG_HostChannelTypeDef* hc = USB_HC(ps->ch_num);
	uint16_t max_packet_size = HCCHAR_GET_MPSIZ(ps->ch_num);
	uint32_t num_packets = (length + max_packet_size - 1U) / max_packet_size;
	assert(length < USB_OTG_HCTSIZ_XFRSIZ);
	hc->HCTSIZ = HCTSIZ_SET_XFRSIZ(length) | HCTSIZ_SET_PKTCNT(num_packets) | HCTSIZ_SET_PID(HC_PID_DATA1);
	ps->xfer_length = 0;
	ps->data_length = length;
	ps->data = data;
	if(dir){
		hc->HCINTMSK |= USB_OTG_HCINTMSK_BBERRM;
		hc->HCCHAR |= USB_OTG_HCCHAR_EPDIR;
	} else {
		hc->HCINTMSK &= ~USB_OTG_HCINTMSK_BBERRM;
		hc->HCCHAR &= ~USB_OTG_HCCHAR_EPDIR;
		USB_HOST_WritePacket(ps->ch_num,data, length);
	}
	USB_HOST_ENABLE_CHANNEL(ps->ch_num);
}
bool CtrlPipeSendData(t_pipe_state * ps) {
	USB_OTG_HostChannelTypeDef* hc = USB_HC(ps->ch_num);
	USBH_ControlRequestTypeDef* req = ps->pData;
	TrasferData(ps, req->Length,0, req->Data);
	return false;
}
bool CtrlPipeRecvData(t_pipe_state * ps) {
	USB_OTG_HostChannelTypeDef* hc = USB_HC(ps->ch_num);
	USBH_ControlRequestTypeDef* req = ps->pData;
	TrasferData(ps, req->Length,1, req->Data);
	return false;
}
bool CtrlPipeSendSetup(t_pipe_state * ps) {
	USB_OTG_HostChannelTypeDef* hc = USB_HC(ps->ch_num);
	USBH_ControlRequestTypeDef* req = ps->pData;
	hc->HCINTMSK &= ~USB_OTG_HCINTMSK_BBERRM;
	hc->HCCHAR &= ~USB_OTG_HCCHAR_EPDIR;
	hc->HCTSIZ = HCTSIZ_SET_XFRSIZ(8) | HCTSIZ_SET_PKTCNT(1) | HCTSIZ_SET_PID(HC_PID_SETUP);
	if(req->RequestType & USB_SETUP_DEVICE_TO_HOST){
		ps->TransferIdle = req->Length == 0 ? CtrlPipeSendStatus : CtrlPipeRecvData ;
	} else {
		ps->TransferIdle = req->Length == 0 ? CtrlPipeRecvStatus : CtrlPipeSendData ;
	}
	uint32_t * words = (uint32_t *)req;
	USB_DFIFO(ps->ch_num) = words[0];
	USB_DFIFO(ps->ch_num) = words[1];
	USB_HOST_ENABLE_CHANNEL(ps->ch_num);
	return false;
}
void USB_HOST_SubmitControlRequest(uint8_t ch_num, USBH_ControlRequestTypeDef* req){
	req->status = USB_HOST_WORKING;
	t_pipe_state* ps = &pipe_state[ch_num];
	USB_HOST_InitEndpoint(ch_num,EP_TYPE_CTRL,0,0);
	ps->TransferError = CtrlPipeError;
	ps->TransferComplete = CtrlPipeComplete;
	ps->pData = req;

	CtrlPipeSendSetup(ps);
}
#pragma GCC optimize ("O0")
void USB_SubmitTrasfer(uint8_t ch_num, uint8_t* data, size_t length) {
	USB_OTG_HostChannelTypeDef* hc = USB_HC(ch_num);
	uint32_t hcchar = hc->HCCHAR;
	uint16_t max_packet_size = HCCHAR_GET_MPSIZ(hcchar);
	t_pipe_state* ps = &pipe_state[ch_num];
  uint16_t num_packets = 0U;
 // uint16_t max_hc_pkt_count = 256U;
#if !defined(USB_IS_FS) && defined(DMA_ENABLED) && DMA_ENABLED != 0
  if((USBx != USB_OTG_FS) && (hc->speed == USB_OTG_SPEED_HIGH))
  {

    if((dma == 0U) && (hc->do_ping == 1U))
    {
      USB_DoPing(USBx, hc->ch_num);
      return HAL_OK;
    }
    else if(dma == 1U)
    {
      USBx_HC(hc->ch_num)->HCINTMSK &= ~(USB_OTG_HCINTMSK_NYET | USB_OTG_HCINTMSK_ACKM);
      hc->do_ping = 0U;
    }
  }
#endif
  /* Compute the expected number of packets associated to the transfer */
  if(data == NULL || length == 0) {
	  ps->data_length = 0;
	  ps->data = NULL;
	 num_packets = 1U;
  } else {
	  ps->data_length = length;
	  ps->data = data;
	  num_packets = (length + max_packet_size - 1U) / max_packet_size;
  }
  ps->xfer_length = 0;
  /* Initialize the HCTSIZn register */
  hc->HCTSIZ = HCTSIZ_SET_XFRSIZ(length) | HCTSIZ_SET_PKTCNT(num_packets) | HCTSIZ_SET_PID(HC_PID_DATA1);
#if 0
  uint32_t  hctsiz
  USB_HOST_PrintPipe(ch_num);
  switch(HCTSIZ_GET_PID(hc->HCTSIZ)){
  case HC_PID_DATA0: // DATA0
	  hctsiz |= HCTSIZ_SET_PID(HC_PID_DATA1); // Set to DATA 1
	  break;
  case HC_PID_DATA1: // DATA1
	  hctsiz |= HCTSIZ_SET_PID(HC_PID_DATA0); // Set to DATA 0
	  break;
  case HC_PID_SETUP: // SETUP,
 	  hctsiz |= HCTSIZ_SET_PID(HC_PID_SETUP);
 	  break;
#ifndef USB_IS_FS
  case 1: // DATA2
	  assert(0); // fuck me, how does this work?
	  break;
#else
  default:
	  USB_HOST_PrintPipe(ch_num);
	  assert(0); // fuck me, how does this work?
	  break; // f
#endif
  }
  hc->HCTSIZ = hctsiz;
#endif
#if !defined(USB_IS_FS) && defined(DMA_ENABLED) && DMA_ENABLED != 0
  * xfer_buff MUST be 32-bits aligned */
  {
	  uint32_t aligment_check = (uint32_t)data;
	  assert((aligment_check& 3) == 0);
	  USB_HC(hc->ch_num)->HCDMA = (uint32_t)data;
  }

#endif
  if((USB_HOST->HFNUM & 0x01U))
	  hc->HCCHAR |= USB_OTG_HCCHAR_ODDFRM;
  else
	  hc->HCCHAR &= ~USB_OTG_HCCHAR_ODDFRM;
  /* Set host channel enable */

#if defined(USB_IS_FS) || (defined(DMA_ENABLED) && DMA_ENABLED == 0)
  // if we are writing data, be sure to write it here
	if(HCCHAR_GET_EPDIR(hcchar)==0 && num_packets > 0)  {
		uint32_t len_words = (pipe_state[ch_num].xfer_length + 3U) / 4U;
		if(HCCHAR_GET_PERIODIC_TYPE(hc->HCCHAR)) { // if its periotic
			/* split the transfer */
			if(len_words > (USB_HOST->HPTXSTS & 0xFFFFU)) USB->GINTMSK |= USB_OTG_GINTMSK_PTXFEM;
		} else {
			/* need to process data in nptxfempty interrupt */
			if(len_words > (USB->HNPTXSTS & 0xFFFFU))USB->GINTMSK |= USB_OTG_GINTMSK_NPTXFEM;
		}
		USB_HOST_WritePacket(ch_num, data,length);
	}
#endif
	//ps->Callback = HCCHAR_GET_EPDIR(hc->HCCHAR) ? USB_HOST_IN_IRQHandler : USB_HOST_OUT_IRQHandler;
	USB_HOST_ENABLE_CHANNEL(ch_num);
}

