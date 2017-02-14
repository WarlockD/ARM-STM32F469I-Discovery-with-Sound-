#include "main.h"
#include "stm32f4xx_hal.h"
#include "usart.h"

// we config the system tick and a better timer here
#include <sys/_timeval.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/times.h>
#include <errno.h>
#include <assert.h>
#define TIMx                           TIM2
#define TIMx_CLK_ENABLE()              __HAL_RCC_TIM2_CLK_ENABLE()
/* Definition for TIMx's NVIC */
#define TIMx_IRQn                      TIM2_IRQn
#define TIMx_IRQHandler                TIM2_IRQHandler


#if 0
int getitimer(int which, struct itimerval *curr_value){
	if(!curr_value) {
			errno = EFAULT ;
			return -1;
		}
	if(which == ITIMER_VIRTUAL || which == ITIMER_PROF) {
		errno = EINVAL;
		return -1;
	}
	if(curr_value) *curr_value = itimer_real;
	return 0;
}
int setitimer(int which, const struct itimerval *new_value,
                     struct itimerval *old_value){
	if(!new_value) {
		errno = EFAULT ;
		return -1;
	}
	if(which == ITIMER_VIRTUAL || which == ITIMER_PROF) {
		errno = EINVAL;
		return -1;
	}
	if(old_value) *old_value = itimer_real;
	itimer_real = *new_value;
	return 0;
}
#endif
void print_time(const char* msg, struct timeval * tp) {
	uart_print("%s %i.%i\r\n", msg, (int)tp->tv_sec,(int)tp->tv_usec);
}
void fifo_fill_dma_buffer(); // for the usart
void HAL_SYSTICK_Callback() {
	fifo_fill_dma_buffer() ;
#if 0
	if(timerisset(&itimer_real.it_value)){
		struct timeval current;
		gettimeofday(&current,NULL);
		if(timercmp(&current, &itimer_real.it_value, >=)){
			raise(SIGALRM);
			if(timerisset(&itimer_real.it_interval)){
				timeradd(&current,&itimer_real.it_interval, &itimer_real.it_value);
				print_time("NEXT: ", &itimer_real.it_value);
			} else {
				timerclear(&itimer_real.it_value);
			}
		}
	}
#endif
}

// https://github.com/micropython/micropython/blob/master/stmhal/timer.c
// Get the frequency (in Hz) of the source clock for the given timer.
// On STM32F405/407/415/417 there are 2 cases for how the clock freq is set.
// If the APB prescaler is 1, then the timer clock is equal to its respective
// APB clock.  Otherwise (APB prescaler > 1) the timer clock is twice its
// respective APB clock.  See DM00031020 Rev 4, page 115.
uint32_t timer_get_source_freq(uint32_t tim_id) {
    uint32_t source;
    if (tim_id == 1 || (8 <= tim_id && tim_id <= 11)) {
        // TIM{1,8,9,10,11} are on APB2
        source = HAL_RCC_GetPCLK2Freq();
        if ((uint32_t)((RCC->CFGR & RCC_CFGR_PPRE2) >> 3) != RCC_HCLK_DIV1) {
            source *= 2;
        }
    } else {
        // TIM{2,3,4,5,6,7,12,13,14} are on APB1
        source = HAL_RCC_GetPCLK1Freq();
        if ((uint32_t)(RCC->CFGR & RCC_CFGR_PPRE1) != RCC_HCLK_DIV1) {
            source *= 2;
        }
    }
    //__HAL_TIM_GET_COUNTER
    return source;

}
volatile uint8_t sssf = 0;
void test_signal(int meh){
	sssf = 1;
}

#if 0

int _EXFUN(setitimer, (int __which, const struct itimerval *__restrict __value,
					struct itimerval *__restrict __ovalue));
int _EXFUN(utimes, (const char *__path, const struct timeval *__tvp));


int _EXFUN(adjtime, (const struct timeval *, struct timeval *));
int _EXFUN(futimes, (int, const struct timeval *));
int _EXFUN(futimesat, (int, const char *, const struct timeval [2]));
int _EXFUN(lutimes, (const char *, const struct timeval *));
int _EXFUN(settimeofday, (const struct timeval *, const struct timezone *));

int _EXFUN(getitimer, (int __which, struct itimerval *__value));
int _EXFUN(gettimeofday, (struct timeval *__restrict __p,
			  void *__restrict __tz));
#endif



TIM_HandleTypeDef        htim3;
uint32_t                 uwIncrementState = 0;
static TIM_HandleTypeDef ClockHandle;
static volatile struct timeval tv_time = { 0, 0};
static volatile bool hal_timer_enable = true;
void TIMx_IRQHandler(void) {
	if((TIMx->SR & TIM_SR_CC1IF) && (TIMx->DIER & TIM_DIER_CC1IE)){
		if(hal_timer_enable) HAL_IncTick();
		TIMx->CCR1 += 1000;
		if(TIMx->CCR1 > TIMx->ARR) TIMx->CCR1 = 999;
		TIMx->SR &= ~TIM_SR_CC1IF;
	}
	if((TIMx->SR & TIM_SR_UIF) && (TIMx->DIER & TIM_DIER_UIE)){
		tv_time.tv_usec = TIMx->CNT;
	    tv_time.tv_sec++;
	    //TIMx->CCR1 = 999;
	    TIMx->SR &= ~TIM_SR_UIF;
	}
}

int _times(struct tms *buf)
{
	if(buf) {
		buf->tms_cstime  = tv_time.tv_sec;
		buf->tms_cutime  = buf->tms_cstime;
		buf->tms_stime = buf->tms_cstime;
		buf->tms_utime = buf->tms_cstime;
		return 0;
	}
	errno = EACCES;
	return -1;
}
int _gettimeofday (struct timeval * tp, struct timezone * tzp)
{
  /* Return fixed data for the timezone.  */
  if (tzp) tzp->tz_dsttime = tzp->tz_minuteswest = 0;
  if(tp) {
	  *tp = tv_time;
	  tp->tv_usec += TIMx->CNT;
	  if(tp->tv_usec> 1000000){
		  tp->tv_sec+=1;
		  tp->tv_usec -= 1000000;
	  }
  }
  return 0;
}

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
	uint32_t  uwPrescalerValue = (uint32_t)((timer_get_source_freq(2) / 1000000) - 1); // 1us
	/* Set TIMx instance */
	ClockHandle.Instance = TIMx;
	ClockHandle.Init.Period            = 999999; /* 1 second */
	//ClockHandle.Init.Period            = 999; /* 1 millsecond */
	ClockHandle.Init.Prescaler         = uwPrescalerValue;
	ClockHandle.Init.ClockDivision     = 0;
	ClockHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
	ClockHandle.Init.RepetitionCounter = 0;
	TIMx_CLK_ENABLE();
	assert (HAL_TIM_Base_Init(&ClockHandle) == HAL_OK);
	TIMx->CCER |= TIM_CCER_CC1E;
	TIMx->DIER |= TIM_DIER_CC1IE;
	TIMx->CCR1 = 999;
	assert (HAL_TIM_Base_Start_IT(&htim3)== HAL_OK);
	__HAL_TIM_ENABLE_IT(&ClockHandle, TIM_IT_UPDATE);
	__HAL_TIM_ENABLE(&ClockHandle);
	//assert(HAL_TIM_Base_Start_IT(&ClockHandle) == HAL_OK);
	HAL_NVIC_SetPriority(TIMx_IRQn, TickPriority, 0); //Enable the TIMx global Interrupt
	HAL_NVIC_EnableIRQ(TIMx_IRQn); //should sync this better
	return HAL_OK;
}

void HAL_SuspendTick(void)
{
	hal_timer_enable = false;
}
void HAL_ResumeTick(void)
{
	hal_timer_enable = true;
}
/* Macros for word accesses */
#define HW32_REG(ADDRESS) (*((volatile unsigned long *)(ADDRESS)))
typedef struct __t_task {
	uint32_t stack[0x100];// small stack
	void(*task)();
	struct __t_task* next;
	uint32_t id;
} t_task;
#define STACK_SIZE 0x200
uint32_t PSP_stack[16][STACK_SIZE];
uint32_t PSP_array[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }; // Process Stack Pointer for each task
volatile uint32_t curr_task = 0;
bool CreateTask(void(*func)()) {
	uint32_t id = -1;
	t_task* task=NULL;
	for(size_t i=0;i < 16;i++) {
		if(PSP_array[i]!= 0) { id = i; break; }
	}
	if(id == ((uint32_t)-1)) return false;
	PSP_array[id] = ((unsigned int) PSP_stack[id]) + STACK_SIZE - 16*4;
    HW32_REG((PSP_array[id] + (14<<2))) = (unsigned long) func;     // initial Program Counter
    HW32_REG((PSP_array[id] + (15<<2))) = 0x01000000; 				// initial xPSR

    curr_task = 0; // Switch to task #0 (Current task)
    __set_PSP((PSP_array[curr_task] + 16*4)); // Set PSP to topof task 0 stack
    VIC_SetPriority(PendSV_IRQn, 0xFF); // Set PendSV to lowest possible priority
    __set_CONTROL(0x3); // Switch to use Process Stack, unprivilegedstate
     __ISB(); // Execute ISB after changing CONTROL (architecturalrecommendation)
     while(1) ;
}

void SVC_Handler(void)
{
	assert(0);
}
#if 0
void PendSV_Handler(void)
{

}
#endif
void PendSV_Handler(void) { // Context switching code
#if 0
// Simple version - assume No floating point support
// -------------------------
// Save current context
	__asm (


  MRS R0, PSP // Get current process stack pointer value
  STMDB R0!,{R4-R11} // Save R4 to R11 in task stack (8 regs)
  LDR R1,=__cpp(&curr_task)
  LDR R2,[R1] // Get current task ID
  LDR R3,=__cpp(&PSP_array)
  STR R0,[R3, R2, LSL #2] // Save PSP value into PSP_array
  // -------------------------
  // Load next context
  LDR R4,=__cpp(&next_task)
  LDR R4,[R4] // Get next task ID
  STR R4,[R1] // Set curr_task = next_task
  LDR R0,[R3, R4, LSL #2] // Load PSP value from PSP_array
  LDMIA R0!,{R4-R11} // Load R4 to R11 from task   stack (8 regs)
  MSR PSP, R0 // Set PSP to next task
  BX LR // Return
  ALIGN 4
	);
#endif
}


void SysTick_Handler(void)
{
  HAL_SYSTICK_IRQHandler();
#if 0
      switch(curr_task) {
      case(0): next_task=1; break;
      case(1): next_task=2; break;
      case(2): next_task=3; break;
      case(3): next_task=0; break;
      default: next_task=0;
      stop_cpu;
      break; // Should not be here
      }
      if (curr_task!=next_task){ // Context switching needed
      SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; // Set PendSV to pending
      }
      return;
#endif
}
