#include "usb_host.h"
#include "usbh_desc.h"
#include "link_list.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdarg.h>
#include "usart.h"

// timer functions
#define TIMx                           TIM3
#define TIMx_CLK_ENABLE()              __HAL_RCC_TIM3_CLK_ENABLE()
/* Definition for TIMx's NVIC */
#define TIMx_IRQn                      TIM3_IRQn
#define TIMx_IRQHandler                TIM3_IRQHandler
typedef void (*TimerCallback)(void*);
//static TIM_HandleTypeDef USBH_ProcessTimer;
typedef enum {
	HOST_PORT_DISCONNECTED=0,
	HOST_PORT_CONNECTED,
	HOST_PORT_WAIT_FOR_ATTACHMENT,
	HOST_PORT_DISCONNECTING_PORT_OFF,
	HOST_PORT_DISCONNECTING_PORT_ON,
	HOST_PORT_IDLE,		// no data going in or out
	HOST_PORT_ACTIVE,	// waiting for packet or finish trasmit
} PORT_StateTypeDef;
static PORT_StateTypeDef port_state = HOST_PORT_DISCONNECTED;
// only support 15 end points in high and only 11 in full ugh, REMEMBER THIS
HCD_HandleTypeDef hhcd;
static uint32_t usbh_timer = 0;
static USBH_PortSpeedTypeDef port_speed;
static TimerCallback timer_callback = NULL;
static void* timer_data_callback=NULL;
static volatile uint32_t ms_prescale_value;

struct __PipeStatus ;
typedef void (*PipeCallback)(struct __PipeStatus *, uint8_t);

typedef struct __PipeStatus {
	__IO USBH_EndpointStatusTypeDef status;
	__IO USBH_URBStateTypeDef urb_state;
	__IO USBH_URBStateTypeDef hc_state;
	PipeCallback Callback;
	void* data;
	uint32_t value;
} PipeStatus;

PipeStatus pipe_status[15];

void StartPipe(uint8_t i) {
	assert(pipe_status[i].status != USBH_PIPE_WORKING);
	pipe_status[i].status = USBH_PIPE_WORKING;
	pipe_status[i].urb_state = USBH_URB_IDLE;
	pipe_status[i].hc_state = HC_IDLE;
	pipe_status[i].Callback = NULL;
	pipe_status[i].data = NULL;
	pipe_status[i].value = 0;

}
void StopPipe(uint8_t i) {
	pipe_status[i].status = USBH_PIPE_IDLE;
	pipe_status[i].Callback = NULL;
	pipe_status[i].data = NULL;
	pipe_status[i].value = 0;
}
void StartPipeCallback(uint8_t i,PipeCallback callback, void* data, uint32_t value) {
	assert(pipe_status[i].status != USBH_PIPE_WORKING);
	pipe_status[i].status = USBH_PIPE_WORKING;
	pipe_status[i].urb_state = USBH_URB_IDLE;
	pipe_status[i].hc_state = HC_IDLE;
	pipe_status[i].Callback = callback;
	pipe_status[i].data = data;
	pipe_status[i].value = value;
}
typedef struct {
	uint8_t* data;
	uint32_t size;
} t_buffer;

typedef struct _internal_endpoint {
	USBH_EndpointTypeDef ep;
	bool initialized;
	void (*Callback)(const USBH_EndpointTypeDef*, uint8_t);
	void* pData;
} t_internal_endpoint;

struct _internal_event {
	uint32_t event;
	uint32_t lparm;
	uint32_t rparm;
};
#define MAX_ENDPOINTS 15
t_internal_endpoint endpoints[MAX_ENDPOINTS];

typedef struct {
	uint8_t ep_type;
	uint8_t ep_direction;
	uint8_t do_ping;
} EpTypeConvertTypeDef;

static const EpTypeConvertTypeDef EpTypeConvert[] = {
	{ EP_TYPE_CTRL, 0,  0 },
	{ EP_TYPE_CTRL, 1,  0 },
	{ EP_TYPE_ISOC, 0,  1 },
	{ EP_TYPE_ISOC, 1,  0 },
	{ EP_TYPE_BULK, 0,  1 },
	{ EP_TYPE_BULK, 1,  0 },
	{ EP_TYPE_INTR, 0,  1 },
	{ EP_TYPE_INTR, 1,  0 },
};

void* USBH_malloc(size_t size, void** owner) {
	(void)owner;
	return malloc(size);
}
void USBH_free(void * ptr) { free(ptr); }
void USBH_memset(void* ptr, uint8_t value, size_t size) { memset(ptr,value,size); }
void USBH_memcpy(void* dst, const void* src, size_t size){ memcpy(dst,src,size); }


void USBH_Log(USBH_LogLevelTypeDef level, const char* fmt, ...){
	uint32_t ticks = HAL_GetTick();
	static const char* level_to_string[] = {
			"\033[41mERROR\033[0m:",
			"\033[33mWARN \033[0m:",
			"\033[33mUSER \033[0m:",
			"\033[37mDEBUG\033[0m:",
	};
	struct timeval t1;
	gettimeofday(&t1, NULL);
	uart_print("[%lu.%lu]%s", t1.tv_sec ,t1.tv_usec ,level_to_string[level]);
	char buf[128];
	va_list va;
	va_start(va,fmt);
	vsprintf(buf,fmt,va);
	va_end(va);
	buf[127]=0;
	uart_puts(buf);
	uart_puts("\r\n");
}

//static TIM_HandleTypeDef USBH_Timer;

static void InitUSBTimer() {
	for(int i=0;i<15;i++) StopPipe(i);
	printf("Its only bytes=%i\r\n", sizeof(USBH_EndpointTypeDef));
	// TIM{2,3,4,5,6,7,12,13,14} are on APB1
	 uint32_t source = HAL_RCC_GetPCLK1Freq();
	 if ((uint32_t)(RCC->CFGR & RCC_CFGR_PPRE1) != RCC_HCLK_DIV1)   source *= 2;
	 source /=2; // TIM_CR1_CKD_1
	  ms_prescale_value = (uint32_t)(((source/ 1000)) - 1); // 2ms
	  assert(ms_prescale_value < 0x10000);
	  uart_print("Setting source=%lu PSC=%lu \r\n", source,ms_prescale_value);
#if 0
	  USBH_ProcessTimer.Instance = TIMx;
	  USBH_ProcessTimer.Init.Period            = 999999; /* 1 second */
	  USBH_ProcessTimer.Init.Prescaler         = uwPrescalerValue;
	  USBH_ProcessTimer.Init.ClockDivision     = 0;
	  USBH_ProcessTimer.Init.CounterMode       = TIM_COUNTERMODE_UP;
	  USBH_ProcessTimer.Init.RepetitionCounter = 0;
	  	TIMx_CLK_ENABLE();
	  	assert (HAL_TIM_Base_Init(&USBH_ProcessTimer) == HAL_OK);
#endif
	  	///assert(HAL_TIM_Base_Start_IT(&USBH_ProcessTimer) == HAL_OK);
	 TIMx_CLK_ENABLE();
	 TIMx->PSC  = (uint16_t)ms_prescale_value;
	 TIMx->EGR = TIM_EGR_UG;
	 while ((TIMx->SR & TIM_SR_UIF)==0);
	 TIMx->SR=0; // clear
	 uint32_t value = TIMx->PSC;
	 uart_print("Setting source=%i    PSC=%i \r\n", (int)source, (int)value);
	 assert( TIMx->PSC== ms_prescale_value);
	 HAL_NVIC_SetPriority(TIMx_IRQn, 3, 0);
	 HAL_NVIC_EnableIRQ(TIMx_IRQn);  /* Enable the TIMx global Interrupt */
	// TIMx->CR1 |= TIM_CR1_CEN | TIM_CR1_OPM;
}
volatile uint32_t ticks_test;
void TIMx_IRQHandler() {
	if(TIMx->SR & TIM_SR_UIF) {
		ticks_test = HAL_GetTick()-ticks_test;
		TIMx->SR &= ~TIM_SR_UIF; // clear it
		if(timer_callback) timer_callback(timer_data_callback);
		uart_print("Ticks %lu\r\n", ticks_test);
	}
}
void SetTimer(uint16_t ms_delay) {
	assert((TIMx->CR1 & TIM_CR1_CEN)==0); // test if we are running
	TIMx->SR = 0;
	//TIMx->PSC  = ms_prescale_value;
	TIMx->ARR = (uint16_t)(ms_delay<<1); 	// have to double it as its a 16bit counter
	TIMx->DIER |= TIM_DIER_UIE;		// make sure IT is enabled

}
static void OneShotTimer(uint16_t ms_delay, TimerCallback func, void* data){
	uart_print("StartTicks %lu\r\n", ms_delay);
	timer_data_callback = data;
	timer_callback = func;
	SetTimer(ms_delay);
	ticks_test = HAL_GetTick();
	TIMx->CR1 = TIM_CR1_CEN | TIM_CR1_OPM  | TIM_CR1_CKD_0;; // OPM is one shot and cmkd_! /4
}

static void _InitEndpoint(USBH_EndpointTypeDef* ep) {
	const EpTypeConvertTypeDef* cvt = &EpTypeConvert[ep->Init.Type];
	ep->Direction = cvt->ep_direction;
	ep->DoPing = cvt->do_ping;
	ep->Type = cvt->ep_type;
	if(ep->Init.Speed == USBH_SPEED_DEFAULT_PORT) ep->Init.Speed = (USBH_PortSpeedTypeDef)HAL_HCD_GetCurrentSpeed(&hhcd);
	if(ep->Direction) ep->Init.Number|=0x80; // not sure if this is always true but eh
	HAL_HCD_HC_Init(&hhcd, ep->Channel, ep->Init.Number, ep->Init.DevAddr, ep->Init.Speed, ep->Init.Type , ep->Init.MaxPacketSize);
}
const USBH_EndpointTypeDef*
USBH_OpenEndpoint(const USBH_EndpointInitTypeDef* Init) {
	uint8_t max = port_speed == USBH_SPEED_FULL ? (11-1) : (15-1);
	for(uint8_t i=0; i < max; i++){
		t_internal_endpoint* iep = &endpoints[i];
		if(iep->ep.status == USBH_PIPE_NOT_ALLOCATED) {
			iep->ep.status = USBH_PIPE_IDLE;
			iep->ep.Channel = i+1;
			const EpTypeConvertTypeDef* cvt = &EpTypeConvert[Init->Type];
			iep->ep.Init = *Init;
			_InitEndpoint(&iep->ep);
			iep->initialized = true;
			return &iep->ep;
		}
	}
	return NULL;
}
void USBH_ModifyEndpoint(const USBH_EndpointTypeDef* ep, const USBH_EndpointInitTypeDef* Init) {
	USBH_EndpointTypeDef* _ep = (USBH_EndpointTypeDef*)ep;
	_ep->Init = *Init;
	_InitEndpoint(_ep);
}
void USBH_ModifyEndpointCallback(const USBH_EndpointTypeDef* ep,   void (*Callback)(const USBH_EndpointTypeDef*, void*), void* userdata){
	t_internal_endpoint* _ep = &endpoints[ep->Channel-1];
	_ep->Callback = Callback;
	_ep->pData = userdata;
}

void CloseEndpoint(const USBH_EndpointTypeDef* ep) {
	t_internal_endpoint* _ep = &endpoints[ep->Channel-1];
	_ep->ep.status = USBH_PIPE_NOT_ALLOCATED;
	_ep->Callback = NULL;
    _ep->pData = NULL;
	HAL_HCD_HC_Halt(&hhcd,ep->Channel);
}

USBH_URBStateTypeDef USBH_PipeStatus(uint8_t pipe) {
	return (USBH_URBStateTypeDef)HAL_HCD_HC_GetURBState(&hhcd,pipe);
}

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
USBH_StatusTypeDef USBH_Init(void* user_data)
{
	InitUSBTimer(); // init the process timer
  /* Set the LL Driver parameters */
  hhcd.Instance = USB_OTG_FS;
  hhcd.Init.Host_channels = 11;
  hhcd.Init.dma_enable = 0;
  hhcd.Init.low_power_enable = 0;
  hhcd.Init.phy_itface = HCD_PHY_EMBEDDED;
  hhcd.Init.Sof_enable = 0;
  hhcd.Init.speed = HCD_SPEED_FULL;
  hhcd.Init.vbus_sensing_enable = 0;
  hhcd.Init.lpm_enable = 0;

  /* Link the driver to the stack */
  hhcd.pData = user_data;

  /* Initialize the LL Driver */
  HAL_HCD_Init(&hhcd);
  usbh_timer = HAL_HCD_GetCurrentFrame(&hhcd);
  USBH_Stop();
  USBH_Start();
  return USBH_OK;
}
static void SetEndPointStatus(const USBH_EndpointTypeDef* cep, USBH_EndpointStatusTypeDef status) {
	USBH_EndpointTypeDef* ep = (USBH_EndpointTypeDef*)cep;
	ep->status = status;
}
// this main function does it all, the rest are just helpers
void  USBH_SubmitRequest(const USBH_EndpointTypeDef* ep, uint8_t* data, uint16_t length) {
	assert(ep);
	assert(ep->status == USBH_PIPE_IDLE);
	SetEndPointStatus(ep, USBH_PIPE_WORKING);
	HAL_HCD_HC_SubmitRequest (&hhcd,  ep->Channel,  ep->Direction , ep->Type ,1, data,length,ep->DoPing);
}

void  USBH_SubmitSetup(const USBH_EndpointTypeDef* ep, const USB_Setup_TypeDef* setup) {
	assert(ep);
	assert(setup);
	assert(ep->status == USBH_PIPE_IDLE);
	SetEndPointStatus(ep, USBH_PIPE_WORKING);
	if(ep->Type != EP_TYPE_CTRL) {
		USBH_ErrLog("Cannot submit setup without a Control Out endpoint!");
	}
	HAL_HCD_HC_SubmitRequest(&hhcd,  ep->Channel,  0, EP_TYPE_CTRL ,0, (uint8_t*)setup, 8,0);
}
USBH_URBStateTypeDef USBH_WaitEndpoint(const USBH_EndpointTypeDef* ep) {
	uint32_t timeout = HAL_GetTick()+100;
	do {
		if(ep->status != USBH_PIPE_WORKING) {
			USBH_DbgLog("USBH_WaitEndpoint pipe=%i state=%s", ep->Channel, ENUM_TO_STRING(USBH_URBStateTypeDef, ep->urb_state));
			return ep->urb_state;
		}
	} while(timeout > HAL_GetTick());
	USBH_DbgLog("USBH_WaitEndpoint pipe=%i state=%s", ep->Channel, ENUM_TO_STRING(USBH_URBStateTypeDef, USBH_URB_TIMEOUT));
	//HAL_HCD_HC_Halt(&hhcd,ep->Channel); // stop it
	return USBH_URB_TIMEOUT;
}
USBH_URBStateTypeDef USBH_WaitPipe(uint8_t pipe) {
	uint32_t timeout = usbh_timer+100;
	while(pipe_status[pipe].status == USBH_PIPE_WORKING) {
		uint32_t time = usbh_timer;
			if(timeout < time) {
			//	USBH_DbgLog("USBH_WaitEndpoint timeout pipe=%i %lu time=%lu", pipe,time, time-timeout);
				HAL_HCD_HC_Halt(&hhcd,pipe);
				HAL_Delay(1);
				return USBH_URB_TIMEOUT;
			}
		}
		//USBH_DbgLog("USBH_WaitEndpoint pipe=%i state=%s", pipe, ENUM_TO_STRING(USBH_URBStateTypeDef, pipe_status[pipe].urb_state));
		return (USBH_URBStateTypeDef)pipe_status[pipe].urb_state;;
}

USBH_URBStateTypeDef  USBH_SubmitRequestBlocking(const USBH_EndpointTypeDef* ep, uint8_t* data, uint16_t length){
	USBH_SubmitRequest(ep,data,length);
	return USBH_WaitEndpoint(ep);
}

USBH_URBStateTypeDef  USBH_SubmitSetupBlocking(const USBH_EndpointTypeDef* ep, const USB_Setup_TypeDef* setup){
	USBH_SubmitSetup(ep,setup);
	return USBH_WaitEndpoint(ep);
}

USBH_URBStateTypeDef  _USBH_SubmitSetup(uint8_t pipe, const USB_Setup_TypeDef* setup) {
	//USBH_ErrLog("_USBH_SubmitSetup");
	StartPipe(pipe);
	HAL_HCD_HC_SubmitRequest(&hhcd,  pipe,  0, EP_TYPE_CTRL ,0, (uint8_t*)setup, 8,0);
	return USBH_WaitPipe(pipe);
}
USBH_URBStateTypeDef  _USBH_RecvControl(uint8_t pipe, uint8_t* data, uint16_t length) {
	//USBH_ErrLog("_USBH_RecvControl");
	StartPipe(pipe);
	USBH_CtlReceiveData(data,length,pipe);
	//HAL_HCD_HC_SubmitRequest(&hhcd,  pipe,  1, EP_TYPE_CTRL ,1, (uint8_t*)data, length,0);
	return USBH_WaitPipe(pipe);
}
USBH_URBStateTypeDef  _USBH_SendControl(uint8_t pipe, uint8_t* data, uint16_t length) {
	//USBH_ErrLog("_USBH_SendControl");
	StartPipe(pipe);
	USBH_CtlSendData(data,length,pipe,0);
	//HAL_HCD_HC_SubmitRequest(&hhcd,  pipe,  0, EP_TYPE_CTRL ,1, (uint8_t*)data, length,0); // ping?
	return USBH_WaitPipe(pipe);
}

static void SetVBUS(bool value) {
	  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, value ? GPIO_PIN_SET: GPIO_PIN_RESET);
	  // HAL_Delay(200);
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




void PipeCallbackSendData(PipeStatus* status, uint8_t pipe);
void PipeCallbackRecvData(PipeStatus* status, uint8_t pipe) ;
void PipeCallbackRecvStatus(PipeStatus* status, uint8_t pipe);
void PipeCallbackSendStatus(PipeStatus* status, uint8_t pipe);

// WORKS! DO NOT OTUCH
void PipeCallbackRecvData(PipeStatus* status, uint8_t pipe) {
	USBH_DbgLog("PipeCallbackRecvData");
	HCD_HCTypeDef* hc = &hhcd.hc[pipe];
	HAL_HCD_HC_Init(&hhcd, pipe, hc->ep_num  & 0x80, hc->dev_addr, hc->speed, hc->ep_type,hc->max_packet);
	StartPipeCallback(pipe, PipeCallbackSendStatus, NULL,0);
	HAL_HCD_HC_SubmitRequest(&hhcd, pipe,1, hc->ep_type, 1, status->data, status->value,0);
}
void PipeCallbackSendData(PipeStatus* status, uint8_t pipe) {
	USBH_DbgLog("PipeCallbackSendData");
	HCD_HCTypeDef* hc = &hhcd.hc[pipe];
	HAL_HCD_HC_Init(&hhcd, pipe, hc->ep_num, hc->dev_addr, hc->speed, hc->ep_type,hc->max_packet);
	StartPipeCallback(pipe, PipeCallbackRecvStatus, NULL,0);
	HAL_HCD_HC_SubmitRequest(&hhcd, pipe,0, hc->ep_type, 1, status->data, status->value,0);
}
void PipeCallbackRecvStatus(PipeStatus* status, uint8_t pipe) {
	USBH_DbgLog("PipeCallbackRecvStatus");
	UNUSED(status);
	HCD_HCTypeDef* hc = &hhcd.hc[pipe];
	HAL_HCD_HC_Init(&hhcd, pipe, hc->ep_num  & 0x80, hc->dev_addr, hc->speed, hc->ep_type,hc->max_packet);
	StartPipe(pipe);
	HAL_HCD_HC_SubmitRequest(&hhcd, pipe,1, hc->ep_type, 1, NULL, 0 ,0);
}
void PipeCallbackSendStatus(PipeStatus* status, uint8_t pipe) {
	USBH_DbgLog("PipeCallbackSendStatus");
	UNUSED(status);
	HCD_HCTypeDef* hc = &hhcd.hc[pipe];
	HAL_HCD_HC_Init(&hhcd, pipe, hc->ep_num, hc->dev_addr, hc->speed, hc->ep_type,hc->max_packet);
	StartPipe(pipe);
	HAL_HCD_HC_SubmitRequest(&hhcd, pipe,0, hc->ep_type, 1,  NULL, 0 ,0);
}

void TestDescBlockingSetup(uint8_t pipe, uint8_t addr, uint8_t pkt_size, USB_Setup_TypeDef* setup, void* data)
{
	StopPipe(pipe);
	if(setup->b.wLength.w>0) {
		if(setup->b.bmRequestType  &  USB_SETUP_DEVICE_TO_HOST)
			StartPipeCallback(pipe, PipeCallbackRecvData, data,setup->b.wLength.w);
		else
			StartPipeCallback(pipe, PipeCallbackSendData, data,setup->b.wLength.w);
	}else {
		if(setup->b.bmRequestType  &  USB_SETUP_DEVICE_TO_HOST)
			StartPipeCallback(pipe, PipeCallbackSendData, NULL,0);
		else
			StartPipeCallback(pipe, PipeCallbackRecvData,  NULL,0);
	}
	USBH_DbgLog("Setup Packet");
	HAL_HCD_HC_SubmitRequest(&hhcd, pipe,0, EP_TYPE_CTRL, 0, (uint8_t*)setup->d8, 8,0);
}

USBH_URBStateTypeDef TestGetDesc(uint8_t pipe, uint8_t addr, uint8_t pkt_size, USB_DEVICE_DESCRIPTOR* desc, uint8_t length) {
	USB_Setup_TypeDef setup;
	setup.b.bmRequestType 	= USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_DEVICE | USB_SETUP_TYPE_STANDARD;
	setup.b.bRequest 		= USB_REQUEST_GET_DESCRIPTOR;
	setup.b.wValue.w  		=   ((USB_DESCRIPTOR_DEVICE << 8) & 0xFF00);// USB_DESCRIPTOR_DEVICE;
	setup.b.wIndex.w  		= 0;
	setup.b.wLength.w  		= length;// sizeof(USB_DEVICE_DESCRIPTOR);
	TestDescBlockingSetup(pipe, addr, pkt_size, &setup, desc);
	return USBH_WaitPipe(pipe);
}


void TestGetDesc3(uint8_t addr, uint8_t pkt_size, USB_DEVICE_DESCRIPTOR* desc, uint8_t length) {
	USB_Setup_TypeDef setup;
	setup.b.bmRequestType 	= USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_DEVICE | USB_SETUP_TYPE_STANDARD;
	setup.b.bRequest 		= USB_REQUEST_GET_DESCRIPTOR;
	setup.b.wValue.w  		=   ((USB_DESCRIPTOR_DEVICE << 8) & 0xFF00);// USB_DESCRIPTOR_DEVICE;
	setup.b.wIndex.w  		= 0;
	setup.b.wLength.w  		= length;// sizeof(USB_DEVICE_DESCRIPTOR);
	HAL_HCD_HC_Init(&hhcd, 1, 0x00, addr, port_speed, EP_TYPE_CTRL, pkt_size); // OUT

	assert(_USBH_SubmitSetup(1,&setup)==USBH_URB_DONE);
	HAL_HCD_HC_Init(&hhcd, 1, 0x80, addr, port_speed, EP_TYPE_CTRL, pkt_size); // IN
	//HAL_HCD_HC_Init(&hhcd, 1, 0x00, addr, port_speed, EP_TYPE_CTRL, pkt_size); // IN
	assert(_USBH_RecvControl(1, (uint8_t*)desc, length)==USBH_URB_DONE);
	HAL_HCD_HC_Init(&hhcd, 1, 0x00, addr, port_speed, EP_TYPE_CTRL, pkt_size); // OUT
	assert(_USBH_SendControl(1, NULL,0)==USBH_URB_DONE);
}
struct timeval t1, t2, t3,t4;
double elapsedTime;
void PrintTime(const char* message,struct timeval* tv){
	USBH_DbgLog("%s diff=%lu.%06lu",
				message,tv->tv_sec, tv->tv_usec);
}
USB_DEVICE_DESCRIPTOR root_dev_desc;
void TestBlocking() {
	gettimeofday(&t1, NULL);
	//const USBH_CtrlType2TypeDef* ctrl = USBH_OpenControl(0, 8);
	//HAL_HCD_HC_Init(&hhcd, 1, 0x00, 0, port_speed, EP_TYPE_CTRL, 8); // OUT
	//HAL_HCD_HC_Init(&hhcd, 2, 0x80, 0, port_speed, EP_TYPE_CTRL, 8); // IN

	assert(TestGetDesc(1,0,8,&root_dev_desc,8)==USBH_URB_DONE);
	//HAL_HCD_HC_Init(&hhcd, 1, 0x00, 0, port_speed, EP_TYPE_CTRL, root_dev_desc.bMaxPacketSize0); // OUT
	//HAL_HCD_HC_Init(&hhcd, 2, 0x80, 0, port_speed, EP_TYPE_CTRL, root_dev_desc.bMaxPacketSize0); // IN
	assert(TestGetDesc(1,0,root_dev_desc.bMaxPacketSize0,&root_dev_desc,18)==USBH_URB_DONE);
	gettimeofday(&t3, NULL);
	timersub(&t3,&t1, &t4);
	PrintTime("Final Time", &t4);
	PRINT_STRUCT(USB_DEVICE_DESCRIPTOR,&root_dev_desc);
}
void USBH_SetUserData(void* userdata) { hhcd.pData = userdata; }



bool enumerate_root_device = false;
USBH_URBStateTypeDef  		 USBH_Poll() {
	if(port_state == HOST_PORT_CONNECTED){
		if(enumerate_root_device){
			usbh_timer = 0;
			TestBlocking(0,8);
			enumerate_root_device = false;
		}

		//TestGetDeviceInfo() ;
	}
	return USBH_OK;
}

__attribute__((weak)) void USBH_PortConnectCallback(void* userdata)
{
	UNUSED(userdata);
	USBH_DbgLog("Port Connected!");
	 enumerate_root_device = true;
}
__attribute__((weak))  void USBH_PortDisconnectCallback(void* userdata)
{
	UNUSED(userdata);
	USBH_DbgLog("Port Disconnected!");
}

void HAL_HCD_SOF_Callback(HCD_HandleTypeDef *hhcd){
	UNUSED(hhcd);
	usbh_timer++;
}
// state machine used just for making solid connections



static void USBH_ResetPort(void * data) {
	port_state = (PORT_StateTypeDef)data;
	HAL_HCD_ResetPort(&hhcd);
}

static void USBH_DisconnectTimerCallback(void *data) {
	switch(port_state){
	case HOST_PORT_DISCONNECTING_PORT_OFF:
		HAL_HCD_Start(&hhcd);
		SetVBUS(true);
		port_state = HOST_PORT_DISCONNECTING_PORT_ON;
		OneShotTimer(200, USBH_DisconnectTimerCallback, NULL); // reset the port in 200 ms
		break;
	case HOST_PORT_DISCONNECTING_PORT_ON:
		port_state = HOST_PORT_DISCONNECTED;
		break;
	default: break;
	}
}


void HAL_HCD_Connect_Callback(HCD_HandleTypeDef *hhcd){
	switch(port_state) {
	case HOST_PORT_DISCONNECTED:
		OneShotTimer(200, USBH_ResetPort, (void*)HOST_PORT_WAIT_FOR_ATTACHMENT); // reset the port in 200 ms
		break;
	case HOST_PORT_WAIT_FOR_ATTACHMENT:
		port_state = HOST_PORT_CONNECTED;
		port_speed = USBH_GetSpeed();
		USBH_PortConnectCallback(hhcd->pData); // callback the connection
		break;
	default: break;
	}
}
void HAL_HCD_Disconnect_Callback(HCD_HandleTypeDef *hhcd){
	port_state = HOST_PORT_DISCONNECTING_PORT_OFF;
	// clear and flush all the pipes?
	HAL_HCD_Stop(hhcd);
	SetVBUS(false);
	USBH_PortDisconnectCallback(hhcd->pData);
	OneShotTimer(200, USBH_DisconnectTimerCallback, NULL); // reset the port in 200 ms
}


void HAL_HCD_HC_NotifyURBChange_Callback(HCD_HandleTypeDef *hhcd, uint8_t chnum, HCD_URBStateTypeDef urb_state){
	USBH_URBStateTypeDef usbh_state = (USBH_URBStateTypeDef)urb_state;
	PipeStatus* ps = &pipe_status[chnum];
	if(ps->status == USBH_PIPE_WORKING){
		if(usbh_state == USBH_URB_NOTREADY) return;
		ps->status = USBH_PIPE_IDLE;
		ps->urb_state= (USBH_URBStateTypeDef)urb_state;
		ps->hc_state=  hhcd->hc[chnum].state;
		if(ps->Callback) ps->Callback(ps,chnum);
		//USBH_DbgLog("NotifyURBChange pipe=%i %s %s", chnum,ENUM_TO_STRING(USB_OTG_HCStateTypeDef, ps->hc_state),ENUM_TO_STRING(USBH_URBStateTypeDef, ps->urb_state));
	}



}

