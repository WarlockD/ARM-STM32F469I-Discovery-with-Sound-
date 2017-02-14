#include "usb_host.h"
#include "usbh_desc.h"
#include "link_list.h"
#include "usart.h"

#include "usb_host_new.h"

#define USB_PORT USB_OTG_FS
//#define USB_HOST USB_OTG_FS
// timer functions

// only support 15 end points in high and only 11 in full ugh, REMEMBER THIS
HCD_HandleTypeDef hhcd;
static USBH_PortSpeedTypeDef port_speed;

#define USBH_HCTSIZ_XFRSIZ(VALUE) ( (VALUE)          & USB_OTG_HCTSIZ_XFRSIZ)
#define USBH_HCTSIZ_PKTCNT(VALUE) ( ((VALUE) << 19U) & USB_OTG_HCTSIZ_PKTCNT)
#define USBH_HCCHAR_EPNUM(VALUE)  ( ((VALUE) << 11U) & USB_OTG_HCCHAR_EPNUM)
#define USBH_HCCHAR_TYPE(VALUE)   ( ((VALUE) << 18U) & USB_OTG_HCCHAR_EPTYP)
#define USBH_HCCHAR_DEVICE_ADDRESS(VALUE)   		( ((VALUE) << 22U) & USB_OTG_HCCHAR_DAD)
#define USBH_HCCHAR_DEVICE_MAX_PACKET_SIZE(VALUE)   ( ((VALUE) <<  0U) & USB_OTG_HCCHAR_MPSIZ)
#define USBH_HCCHAR_DEVICE_EP_TYPE(VALUE)           ( ((VALUE) << 18U) & USB_OTG_HCCHAR_EPTYP)
#define USBH_HCCHAR_DEVICE_EP_NUM(VALUE)            ( ((VALUE) << 11U) & USB_OTG_HCCHAR_EPNUM)
#define HCCHAR_GET_DAD(REG)              			( ((REG) & USB_OTG_HCCHAR_DAD) >> 22U)
#define HCCHAR_GET_MPSIZ(REG)             			( ((REG) & USB_OTG_HCCHAR_MPSIZ) >> 0U)
#define HCCHAR_GET_EPTYP(REG)              			( ((REG) & USB_OTG_HCCHAR_EPTYP) >> 18U)
#define HCCHAR_GET_EPNUM(REG)                      ( ((REG) & USB_OTG_HCCHAR_EPNUM) >> 11U)
#define HCCHAR_GET_EPDIR(REG)                      ( ((REG) & USB_OTG_HCCHAR_EPDIR) ? 1 : 0)
#define IS_EP_IN(EP) (((EP)->HCCAR &  USB_OTG_HCCHAR_EPDIR) !=0)
#define IS_ODD_FRAME ((USB_HOST(hhcd.Instance)->HFNUM & 0x01U) ? 0U : 1U)

void PrintHCCHAR(uint32_t hcchar){
	USBH_DbgLog("HCCHAR = DEV_ADDR(%i) | MPS(%i) | EP_NUM(%i) | EP_TYPE(%i) | EP_DIR(%i)",
			HCCHAR_GET_DAD(hcchar),
			HCCHAR_GET_MPSIZ(hcchar),
			HCCHAR_GET_EPNUM(hcchar),
			HCCHAR_GET_EPTYP(hcchar),
			HCCHAR_GET_EPDIR(hcchar)
	);
}
struct __PipeStatus ;
typedef struct {
	union {
		uint32_t* adata;
		uint8_t* bdata;
	};
	size_t length;
	size_t pkt_cnt;
	size_t pkt_remaining;
	size_t xfer_len;
} t_alligned_data;

typedef struct __PipeStatus {
	USBH_PipeHandleTypeDef handle;
	struct __PipeStatus* next;
	USBH_DeviceTypeDef* owner;
	USB_OTG_HostChannelTypeDef *hc;
	PipeCallback Callback;
	void* udata;
	t_alligned_data data;
	uint8_t chan_num;
	uint8_t do_ping;
} PipeStatus;

PipeStatus pipe_status_list[16];
PipeStatus* free_pipes=NULL;

static  PipeStatus* OpenPipe(USBH_DeviceTypeDef* device){
	assert(device);
	if(free_pipes==NULL) return NULL;
	PipeStatus* pipe = free_pipes;
	free_pipes = pipe->next;
	pipe->owner = device;
	pipe->Callback = NULL;
	pipe->udata = NULL;
	pipe->handle.hc_state = HC_IDLE;
	pipe->handle.status = USBH_PIPE_IDLE;
	pipe->handle.urb_state = USBH_URB_IDLE;
	pipe->next = NULL;
	  /* Enable the top level host channel interrupt. */
	USB_HOST(hhcd.Instance)->HAINTMSK |= (1 << pipe->chan_num);
	return pipe;
}

static void ClosePipe(PipeStatus* pipe) {
	assert(pipe);
	assert(pipe->owner);
	assert(pipe->handle.status == USBH_PIPE_IDLE);
	pipe->owner = NULL;
	pipe->Callback = NULL;
	pipe->udata = NULL;
	pipe->next = free_pipes;
	free_pipes = pipe;
	USB_HOST(hhcd.Instance)->HAINTMSK &= ~(1 << pipe->chan_num);
}
void USBH_SetPipeCallback(USBH_PipeHandleTypeDef* handle,PipeCallback callback, void*udata){
	assert(handle);
	PipeStatus* pipe = (PipeStatus*)handle;
	pipe->Callback = callback;
	pipe->udata = udata;
}



USBH_PipeHandleTypeDef* USBH_OpenPipe(USBH_DeviceTypeDef* device){
	PipeStatus* ihandle = OpenPipe(device);
	if(!ihandle) return NULL;
	USBH_ResetPipe((USBH_PipeHandleTypeDef*)&(ihandle->handle));
	/* Make sure host channel interrupts are enabled. */
	hhcd.Instance->GINTMSK |= USB_OTG_GINTMSK_HCIM;
	return (USBH_PipeHandleTypeDef*)(&(ihandle->handle));
}
void USBH_ClosePipe(USBH_PipeHandleTypeDef* handle){
	ClosePipe((PipeStatus*)handle);
}

void* USBH_malloc(size_t size, void** owner) {
	(void)owner;
	return malloc(size);
}
void USBH_free(void * ptr) { free(ptr); }
void USBH_memset(void* ptr, uint8_t value, size_t size) { memset(ptr,value,size); }
void USBH_memcpy(void* dst, const void* src, size_t size){ memcpy(dst,src,size); }


volatile uint32_t ticks_test;
#if 0
void TIMx_IRQHandler() {
	if(TIMx->SR & TIM_SR_UIF) {
		ticks_test = HAL_GetTick()-ticks_test;
		TIMx->SR &= ~TIM_SR_UIF; // clear it
		if(timer_callback) timer_callback(timer_data_callback);
		uart_print("Ticks %lu\r\n", ticks_test);
	}
}
#endif


static  inline void SetHostChannelEnable(USB_OTG_HostChannelTypeDef * hc){
	uint32_t tmpreg = hc->HCCHAR; /* Set host channel enable */
	tmpreg &= ~USB_OTG_HCCHAR_CHDIS;
	tmpreg |= USB_OTG_HCCHAR_CHENA;
	hc->HCCHAR = tmpreg;
}
static inline void USBH_DoPing(USB_OTG_HostChannelTypeDef * hc){
	hc->HCTSIZ = USBH_HCTSIZ_PKTCNT(1) | USB_OTG_HCTSIZ_DOPING;
	SetHostChannelEnable(hc);
}
USBH_URBStateTypeDef USBH_PipeStatus(uint8_t pipe) {
	return (USBH_URBStateTypeDef)HAL_HCD_HC_GetURBState(&hhcd,pipe);
}
#if 0

void USBH_WritePacket(USB_OTG_GlobalTypeDef *USBx, uint8_t *src, uint8_t ch_ep_num, uint16_t len, uint8_t dma)
{
  uint32_t count32b = 0U , i = 0U;

  if (dma == 0U)
  {
    count32b =  (len + 3U) / 4U;
    for (i = 0U; i < count32b; i++, src += 4U)
      USBx_DFIFO(ch_ep_num) = *((__packed uint32_t *)src);
    }
  }
}

#pragma GCC optimize ("O0")
HAL_StatusTypeDef USBH_StartTrasfer(USBH_DeviceTypeDef* dev, USBH_PipeHandleTypeDef* hpipe, uint8_t dma)
{
	PipeStatus* pipe = (PipeStatus*)hpipe;
	USB_OTG_HostChannelTypeDef * hc = pipe->hc;
  uint8_t  is_oddframe = 0U;
  uint16_t len_words = 0U;
  uint16_t num_packets = 0U;
  uint16_t max_hc_pkt_count = 256U;
  uint32_t tmpreg = 0U;
  uint32_t hccar = hc->HCCHAR;
  uint16_t max_packet_size =  hccar & USB_OTG_HCCHAR_MPSIZ;
\
  if((USB_HOST(hhcd.Instance) != USB_OTG_FS) && (pipe->owner->Speed == USB_OTG_SPEED_HIGH))
  {
    if((dma == 0U) && (pipe->do_ping == 1U))
    {
      USB_DoPing(hc);
      return HAL_OK;
    }
    else if(dma == 1U)
    {
      hc->HCINTMSK &= ~(USB_OTG_HCINTMSK_NYET | USB_OTG_HCINTMSK_ACKM);
      pipe->do_ping= 0U;
    }
  }
  // caculate the number of packets
  if (pipe->data.length  > 0U)
	  pipe->data.pkt_cnt = (pipe->data.length + max_packet_size - 1U) / max_packet_size;
  else
	  pipe->data.pkt_cnt = 1U;

  if (hccar &  USB_OTG_HCCHAR_EPDIR) // if incomming, record how many packets we need
	pipe->data.xfer_len = pipe->data.pkt_cnt * max_packet_size;
  else {
	if (pipe->data.pkt_cnt > max_hc_pkt_count){
	  num_packets = max_hc_pkt_count;
	  pipe->data.xfer_len = num_packets  * max_packet_size;
  	} else {
	  num_packets = pipe->data.pkt_cnt;
	  pipe->data.xfer_len  = pipe->data.length;
	}
  }
  // Initialize the HCTSIZn register
  hc->HCTSIZ = USBH_HCTSIZ_XFRSIZ(pipe->data.xfer_len) | USBH_HCTSIZ_PKTCNT(pipe->data.pkt_cnt) | USB_OTG_HCTSIZ_DPID;
  /* xfer_buff MUST be 32-bits aligned */
  if (dma) hc->HCDMA = (uint32_t)pipe->data.adata;

  if(IS_ODD_FRAME) hccar |= USB_OTG_HCCHAR_ODDFRM; else hccar &= ~USB_OTG_HCCHAR_ODDFRM;
  // Set host channel enable
  hccar &= ~USB_OTG_HCCHAR_CHDIS;
  hccar |= USB_OTG_HCCHAR_CHENA;
  hc->HCCHAR = hccar; // enable

  if (dma == 0U) /* Slave mode */
  {
	 // if trasmitting
    if((hccar &  USB_OTG_HCCHAR_EPDIR) && (pipe->data.length > 0U))
    {
    	if(hccar & USB_OTG_HCCHAR_EPTYP_0) {
    		// Periodic transfer
    		len_words = (pipe->data.xfer_len + 3U) / 4U;
			/* check if there is enough space in FIFO space */
			if(len_words > (USB_HOST(hhcd.Instance)->HPTXSTS & 0xFFFFU)) /* split the transfer */
			{
			  /* need to process data in ptxfempty interrupt */
				hhcd.Instance->GINTMSK |= USB_OTG_GINTMSK_PTXFEM;
			}
    	} else {
    		len_words = (pipe->data.xfer_len + 3U) / 4U;

			/* check if there is enough space in FIFO space */
			if(len_words > (hhcd.Instance->HNPTXSTS & 0xFFFFU))
			{
			  /* need to process data in nptxfempty interrupt */
				hhcd.Instance->GINTMSK |= USB_OTG_GINTMSK_NPTXFEM;
			}
    	}
      /* Write packet into the Tx FIFO. */
    //  USB_WritePacket(USBx, hc->xfer_buff, hc->ch_num, hc->xfer_len, 0);
    }
  }

  return HAL_OK;
}
#endif
/**
  * @brief  Returns the USB Host Speed from the Low Level Driver.
  * @param  phost: Host handle
  * @retval USBH Speeds
  */
USBH_PortSpeedTypeDef USBH_GetSpeed()
{
	USBH_PortSpeedTypeDef speed = USBH_SPEED_FULL;

  switch (HAL_HCD_GetCurrentSpeed(&hhcd))
  {
  case 0:
    speed = USBH_SPEED_HIGH;
    break;

  case 1:
    speed = USBH_SPEED_FULL;
    break;

  case 2:
    speed = USBH_SPEED_LOW;
    break;

  default:
    speed = USBH_SPEED_FULL;
    break;
  }
  return speed;
}

void DMAInit() {
	// more to this?
	//if (cfg.dma_enable == DISABLE)
	 // {
	 //   USBx->GINTMSK |= USB_OTG_GINTMSK_RXFLVLM;
	//  }
}


void USBH_PipeChangeDirection(USBH_PipeHandleTypeDef* handle, uint8_t direction) {
	PipeStatus* pipe = (PipeStatus*)handle;
	__IO USB_OTG_HostChannelTypeDef * hc = pipe->hc;
	hhcd.hc[pipe->chan_num].ep_is_in = direction;
	if(direction){
		hc->HCINTMSK |= USB_OTG_HCINTMSK_BBERRM;
		hc->HCCHAR |= USB_OTG_HCCHAR_EPDIR;
		if(hhcd.Instance != USB_OTG_FS)
			hc->HCINTMSK &= ~(USB_OTG_HCINTMSK_NYET | USB_OTG_HCINTMSK_ACKM);
	} else {
		hc->HCINTMSK &= ~USB_OTG_HCINTMSK_BBERRM;
		hc->HCCHAR &= ~USB_OTG_HCCHAR_EPDIR;
		if(hhcd.Instance != USB_OTG_FS)
			hc->HCINTMSK |= (USB_OTG_HCINTMSK_NYET | USB_OTG_HCINTMSK_ACKM);
	}
}



void USBH_ConfigurePipeControl(USBH_PipeHandleTypeDef* handle){
	PipeStatus* pipe = (PipeStatus* )handle;
	__IO  USB_OTG_HostChannelTypeDef * hc = pipe->hc;
	uint8_t ch_num =  pipe->chan_num;
	uint32_t tmp_reg;
	hhcd.hc[ch_num].ch_num = ch_num;
	hhcd.hc[ch_num].ep_type = EP_TYPE_CTRL;
	hhcd.hc[ch_num].ep_num = 0;
	hhcd.hc[ch_num].ep_is_in = 0; // set out first
	//hc->HCINT = 0xFFFFFFFFU;  /* Clear old interrupt conditions for this host channel. */
	hc->HCINTMSK = USB_OTG_HCINTMSK_XFRCM | USB_OTG_HCINTMSK_STALLM | USB_OTG_HCINTMSK_TXERRM
			     | USB_OTG_HCINTMSK_DTERRM | USB_OTG_HCINTMSK_AHBERR | USB_OTG_HCINTMSK_NAKM ;
   tmp_reg = hc->HCCHAR & ~(USB_OTG_HCCHAR_EPNUM | USB_OTG_HCCHAR_EPDIR | USB_OTG_HCCHAR_EPTYP);
   tmp_reg |= (((0 & 0x7FU)<< 11U) & USB_OTG_HCCHAR_EPNUM); // end point 0
   tmp_reg |= (EP_TYPE_CTRL << 18U) & USB_OTG_HCCHAR_EPTYP; // type
   hc->HCCHAR = tmp_reg;
}
void USBH_SubmitSetup(USBH_PipeHandleTypeDef* hpipe, uint32_t* setup) {
	PipeStatus* pipe = (PipeStatus* )hpipe;
	__IO  USB_OTG_HostChannelTypeDef * hc = pipe->hc;
	uint8_t ch_num =  pipe->chan_num;
	hhcd.hc[ch_num].data_pid = HC_PID_SETUP;
	hhcd.hc[ch_num].ep_is_in = 0;
	hhcd.hc[ch_num].ep_type  = EP_TYPE_CTRL;
	hhcd.hc[ch_num].xfer_buff = (uint8_t)setup;
	hhcd.hc[ch_num].xfer_len  = 8;
	hhcd.hc[ch_num].urb_state = URB_IDLE;
	hhcd.hc[ch_num].xfer_count = 0U;
	hhcd.hc[ch_num].ch_num = ch_num;
	hhcd.hc[ch_num].state = HC_IDLE;
	USB_HC_StartXfer(hhcd.Instance, &(hhcd.hc[ch_num]), 0);
}
void USBH_SubmitControl(USBH_PipeHandleTypeDef* hpipe, uint32_t* data,uint16_t length){
	PipeStatus* pipe = (PipeStatus* )hpipe;
	__IO  USB_OTG_HostChannelTypeDef * hc = pipe->hc;
	uint8_t ch_num =  pipe->chan_num;
	hhcd.hc[ch_num].data_pid = HC_PID_DATA1;
	hhcd.hc[ch_num].ep_type  = EP_TYPE_CTRL;
	hhcd.hc[ch_num].xfer_buff = (uint8_t)data;
	hhcd.hc[ch_num].xfer_len  = length;
	hhcd.hc[ch_num].urb_state = URB_IDLE;
	hhcd.hc[ch_num].xfer_count = 0U;
	hhcd.hc[ch_num].ch_num = ch_num;
	hhcd.hc[ch_num].state = HC_IDLE;
	if(hhcd.hc[ch_num].ep_is_in == 0) /*send data */
	{
	  if (length == 0U) hhcd.hc[ch_num].toggle_out = 1U;
	  if (hhcd.hc[ch_num].toggle_out == 0U)
		hhcd.hc[ch_num].data_pid = HC_PID_DATA0;
	  else
		hhcd.hc[ch_num].data_pid = HC_PID_DATA1;
	}
	USB_HC_StartXfer(hhcd.Instance, &(hhcd.hc[ch_num]), hhcd.Init.dma_enable);
}
void RawSendSetup(PipeStatus* pipe, uint32_t* setup) {
	__IO  USB_OTG_HostChannelTypeDef * hc = pipe->hc;
	uint8_t ch_num =  pipe->chan_num;
	hc->HCTSIZ = USBH_HCTSIZ_XFRSIZ(8) | USBH_HCTSIZ_PKTCNT(1); // size of the packet setup
	hc->HCINTMSK = USB_OTG_HCINTMSK_XFRCM | USB_OTG_HCINTMSK_STALLM | USB_OTG_HCINTMSK_TXERRM // interrupts
			     | USB_OTG_HCINTMSK_DTERRM | USB_OTG_HCINTMSK_AHBERR | USB_OTG_HCINTMSK_NAKM ;

	USB_HOST(hhcd.Instance)->HAINTMSK |= (1 << pipe->chan_num); // make sure we are unmasked
	uint32_t tmp_reg = hc->HCCHAR & ~(USB_OTG_HCCHAR_CHDIS | USB_OTG_HCCHAR_ODDFRM | USB_OTG_HCCHAR_EPNUM | USB_OTG_HCCHAR_EPDIR | USB_OTG_HCCHAR_EPTYP);
	tmp_reg |= USBH_HCCHAR_EPNUM(0); // end point 0
	tmp_reg |= USBH_HCCHAR_TYPE(EP_TYPE_CTRL); // type
	tmp_reg |= USB_OTG_HCCHAR_CHENA; /* Set host channel enable */
	if(USB_HOST(hhcd.Instance)->HFNUM & 0x1U) tmp_reg |= USB_OTG_HCCHAR_ODDFRM; // odd frame
	hc->HCCHAR = tmp_reg;
	USB_DFIFO(hhcd.Instance, ch_num) = *((__packed uint32_t *)setup); setup++;
	USB_DFIFO(hhcd.Instance, ch_num) = *((__packed uint32_t *)setup);
}
// returns pipe for debugging
void USBH_WaitPipeBlocking(uint8_t pipe) {
	USB_OTG_URBStateTypeDef status;
	while(1){
		status = USB_HOST_PipeState(pipe);
		if(status == URB_IDLE) continue;
		if(status == URB_DONE) return;
		USBH_DbgLog("USBH_WaitPipeBlocking = %s", ENUM_TO_STRING(USB_OTG_URBStateTypeDef,status));
		assert(0);
	}
}

PipeStatus*  TestDescNonBlockingSetup(USBH_DeviceTypeDef* dev, USBH_ControlRequestTypeDef* ctrl) {
	ctrl->status = USBH_PIPE_WORKING;
	USB_HOST_InitEndpoint(1,EP_TYPE_CTRL, 0,0);
	USB_SubmitSetup(1,(uint8_t*)ctrl);
	USBH_WaitPipeBlocking(1);
	USB_HOST_InitEndpoint(1,EP_TYPE_CTRL, 0,1);
	USB_SubmitTrasfer(1, (uint8_t*)ctrl->Data, ctrl->Length);
	USBH_WaitPipeBlocking(1);
	USB_HOST_InitEndpoint(1,EP_TYPE_CTRL, 0,0);
	USB_SubmitTrasfer(1, NULL, 0);
	USBH_WaitPipeBlocking(1);
	ctrl->status = USBH_PIPE_IDLE;
#if 0
	//PipeStatus* pipe = OpenPipe(dev);
	assert(pipe);
	USBH_DbgLog("TestDescNonBlockingSetup pipe=%i", pipe->chan_num);

	USBH_PipeHandleTypeDef* hpipe=(USBH_PipeHandleTypeDef*)&(pipe->handle);
	if(ctrl->Length>0) {
		if(ctrl->RequestType  &  USB_SETUP_DEVICE_TO_HOST)
			USBH_SetPipeCallback(hpipe, PipeCallbackRecvData,ctrl);
		else
			USBH_SetPipeCallback(hpipe, PipeCallbackSendData,ctrl);
	}else {
		if(ctrl->RequestType  &  USB_SETUP_DEVICE_TO_HOST)
			USBH_SetPipeCallback(hpipe, PipeCallbackSendData,ctrl);
		else
			USBH_SetPipeCallback(hpipe, PipeCallbackRecvData,ctrl);
	}
	pipe->handle.status = USBH_PIPE_WORKING;
	SetPipeEndpointConfig(pipe, 0x00,EP_TYPE_CTRL, 0);
	PrintHCCHAR(pipe->hc->HCCHAR);
	//RawSendSetup(pipe, (uint32_t*)ctrl);
	USBH_SubmitSetup(pipe, (uint8_t*)ctrl);
#endif
	return NULL; ///pipe;
}


USBH_URBStateTypeDef USBH_WaitPipe(USBH_ControlRequestTypeDef* ctrl,uint8_t debug_pipe) {
	HCD_URBStateTypeDef first = URB_IDLE;
	USBH_DbgLog("USBH_WaitPipe");
	while(ctrl->status == USBH_PIPE_WORKING){
		HCD_URBStateTypeDef current= HAL_HCD_HC_GetURBState(&hhcd,debug_pipe);
		if(first != current){

			USBH_DbgLog("USBH_WaitPipe URB State Change %s -> %s", ENUM_TO_STRING(HCD_URBStateTypeDef, first),
					 ENUM_TO_STRING(HCD_URBStateTypeDef, current)
					);

			first= current;
		}
	}
		//USBH_DbgLog("USBH_WaitEndpoint pipe=%i state=%s", pipe, ENUM_TO_STRING(USBH_URBStateTypeDef, pipe_status[pipe].urb_state));
	return ctrl->status;
}


USBH_StatusTypeDef 			 USBH_Start(){
	HAL_HCD_Start(&hhcd);
	SetVBUS(true);
	HAL_Delay(200);
	return USBH_OK;
}
USBH_StatusTypeDef 			 USBH_Stop(){
	HAL_HCD_Stop(&hhcd);
	SetVBUS(false);
	HAL_Delay(200);
	return USBH_OK;
}



USBH_URBStateTypeDef TestGetDesc3(USBH_DeviceTypeDef* dev, USB_DEVICE_DESCRIPTOR* desc, uint8_t length) {
	USBH_ControlRequestTypeDef ctrl;
	ctrl.RequestType 	= USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_DEVICE | USB_SETUP_TYPE_STANDARD;
	ctrl.Request 		= USB_REQUEST_GET_DESCRIPTOR;
	ctrl.Value 			= ((USB_DESCRIPTOR_DEVICE << 8) & 0xFF00);// USB_DESCRIPTOR_DEVICE;
	ctrl.Index  		= 0;
	ctrl.Length  		= length;// sizeof(USB_DEVICE_DESCRIPTOR);
	ctrl.Data = (uint8_t*)desc;
	USB_HOST_SubmitControlRequest(1,&ctrl);
	while(1){
		switch(ctrl.status){
		case USB_HOST_WORKING: continue;
		case USB_HOST_ERROR:
			assert(0);
			break;
		}
		break;
	}
	//while(ctrl.status == USB_HOST_WORKING);
	//assert(ctrl.status == USB_HOST_COMPLETE);
	//PipeStatus* pipe =
	//TestDescNonBlockingSetup(dev,&ctrl);
	return USBH_URB_DONE;
}

struct timeval t1, t2, t3,t4;
double elapsedTime;
#ifndef timersub
#define	timersub(tvp, uvp, vvp)						\
	do {								\
		(vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;		\
		(vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;	\
		if ((vvp)->tv_usec < 0) {				\
			(vvp)->tv_sec--;				\
			(vvp)->tv_usec += 1000000;			\
		}							\
	} while (0)
#define	timeradd(tvp, uvp, vvp)						\
	do {								\
		(vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;		\
		(vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;	\
		if ((vvp)->tv_usec >= 1000000) {			\
			(vvp)->tv_sec++;				\
			(vvp)->tv_usec -= 1000000;			\
		}							\
	} while (0)
#define	timerdiv(tvp, uvp, vvp)						\
	do {								\
		(vvp)->tv_sec = (tvp)->tv_sec / (uvp)->tv_sec;		\
		(vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;	\
		if ((vvp)->tv_usec >= 1000000) {			\
			(vvp)->tv_sec++;				\
			(vvp)->tv_usec -= 1000000;			\
		}							\
	} while (0)
#endif


void PrintTime(const char* message,struct timeval* tv){
	USBH_DbgLog("%s diff=%lu.%06lu",
				message,tv->tv_sec, tv->tv_usec);
}
void PrintFloatTime(const char* message,float f){
	struct timeval tv;
	tv.tv_sec = (time_t)f;
	tv.tv_usec = (long)((f - (float)tv.tv_sec)*1000000);
	PrintTime(message,&tv);
}
USB_DEVICE_DESCRIPTOR root_dev_desc;

float TestDevRetreavalSpeed(USBH_DeviceTypeDef* dev) {
	struct timeval t1, t2, t3;
	gettimeofday(&t1, NULL);
	assert(TestGetDesc3(dev,&root_dev_desc,18)==USBH_URB_DONE);
	gettimeofday(&t2, NULL);
	timersub(&t2,&t1, &t3);
	return ((float)t3.tv_sec  + (((float)t3.tv_usec)/1000000.0f));
}
void BenchmarkLoop(USBH_DeviceTypeDef* dev, uint32_t count) {
	float sum=0.0f,sumsq=0.0f;
	for(uint32_t i=0;i < count; i++) {
		float time = TestDevRetreavalSpeed(dev);
		sum+=time;
		sumsq+=time*time;
	}
	float var = sumsq - ((sum*sum)/(float)count)/((float)(count -1));
    PrintFloatTime("BenchmarkLoop Average",sum/(float)count);
	PrintFloatTime("BenchmarkLoop Varanence",var);
}
void TestBlocking() {
	USBH_DeviceTypeDef test_dev;
	test_dev.Address = 0;
	test_dev.MaxPacketSize = 8;
	test_dev.Speed = port_speed;
	USB_HOST_SetPipeMaxPacketSize(1, 8) ;
	USB_HOST_SetPipeDevAddress(1, 0);
	//const USBH_CtrlType2TypeDef* ctrl = USBH_OpenControl(0, 8);
	assert(TestGetDesc3(&test_dev,&root_dev_desc,8)==USBH_URB_DONE);
	assert(test_dev.MaxPacketSize!=0);
		USBH_DbgLog("Packet size %i",  root_dev_desc.bMaxPacketSize0);
	test_dev.MaxPacketSize = root_dev_desc.bMaxPacketSize0;
	USB_HOST_SetPipeMaxPacketSize(1, root_dev_desc.bMaxPacketSize0) ;

	BenchmarkLoop(&test_dev,20);
	PRINT_STRUCT(USB_DEVICE_DESCRIPTOR,&root_dev_desc);
}
void USBH_SetUserData(void* userdata) { hhcd.pData = userdata; }


bool enumerate_root_device = false;
USBH_URBStateTypeDef  		 USBH_Poll() {
	if(enumerate_root_device){
			//usbh_timer = 0;
		TestBlocking(0,8);
		enumerate_root_device = false;
	}
	return USBH_OK;
}
void USB_HOST_Disconnect() {
	enumerate_root_device = false;
}
void USB_HOST_Connect() {
	USBH_DbgLog("USB_HOST_Connect");
	enumerate_root_device = true;
}

void HAL_HCD_HC_NotifyURBChange_Callback(HCD_HandleTypeDef *hhcd, uint8_t chnum, HCD_URBStateTypeDef urb_state){
	USBH_URBStateTypeDef usbh_state = (USBH_URBStateTypeDef)urb_state;
	PipeStatus* ps = &pipe_status_list[chnum];
	//if(urb_state != USBH_URB_DONE)
		USBH_DbgLog("NotifyURBChange pipe=%i %s %s", chnum,ENUM_TO_STRING(USB_OTG_HCStateTypeDef, hhcd->hc[chnum].state),ENUM_TO_STRING(USBH_URBStateTypeDef,usbh_state));
	if(usbh_state == USBH_URB_IDLE) return;
	if(ps->handle.status == USBH_PIPE_WORKING){
		ps->handle.urb_state= (USBH_URBStateTypeDef)urb_state;

		if(ps->Callback) ps->Callback(ps->owner, (USBH_PipeHandleTypeDef*)ps, ps->udata);
		else ps->handle.hc_state=  USBH_PIPE_IDLE;
	}
}

